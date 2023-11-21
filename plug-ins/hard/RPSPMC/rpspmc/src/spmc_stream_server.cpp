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


//#include "xil_cache.c"


#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

#include <DataManager.h>
#include <CustomParameters.h>

#include "spmc_dma.h"

#include "fpga_cfg.h"
#include "pacpll.h"

#include "spmc_stream_server.h"


pthread_attr_t stream_server_attr;
pthread_mutex_t stream_server_mutexsum;
int stream_server_control = 1;
static pthread_t stream_server_thread;

extern spmc_stream_server spmc_stream_server_instance;

extern spmc_dma_support *spm_dma_instance;

//extern volatile uint8_t *FPGA_SPMC_bram;
//extern void *FPGA_SPMC_bram;

extern CIntParameter SPMC_GVP_DATA_POSITION;


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

        std::stringstream position_info;
        int data_len = 0;
        static size_t offset = 0;
        static bool started = false;
        static int position_prev=0;
        
        int position   = stream_lastwrite_address();
        position_info << "{StreamInfo";

        if (stream_server_control & 2){ // started!

                if (spm_dma_instance)
                        spm_dma_instance->start_dma ();

                limit = BRAM_POS_HALF;
                count = 0;
                last_offset = 0;
                offset = 0;
                stream_server_control ^= 2; // remove start flag now
                position_info << "RESET:{true},";
                started = true;
                position=0;
                position_prev=-1;
        }
        
        if (stream_server_control & 4){ // stopped!
                stream_server_control = 1; // remove flags
                position_info << "STOPPED:{true},";
                started = false;
        }
        
        // use delayed position_prev
        if (started){ // started?

                if (spm_dma_instance){
                         spm_dma_instance->print_check_dma_all();
                         fprintf(stderr, "GVP WPos: %d, FIFO R,WPos: %08x, %08x\n", position, read_gpio_reg_int32 (12,0), read_gpio_reg_int32 (12,1));
                }
                 
                if (position_prev >= 0 && gvp_finished ()){ // must transfer data written to block when finished but block not full.
                        position_info << std::hex << ",preFinPos{0x" << position << "}";
                        usleep (50000); // make sure all fifos are emptied...
                        position = stream_lastwrite_address(); // update again
                        stream_server_control = 1; // remove all flags, done after this, last block to finish moving!
                        started = false;
                        position_info << ",FinishedNext:{true}";
                }

                data_len = BRAM_POS_HALF; // always resend updated data block in intervals if GVP is active!

                if ((limit > 0 && position_prev > limit) || (limit == 0 && position_prev < BRAM_POS_HALF)){
                        offset = limit > 0 ? (BRAM_POS_HALF<<2) : 0;
                        limit  = limit > 0 ? 0 : BRAM_POS_HALF;
                        count++;
                }
                position_info << std::hex <<  ",BRAMlimit:{0x" << limit << "}" <<  ",BRAMoffset:{0x" << offset << "}";
        }

        position_info <<  std::hex <<  ",Position:{0x" << ( position_prev >= 0 ? position_prev : 0 ) << std::dec << "},Count:{" << count << "}";

        // flush the cache before reading the data from DRAM by calling
        // Xil_DCacheFlushRange(PARAM_BASE_ADDRESS, 4*1); (defined in xil_cache.c)
        // Xil_DCacheInvalidateRange
        // Xil_DCacheFlushRange(FPGA_SPMC_bram, 4*0x4200);

        // *** =>  Xil_DCacheInvalidateRange ((INTPTR) FPGA_SPMC_bram, 4*0x4200);

        // Xil_L1ICacheInvalidateRange ((INTPTR) FPGA_SPMC_bram, 4*0x4200);

        // use volatile ??? -- issues with types and mmap/unmap

        // int msync(void addr[.length], size_t length, int flags);



        position_info << "}" << std::endl;

        //*** msync ((void *)FPGA_SPMC_bram, 4*0x4200, MS_SYNC);
                        
        // Broadcast to all connections
        con_list::iterator it;
        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                if (info_count > 0)
                        m_endpoint.send(*it, info_stream.str(), websocketpp::frame::opcode::text);
                
                if (data_len > 0 && position_prev != position){
                        //uint8_t *bram = (uint8_t*)FPGA_SPMC_bram + offset;
                        m_endpoint.send(*it, position_info.str(), websocketpp::frame::opcode::text);
                        if (position_prev >= 0){
                                if (spm_dma_instance)
                                        m_endpoint.send(*it, (void*)spm_dma_instance->dma_memdest (), (2*BRAM_POS_HALF) * sizeof(uint32_t), websocketpp::frame::opcode::binary); // full block
                                //m_endpoint.send(*it, (void*) FPGA_SPMC_bram, (0x200+2*BRAM_POS_HALF) * sizeof(uint32_t), websocketpp::frame::opcode::binary); // full block
                                //m_endpoint.send(*it, (void*)bram, data_len * sizeof(uint32_t), websocketpp::frame::opcode::binary); // there is a address mix up at block boundaries ?!?!? WTF
                        }
                } 
#if 0
                if (verbose > 3){
                        m_endpoint.send(*it, (void*)FPGA_SPMC_bram, 2*BRAM_POS_HALF * sizeof(uint32_t), websocketpp::frame::opcode::binary);
                }
#endif
        }
        position_prev=position;

        clear_info_stream ();

        // set timer for next telemetry check
        set_timer(200);
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

