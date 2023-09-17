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

#include "spmc_stream_server.h"


pthread_attr_t stream_server_attr;
pthread_mutex_t stream_server_mutexsum;
int stream_server_control = 1;
static pthread_t stream_server_thread;

extern spmc_stream_server spmc_stream_server_instance;

extern volatile void *FPGA_SPMC_bram;


inline bool gvp_finished (){
        return (read_gpio_reg_int32 (3,1) & 2) ? true:false;
}

inline unsigned int stream_lastwrite_address(){
        return read_gpio_reg_int32 (11,0) & BRAM_ADRESS_MASK;
}


void spmc_stream_server::on_timer(websocketpp::lib::error_code const & ec) {
        if (ec) {
                // there was an error, stop telemetry
                m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                            "Timer Error: "+ec.message());
                return;
        }

        std::stringstream val;
        int data_len = 0;
        int offset = 0;
                
        if (verbose > 2){
                val << " ** Telemetry Server: # " << m_count++ << "** ";
                val << " stream_ctrl: " << stream_server_control;
        }
        if (stream_server_control & 2){ // started!
                limit = BRAM_POS_HALF;
                count = 0;
                stream_server_control = (stream_server_control & 0x03); // remove start flag now
        }

        int position   = stream_lastwrite_address();


        if (stream_server_control & 2){ // started?
                if (verbose > 1) val << " position: " << position;
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
                                      << std::endl;
        } else
                if (verbose > 2) val << " **GVP is idle **" << std::endl;
               
        if (verbose > 2) val << " ** SENDING FULL BRAM DUMP **" << std::endl;

        // Broadcast count to all connections
        con_list::iterator it;
        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                if (verbose > 1 && val.str().length()>0)  m_endpoint.send(*it, val.str(), websocketpp::frame::opcode::text);
                if (info_count > 0)
                        m_endpoint.send(*it, info_stream.str(), websocketpp::frame::opcode::text);

                if (data_len > 0){
                        m_endpoint.send(*it, (void*)FPGA_SPMC_bram+offset, data_len * sizeof(uint32_t), websocketpp::frame::opcode::binary);
                } 
                if (verbose > 2){
                        m_endpoint.send(*it, (void*)FPGA_SPMC_bram, 2*BRAM_POS_HALF * sizeof(uint32_t), websocketpp::frame::opcode::binary);
                }
        }

        clear_info_stream ();

        // set timer for next telemetry check
        if (data_len > 0)
                set_timer(250);
        else
                set_timer();
}


void spmc_stream_server::on_http(connection_hdl hdl) {
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

// stream server thread
void *thread_stream_server (void *arg) {
        if (verbose > 1) fprintf(stderr, "Starting thread: Telemetry stream_server test\n");

        std::string docroot;

        docroot = std::string("spmcstream");

        spmc_stream_server_instance.run(docroot, TCP_PORT);

        if (verbose > 1) fprintf(stderr, "Stream server exiting.\n");
        return NULL;
}

