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

//#define SPMC_DMA_BUFFER_BLOCK_SIZE         0x00080000 // Size of memory block per descriptor in bytes: 0.5 M -- two blocks, cyclic
#define BLKSIZE ((SPMC_DMA_N_DESC*SPMC_DMA_BUFFER_BLOCK_SIZE)/4) // 2 * 0x00080000 / 4 = 0x40000

inline bool gvp_finished (){
        return (read_gpio_reg_int32 (3,1) & 2) ? true:false;
}

inline unsigned int stream_lastwrite_address(){
        return read_gpio_reg_int32 (11,0) & (BLKSIZE-1); // 20bit 0x3FFFF   // & BRAM_ADRESS_MASK;
}


void spmc_stream_server::on_timer(websocketpp::lib::error_code const & ec) {
        if (ec) {
                // there was an error, stop telemetry
                m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                            "Stream Server Timer Error: "+ec.message());
                return;
        }

#if 1
        if (verbose > 2){
                size_t webs_nbuf=getBufferedAmount();
                if (webs_nbuf > 16000){
                        fprintf(stderr, "** Skipping ** WebSocket Buffered Amount: %d\n", webs_nbuf);
                        return;
                }else{
                        fprintf(stderr, "WebSocket Buffered Amount: %d\n", webs_nbuf);
                }
        }
