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

#pragma once

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
#include <websocketpp/connection.hpp>
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

#define BRAM_SIZE        0x4000       // (== 1<<14,  14 bit address)
#define BRAM_POS_HALF    0x2000
#define BRAM_ADRESS_MASK 0x3fff

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
                info_count=0;
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
#if 1
        size_t getBufferedAmount() {
                con_list::iterator it;
                size_t webs_nbuf=0;
                for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                        if (it != m_connections.end()){
                                server::connection_ptr connection = m_endpoint.get_con_from_hdl (*it);
                                webs_nbuf += connection->get_buffered_amount();
                        }
                }

                // m_endpoint.get_con_from_hdl (*it)->get_buffered_amount();

                return webs_nbuf;
        }
#endif
#if 0        
        void wait_for_buffer_empty (const char *msg){
                size_t webs_nbuf=getBufferedAmount();
                if (webs_nbuf > 0)
                        fprintf(stderr, "**[%s] WebSocket Buffered Amount: %d\n", msg, webs_nbuf);
        }
#endif   
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

        void on_timer(websocketpp::lib::error_code const & ec);
        void on_http(connection_hdl hdl);
        
        void on_open(connection_hdl hdl) {
                m_connections.insert(hdl);
                add_greeting ();
        };

        void on_close(connection_hdl hdl) {
                m_connections.erase(hdl);
        };

        void add_greeting ();
        
        void add_vector (std::string vec){
                info_stream << vec << std::endl;
                info_count++;
        };
        void add_info (std::string info){
                info_stream << "{Info: {" << info << "}}" << std::endl;
                info_count++;
        };
        void add_xyz_info (double x, double y, double z){
                info_stream << "{XYZInfo: {" << x << ',' << y << ',' << z << "}}" << std::endl;
                info_count++;
        };
        void clear_info_stream(){
                info_stream.str("");
                info_count=0;
                info_x=0;
        };
        
private:
        typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;
    
        server m_endpoint;
        con_list m_connections;
        
        server::timer_ptr m_timer;
    
        std::string m_docroot;

        // Telemetry data
        uint64_t m_count;
        int info_count;
        int info_x;
        std::stringstream info_stream;

        int limit;
        int count;
        int last_offset;
};
