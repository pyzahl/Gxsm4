/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,..2023 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * additional features: Andreas Klust <klust@users.sf.net>
 * WWW Home: http://gxsm.sf.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>



#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

#include <DataManager.h>
#include <CustomParameters.h>

#include "fpga_cfg.h"
#include "pacpll.h"

#define TCP_PORT 9003

#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

pthread_attr_t stream_server_attr;
pthread_mutex_t stream_server_mutexsum;
int stream_server_control = 1;
static pthread_t stream_server_thread;

extern volatile void *FPGA_SPMC_bram;

#define BRAM_SIZE        16384            // (14 bit address)
#define BRAM_POS_HALF    (BRAM_SIZE>>1)
#define BRAM_ADRESS_MASK 0x3fff

inline bool gvp_finished (){
        return (read_gpio_reg_int32 (3,1) & 2) ? true:false;
}

inline unsigned int stream_lastwrite_address(){
        return read_gpio_reg_int32 (11,0) & BRAM_ADRESS_MASK;
}

class spmc_stream_server {
public:
        typedef websocketpp::connection_hdl connection_hdl;
        typedef websocketpp::server<websocketpp::config::asio> server;

        spmc_stream_server() : m_count(0) {
                // set up access channels to only log interesting things
                m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
                m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
                m_endpoint.set_access_channels(websocketpp::log::alevel::app);

                // Initialize the Asio transport policy
                m_endpoint.init_asio();

                // Bind the handlers we are using
                using websocketpp::lib::placeholders::_1;
                using websocketpp::lib::bind;
                m_endpoint.set_open_handler(bind(&spmc_stream_server::on_open,this,_1));
                m_endpoint.set_close_handler(bind(&spmc_stream_server::on_close,this,_1));
                m_endpoint.set_http_handler(bind(&spmc_stream_server::on_http,this,_1));

                limit = BRAM_POS_HALF;
                count = 0;
        }

        void run(std::string docroot, uint16_t port) {
                std::stringstream ss;
                ss << "Running telemetry server on port "<< port <<" using docroot=" << docroot;
                m_endpoint.get_alog().write(websocketpp::log::alevel::app,ss.str());
        
                m_docroot = docroot;
        
                // listen on specified port
                m_endpoint.listen(port);

                // Start the server accept loop
                m_endpoint.start_accept();

                // Set the initial timer to start telemetry
                set_timer();

                // Start the ASIO io_service run loop
                try {
                        m_endpoint.run();
                } catch (websocketpp::exception const & e) {
                        std::cout << e.what() << std::endl;
                }
        }

        void set_timer(int ms=1000) {
                m_timer = m_endpoint.set_timer(
                                               ms,
                                               websocketpp::lib::bind(
                                                                      &spmc_stream_server::on_timer,
                                                                      this,
                                                                      websocketpp::lib::placeholders::_1
                                                                      )
                                               );
        }

        void on_timer(websocketpp::lib::error_code const & ec) {
                if (ec) {
                        // there was an error, stop telemetry
                        m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                                    "Timer Error: "+ec.message());
                        return;
                }
        
                std::stringstream val;
                int data_len = 0;
                int offset = 0;
                
                if (verbose > 1){
                        val << " ** Telemetry Server: # " << m_count++ << "** ";
                        val << " stream_ctrl: " << stream_server_control;
                }
                if (stream_server_control & 2){ // started!
                        limit = BRAM_POS_HALF;
                        count = 0;
                        stream_server_control = (stream_server_control & 0x03); // remove start flag now
                }

                int position   = stream_lastwrite_address();

                if (verbose > 1) val << " position: " << position;

                if (stream_server_control & 2){ // started?
                        if (gvp_finished ()){ // must transfer data written to block when finished but block not full.
                                stream_server_control = 1; // remove all flags, done after this, last block to finish moving!
                                if (verbose > 1){
                                        val << " ** GVP FINISHED ** stream_ctrl: " << stream_server_control;
                                }
                                int clear_len = BRAM_SIZE-position-1;
                                if (clear_len > 0){
                                        //memset ( FPGA_SPMC_bram + sizeof(uint32_t) * (position+1), 0, clear_len);
                                        position += clear_len;
                                } else clear_len = 0;
                        }
                        if ((limit > 0 && position > limit) || (limit = 0 && position < BRAM_POS_HALF)){
                                offset = (limit > 0 ? 0 : BRAM_POS_HALF)<<2;
                                limit  =  limit > 0 ? 0 : BRAM_POS_HALF;
                                data_len = BRAM_POS_HALF;
                                if (verbose > 1){
                                        count++;
                                        val << " ** DATA ** COUNT: " << count;
                                }
                        }
                        if (verbose > 1) val  << (data_len > 0 ? " *SENDING BLOCK* " : "*WAITING FOR DATA*")
                                              << " offset: " << offset
                                                 ;
                } else
                        if (verbose > 1) val << " **GVP is idle **";

                if (verbose > 1) val << "\n";
                
                // Broadcast count to all connections
                con_list::iterator it;
                for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                        if (verbose > 1)  m_endpoint.send(*it, val.str(),websocketpp::frame::opcode::text);
                        if (data_len > 0){
                                //m_endpoint.send(*it, (void*)FPGA_SPMC_bram, 1024 * sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                m_endpoint.send(*it, (void*)FPGA_SPMC_bram+offset, data_len * sizeof(uint32_t), websocketpp::frame::opcode::binary);
                        }
                }

                // set timer for next telemetry check
                if (data_len > 0)
                        set_timer(250);
                else
                        set_timer();
        }

        void on_http(connection_hdl hdl) {
                // Upgrade our connection handle to a full connection_ptr
                server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    
                std::ifstream file;
                std::string filename = con->get_resource();
                std::string response;
    
                m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                            "http request1: "+filename);
    
                if (filename == "/") {
                        filename = m_docroot+"index.html";
                } else {
                        filename = m_docroot+filename.substr(1);
                }
        
                m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                            "http request2: "+filename);
    
                file.open(filename.c_str(), std::ios::in);
                if (!file) {
                        // 404 error
                        std::stringstream ss;
        
                        ss << "<!doctype html><html><head>"
                           << "<title>RP-SPMC-STREAM-WS: Error 404 (Resource not found)</title><body>"
                           << "<h1>Error 404</h1>"
                           << "<p>The requested URL " << filename << " was not found on this server.</p>"
                           << "</body></head></html>";
        
                        con->set_body(ss.str());
                        con->set_status(websocketpp::http::status_code::not_found);
                        return;
                }
    
                file.seekg(0, std::ios::end);
                response.reserve(file.tellg());
                file.seekg(0, std::ios::beg);
    
                response.assign((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
    
                con->set_body(response);
                con->set_status(websocketpp::http::status_code::ok);
        }

        void on_open(connection_hdl hdl) {
                m_connections.insert(hdl);
        }

        void on_close(connection_hdl hdl) {
                m_connections.erase(hdl);
        }
private:
        typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
    
        server m_endpoint;
        con_list m_connections;
        server::timer_ptr m_timer;
    
        std::string m_docroot;

        // Telemetry data
        uint64_t m_count;

        int limit;
        int count;
};

// stream server thread
void *thread_stream_server(void *arg) {
        if (verbose > 1) fprintf(stderr, "Starting thread: Telemetry stream_server test\n");
        
        spmc_stream_server s;
        std::string docroot;
        
        docroot = std::string("spmcstream");
        s.run(docroot, TCP_PORT);
        
        if (verbose > 1) fprintf(stderr, "Stream server exiting.\n");
        return NULL;
}