#endif
        
        std::stringstream position_info;
        static bool started = false;
        static bool data_valid = false;
        static int position_prev=0;
        int stream_position_info_count=0;
        
        unsigned int *dma_mem = spm_dma_instance->dma_memdest (); // 2M total, for descriptors (unavalabale for data) and two DMA blocks a 0x80000 bytes at this address
        
        int position = stream_lastwrite_address();
        if (spm_dma_instance->get_s2mm_nth_status(0) == 0)
                position = position > 32 ? position - 32 : 0;
      
        position_info << "{StreamInfo";

        if (stream_server_control & 2){ // started!
                count = 0;
                stream_server_control ^= 2; // remove start flag now
                position_info << "RESET:{true},";
                started = true;
                data_valid = false;
                position=0;
                position_prev=0;
                stream_position_info_count++;
        }
        
        if (stream_server_control & 4){ // stopped!
                stream_server_control = 1; // remove flags
                position_info << "STOPPED:{true},";
                started = false;
                stream_position_info_count++;
        }
        
        // use delayed position_prev
        if (started){ // started?

                // check for valid header

                if (!data_valid && (dma_mem[0] & 0xffff) == 0xffff)
                        data_valid = true;

                if (data_valid){
                        int lag=0;
                        while (position > 1 && dma_mem[position] == 0xdddddddd)
                                position--, lag++;
                        if (lag){
                                fprintf(stderr, "... position-- lag=%d pos=0x%08x ** DMA_DA0_off=0x%08x\n", lag, position, spm_dma_instance->get_dmaps_DAn_offset(0));
                                // for (int dn=0; dn<8; ++dn) fprintf(stderr, " ** DMA_DA%d_off=0x%06x\n", dn, spm_dma_instance->get_dmaps_DAn_offset(dn));
                        }
                }

                fprintf(stderr, "GVP WPos: %d, A: %08x, B: %08x  *I: %08x Data: %s DA: %08x\n", position, spm_dma_instance->get_s2mm_nth_status(0), spm_dma_instance->get_s2mm_nth_status(1), dma_mem[0], data_valid?"VALID":"WAITING", spm_dma_instance->get_s2mm_destination_address());
                spm_dma_instance->print_check_dma_all();
                        
                // WAIT FOR LAST DATA HACK -- NO GOOD FOR CYCLIC
                if (data_valid && gvp_finished ()){
                        usleep(50000);
                        fprintf(stderr, "GVP FINISHED ** CHECKING END OF DATA\n");
                        position = stream_lastwrite_address();
                        while (position > 1 && dma_mem[position] == 0xdddddddd){
                                position--;
                                fprintf(stderr, "FIN ** position-- %08x\n", position);
                        }
                }
        }

        if ( data_valid && position_prev != position ){
                position_info <<  std::hex <<  ",Position:{0x" << position_prev << std::dec << "},Count:{" << count << "}";
                stream_position_info_count++;
        }
        position_info << "}" << std::endl;

        // Broadcast to all connections
        con_list::iterator it;
        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                if (info_count > 0)
                        m_endpoint.send(*it, info_stream.str(), websocketpp::frame::opcode::text);

                if (stream_position_info_count > 0)
                        m_endpoint.send(*it, position_info.str(), websocketpp::frame::opcode::text);

                if (data_valid && started && position_prev != position){

                        if (verbose > 3)
                                fprintf(stderr, "Buffered amount = %d\n", m_endpoint.get_con_from_hdl (*it)->get_buffered_amount());
                        
                        int n  = position_prev < position ? position - position_prev : 0;
                        const int max_msg = 32768;
                        if (n > 0){
                                fprintf(stderr, "Block: [%08x : %08x] %d\n", position_prev, position, n);
                                //spm_dma_instance->memdump_from (dma_mem, n, position_prev);
                                int o=0;
                                //wait_for_buffer_empty("W1");
                                while (n > max_msg){
                                        //wait_for_buffer_empty("W1n");
                                        m_endpoint.send(*it, (void*)(&dma_mem[position_prev+o]), max_msg*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                        n -= max_msg;
                                        o += max_msg;
                                }
                                if (n > 0)
                                        m_endpoint.send(*it, (void*)(&dma_mem[position_prev+o]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                        } else {
                                fprintf(stderr, "BlockLoop: [%08x : END, 0 : %08x]\n", position_prev, position);
                                //spm_dma_instance->memdump_from (dma_mem, BLKSIZE-position_prev, position_prev);
                                n = BLKSIZE-position_prev;
                                int o=0;
                                //wait_for_buffer_empty("W2");
                                while (n > max_msg){
                                        //wait_for_buffer_empty("W2na");
                                        m_endpoint.send(*it, (void*)(&dma_mem[position_prev+o]), max_msg*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                        n -= max_msg;
                                        o += max_msg;
                                }
                                if (n > 0)
                                        m_endpoint.send(*it, (void*)(&dma_mem[position_prev+o]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                //m_endpoint.send(*it, (void*)(&dma_mem[position_prev]), (BLKSIZE-position_prev)*sizeof(uint32_t), websocketpp::frame::opcode::binary);

                                //spm_dma_instance->memdump_from (dma_mem, position, 0);
                                n = position;
                                o=0;
                                while (n > max_msg){
                                        //wait_for_buffer_empty("W2nb");
                                        m_endpoint.send(*it, (void*)(&dma_mem[o]), max_msg*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                        n -= max_msg;
                                        o += max_msg;
                                }
                                if (n > 0)
                                        m_endpoint.send(*it, (void*)(&dma_mem[o]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                //m_endpoint.send(*it, (void*)(&dma_mem[0]), (position)*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                count++;
                        }
                } 
        }

        if (data_valid){
                position_prev=position;
                if (started && gvp_finished ()){
                        std::stringstream position_fin;
                        position = stream_lastwrite_address();
                        fprintf(stderr, "GVP Finished pos=0x%08x\n", position);
                        stream_server_control = 1; // remove all flags, done after this, last block to finish moving!
                        started = false;
                        position_fin << "{StreamInfo,END,FinishedNext:{true}" << std::hex <<  ",Position:{0x" << position << std::dec << "},Count:{" << count << "}}";
                        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                                m_endpoint.send(*it, position_fin.str(), websocketpp::frame::opcode::text);
                        }
                        rp_spmc_gvp_config (true, false, false);
                }
        }
        
        clear_info_stream ();

        // set timer for next telemetry
        set_timer(50);
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
        if (verbose > 1) fprintf(stderr, "Starting thread: SPMC stream_server test\n");

        std::string docroot;

        docroot = std::string("spmcstream");

        spmc_stream_server_instance.run(docroot, TCP_PORT);

        if (verbose > 1) fprintf(stderr, "SPMC Stream server exiting.\n");
        return NULL;
}

