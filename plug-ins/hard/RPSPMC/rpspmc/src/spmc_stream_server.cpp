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

//extern int verbose;
//extern int stream_debug_flags;

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
                set_timer(100);
                return;
        }

#if 1
        {       // make sure all got send
                size_t webs_nbuf=getBufferedAmount();
                if (webs_nbuf > 0){
                        fprintf(stderr, "** Skipping ** WebSocket Buffered Amount: %d\n", webs_nbuf);
                        set_timer(100);
                        return;
                }
        }
#endif
        
        std::stringstream position_info;
        static bool started = false;
        static bool data_valid = false;
        static int position_prev=0;
        int stream_position_info_count=0;
        static int DDposition = 0;
        static int DDposition_prev = 0;
        
        unsigned int *dma_mem = spm_dma_instance->dma_memdest (); // 2M total, for descriptors (unavalabale for data) and two DMA blocks a 0x80000 bytes at this address

#define MEM_DUMP_ON
#define MEM_DUMP_CYCLE_ON
        
#define CLEAR_SEND_DMA_MEM
#ifdef CLEAR_SEND_DMA_MEM
        if (DDposition_prev != DDposition){
                // clear/invalidate already send mem with 0xdddddddd
                int n  = DDposition_prev < DDposition ? DDposition - DDposition_prev : 0;
                fprintf(stderr, "*** Clearing Send Block w DDDDDDDD: [%08x : %08x] size= %d ***\n", DDposition_prev, DDposition, n);
                if (n > 0){
#ifdef MEM_DUMP_ON
                        if (stream_debug_flags & 4)
                                spm_dma_instance->memdump_from (&dma_mem[DDposition_prev], 64, DDposition_prev); //n
#endif
                        if (!(stream_debug_flags & 2))
                                memset((void*)(&dma_mem[DDposition_prev]), 0xdd, n*sizeof(uint32_t));
                } else {
                        fprintf(stderr, "*** DMA CYCLING detected [%08x : %08x] ***\n", DDposition_prev, DDposition);
                        n = BLKSIZE-DDposition_prev;
                        if (n > 0){
#ifdef MEM_DUMP_CYCLE_ON
                                if (stream_debug_flags & 3)
                                        spm_dma_instance->memdump_from (&dma_mem[DDposition_prev], 64, DDposition_prev); //n
#endif
                                if (!(stream_debug_flags & 2))
                                        memset((void*)(&dma_mem[DDposition_prev]), 0xdd, n*sizeof(uint32_t));
                        }
                        n = DDposition;
                        if (n > 0){
#ifdef MEM_DUMP_CYCLE_ON
                                if (stream_debug_flags & 3)
                                        spm_dma_instance->memdump_from (&dma_mem[0], 64, 0); // n
#endif
                                if (!(stream_debug_flags & 2))
                                        memset((void*)(&dma_mem[0]), 0xdd, n*sizeof(uint32_t));
                        }
                }
                DDposition_prev = DDposition = 0;
        }
#endif


        
        int position = stream_lastwrite_address();


        
        static int p__pos=0;
        static int p__sc=0;
        static int ttic = 0;

        ttic++;
        
        if (p__pos != position || p__sc != stream_server_control || ttic % 64 == 0){
                fprintf(stderr, "STREAM SERVER TIMER FUNC control=%d position=%08x <- %08x DMAmem[0]=%08x\n", stream_server_control, position, position_prev, dma_mem[0]);
                p__pos = position;
                p__sc  = stream_server_control;
        }
        
        position_info << "{StreamInfo::";

        if (stream_server_control == 1){
                ; //started = false;
        }                
        
        if (stream_server_control & 2){ // started!
                count = 0;
                stream_server_control ^= 2; // remove start flag now
                position_info << "RESET:{true},";
                started = true;
                data_valid = false;
                position=0;
                position_prev=0;
                DDposition = 0;
                DDposition_prev = 0;
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
                        fprintf(stderr, "*** Detecting DMA stream END lag ***\n");
                        position += BLKSIZE;
                        while (position > (BLKSIZE>>1) && dma_mem[position%BLKSIZE] == 0xdddddddd && dma_mem[(position-1)%BLKSIZE] == 0xdddddddd)
                                position--, lag++;
                        position = position % BLKSIZE;
                        if (lag){
                                fprintf(stderr, "... position-- lag=%d pos=0x%08x ** DMA_DA0_off=0x%08x\n", lag, position, spm_dma_instance->get_dmaps_DAn_offset(0));
#ifdef MEM_DUMP_ON
                                if (position > 64)
                                        spm_dma_instance->memdump_from (dma_mem, lag+64, position-64);
                                // for (int dn=0; dn<8; ++dn) fprintf(stderr, " ** DMA_DA%d_off=0x%06x\n", dn, spm_dma_instance->get_dmaps_DAn_offset(dn));
#endif
                        }
                }

                fprintf(stderr, "GVP WPos: %x08x [<-%08x], A: %08x, B: %08x  *I: %08x Data: %s DA: %08x\n", position, position_prev, spm_dma_instance->get_s2mm_nth_status(0), spm_dma_instance->get_s2mm_nth_status(1), dma_mem[0], data_valid?"VALID":"WAITING", spm_dma_instance->get_s2mm_destination_address());
                //spm_dma_instance->print_check_dma_all();
                        
                fprintf(stderr, "GVP data[0]=%08x, data[WPos%d] = %08x\n", dma_mem[0], position, dma_mem[position]);
                
                // WAIT FOR LAST DATA HACK -- NO GOOD FOR CYCLIC
                if (data_valid && gvp_finished ()){
                        position = stream_lastwrite_address();
                        fprintf(stderr, "GVP Finished pos=0x%08x\n", position);
                        position_info << "END,FinishedNext:{true}" << std::hex <<  ",Position:{0x" << position << std::dec << "},Count:{" << count << "},";

                        // wait for DMA to complete last bytes if any
                        // HACK -- need to fix via DMA WRITE ADDRESS (spm_dma_instance->get_dmaps_DAn_offset(0))
                        int k=20;
                        while (k && dma_mem[position] == 0xdddddddd){
                                usleep(10000); --k;
                        }
                        position = stream_lastwrite_address();
                        fprintf(stderr, "GVP FINISHED ** CHECKING END OF DATA @ position = %08x\n", position);
                        while (position > 1 && dma_mem[position] == 0xdddddddd){
                                position--;
                                fprintf(stderr, "FIN ** position-- %08x\n", position);
                        }
                        fprintf(stderr, "GVP FINISHED. Stream server on standy after last package send out.\n");

                        stream_server_control = (stream_server_control & 0x01) | 4; // set stop bit
                }
        }

        if ( data_valid && position_prev != position ){
                position_info <<  std::hex <<  ",Position:{0x" << position_prev << std::dec << "},Count:{" << count << "}";
                stream_position_info_count++;
        }
        position_info << "}" << std::endl;

        // Broadcast to all connections
        con_list::iterator it;
        int connection=false;
        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                if (it == m_connections.end()) break;
                connection=true;
                if (info_count > 0)
                        m_endpoint.send(*it, info_stream.str(), websocketpp::frame::opcode::text);

                if (stream_position_info_count > 0)
                        m_endpoint.send(*it, position_info.str(), websocketpp::frame::opcode::text);

                if (data_valid && position_prev != position){
                        fprintf(stderr, "*** SENDING NEW DATA ***\n");

                        //if (verbose > 3)
                        //        fprintf(stderr, "Buffered amount = %d\n", m_endpoint.get_con_from_hdl (*it)->get_buffered_amount());
                        
                        int n  = position_prev < position ? position - position_prev : 0;
//#define LIMIT_MESSAGE_SIZE // break up and send in limited sizes chuncs
#ifdef LIMIT_MESSAGE_SIZE
                        const int max_msg = 32768;
                        if (n > 0){
                                fprintf(stderr, "Block: [%08x : %08x] size= %d ... ", position_prev, position, n);
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
                                fprintf(stderr, "send.\n");
                        } else {
                                fprintf(stderr, "BlockLoop: [%08x : END, 0 : %08x] ... ", position_prev, position);
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
                                fprintf(stderr, "send.\n");
                                count++;
                        }
#else // send full block(s)
                        if (n > 0){
                                fprintf(stderr, "Block: [%08x : %08x] size= %d ... ", position_prev, position, n);
                                //spm_dma_instance->memdump_from (dma_mem, n, position_prev);
                                m_endpoint.send(*it, (void*)(&dma_mem[position_prev]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                fprintf(stderr, "send.\n");
                        } else {
                                fprintf(stderr, "BlockLoop: [%08x : END, 0 : %08x] ... ", position_prev, position);
                                //spm_dma_instance->memdump_from (dma_mem, BLKSIZE-position_prev, position_prev);
                                n = BLKSIZE-position_prev;
                                if (n > 0)
                                        m_endpoint.send(*it, (void*)(&dma_mem[position_prev]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);

                                //spm_dma_instance->memdump_from (dma_mem, position, 0);
                                n = position;
                                if (n > 0)
                                        m_endpoint.send(*it, (void*)(&dma_mem[0]), n*sizeof(uint32_t), websocketpp::frame::opcode::binary);
                                fprintf(stderr, "send.\n");
                                count++;
                        }
#endif
                } 
        }
        if (connection){
                if (data_valid && position_prev != position){
                        DDposition = position;
                        DDposition_prev = position_prev;
                        // update last position now
                        position_prev=position;
                }
        } else {
                if (data_valid && ttic % 64 == 0)
                        fprintf(stderr, "Data OK. BUT: No Socket Connection/lost? Pending to send: [%08x : %08x]\n", position_prev, position);
        }
        
        clear_info_stream ();

        // set timer for next telemetry
        set_timer(100);
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

/*
https://adaptivesupport.amd.com/s/question/0D52E00006iHj3pSAC/axi-dma-sg-cyclic-mode-missing-data?language=en_US
We were able to solve our AXI_DMA cylic issues:
1) created a FIFO with a special "reset" function and TLAST generation
   a) when you assert "reset," it flushes the contents
   b) empty flag tells you when its safe to configure the AXI-DMA unit
   c) all input is dropped until the "reset" is released
   d) input is packetized with TLAST generated based on register-controlled size
2) ensure that no data is flowing to the input AXI-S port until you set up the DMA - then release "reset" on the FIFO
3) carefully manage contention for downstream memory resources
4) don't rely on the run/stop bit to actually stop the cyclic DMA activity - stop the input then reset the DMA controller

We have multi-channel off; DRE on/off doesn't seem to matter; the BD handles the TKEEP; our SG memory is independent from the DMA-MM memory
  
**  FPGA A9 /dev/mem DMA MEMORY AREA DUMP **

** DESCRIPTORS FOR S2MM **
32bit word hexdump starting at 0x01001000:
B[00000000] W[00000000] D 000000: 01000040 00000000 01001000 00000000 00000000 00000000 00080000 88080000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
B[00000040] W[00000010] D 000016: 01000080 00000000 01081000 00000000 00000000 00000000 00080000 80080000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
B[00000080] W[00000020] D 000032: 01000000 00000000 01101000 00000000 00000000 00000000 00000050 80000050 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000
B[000000c0] W[00000030] D 000048: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000

** TARGET MEM FOR S2MM **

...last data at end of block from previous write before looping back to start, correct last entry 00040000:

B[000fffc0] W[0003fff0] D 262128: 0003fff1 0003fff2 0003fff3 0003fff4 0003fff5 0003fff6 0003fff7 0003fff8 0003fff9 0003fffa 0003fffb 0003fffc 0003fffd 0003fffe 0003ffff 00040000

...but missing words 00040001 .. 00040014 while cycling, dump after 1st DMA cycling:

START BLOCK AB: 32bit word hexdump starting at 0x01001000:
B[00000000] W[00000000] D 000000: 00040015 00040016 00040017 00040018 00040019 0004001a 0004001b 0004001c 0004001d 0004001e 0004001f 00040020 00040021 00040022 00040023 00040024
B[00000040] W[00000010] D 000016: 00040025 00040026 00040027 00040028 00040029 0004002a 0004002b 0004002c 0004002d 0004002e 0004002f 00040030 00040031 00040032 00040033 00040034
B[00000080] W[00000020] D 000032: 00040035 00040036 00040037 00040038 00040039 0004003a 0004003b 0004003c 0004003d 0004003e 0004003f 00040040 00040041 00040042 00040043 00040044
B[000000c0] W[00000030] D 000048: 00040045 00040046 00040047 00040048 00040049 0004004a 0004004b 0004004c 0004004d 0004004e 0004004f 00040050 00040051 00040052 00040053 00040054
B[00000100] W[00000040] D 000064: 00040055 00040056 00040057 00040058 00040059 0004005a 0004005b 0004005c 0004005d 0004005e 0004005f 00040060 00040061 00040062 00040063 00040064
B[00000140] W[00000050] D 000080: 00040065 00040066 00040067 00040068 00040069 0004006a 0004006b 0004006c 0004006d 0004006e 0004006f 00040070 00040071 00040072 00040073 00040074
B[00000180] W[00000060] D 000096: 00040075 00040076 00040077 00040078 00040079 0004007a 0004007b 0004007c 0004007d 0004007e 0004007f 00040080 00040081 00040082 00040083 00040084
B[000001c0] W[00000070] D 000112: 00040085 00040086 00040087 00040088 00040089 0004008a 0004008b 0004008c 0004008d 0004008e 0004008f 00040090 00040091 00040092 00040093 00040094
B[00000200] W[00000080] D 000128: 00040095 00040096 00040097 00040098 00040099 0004009a 0004009b 0004009c 0004009d 0004009e 0004009f 000400a0 000400a1 000400a2 000400a3 000400a4
B[00000240] W[00000090] D 000144: 000400a5 000400a6 000400a7 000400a8 000400a9 000400aa 000400ab 000400ac 000400ad 000400ae 000400af 000400b0 000400b1 000400b2 000400b3 000400b4
B[00000280] W[000000a0] D 000160: 000400b5 000400b6 000400b7 000400b8 000400b9 000400ba 000400bb 000400bc 000400bd 000400be 000400bf 000400c0 000400c1 000400c2 000400c3 000400c4
B[000002c0] W[000000b0] D 000176: 000400c5 000400c6 000400c7 000400c8 000400c9 000400ca 000400cb 000400cc 000400cd 000400ce 000400cf 000400d0 000400d1 000400d2 000400d3 000400d4
B[00000300] W[000000c0] D 000192: 000400d5 000400d6 000400d7 000400d8 000400d9 000400da 000400db 000400dc 000400dd 000400de 000400df 000400e0 000400e1 000400e2 000400e3 000400e4
B[00000340] W[000000d0] D 000208: 000400e5 000400e6 000400e7 000400e8 000400e9 000400ea 000400eb 000400ec 000400ed 000400ee 000400ef 000400f0 000400f1 000400f2 000400f3 000400f4
...
B[00033000] W[0000cc00] D 052224: 0004cc15 0004cc16 0004cc17 0004cc18 0004cc19 0004cc1a 0004cc1b 0004cc1c 0004cc1d 0004cc1e 0004cc1f 0004cc20 0004cc21 0004cc22 0004cc23 0004cc24
B[00033040] W[0000cc10] D 052240: 0004cc25 0004cc26 0004cc27 0004cc28 0004cc29 0004cc2a 0004cc2b 0004cc2c 0004cc2d 0004cc2e 0004cc2f 0004cc30 0004cc31 0004cc32 0004cc33 0004cc34
B[00033080] W[0000cc20] D 052256: 0004cc35 0004cc36 0004cc37 0004cc38 0004cc39 0004cc3a 0004cc3b 0004cc3c 0004cc3d 0004cc3e 0004cc3f 0004cc40 0004cc41 0004cc42 0004cc43 0004cc44
B[000330c0] W[0000cc30] D 052272: 0004cc45 0004cc46 0004cc47 0004cc48 0004cc49 0004cc4a 0004cc4b 0004cc4c 0004cc4d 0004cc4e 0004cc4f 0004cc50 0004cc51 0004cc52 0004cc53 0004cc54
B[00033100] W[0000cc40] D 052288: 0004cc55 0004cc56 0004cc57 0004cc58 0004cc59 0004cc5a 0004cc5b 0004cc5c 0004cc5d 0004cc5e 0004cc5f 0004cc60 0004cc61 0004cc62 0004cc63 0004cc64
B[00033140] W[0000cc50] D 052304: 0004cc65 0004cc66 0004cc67 0004cc68 0004cc69 0004cc6a 0004cc6b 0004cc6c 0004cc6d 0004cc6e 0004cc6f 0004cc70 0004cc71 0004cc72 0004cc73 0004cc74
B[00033180] W[0000cc60] D 052320: 0004cc75 0004cc76 0004cc77 0004cc78 0004cc79 0004cc7a 0004cc7b 0004cc7c 0004cc7d 0004cc7e 0004cc7f 0004cc80 0004cc81 0004cc82 0004cc83 0004cc84
B[000331c0] W[0000cc70] D 052336: 0004cc85 0004cc86 0004cc87 0004cc88 0004cc89 0004cc8a 0004cc8b 0004cc8c 0004cc8d 0004cc8e 0004cc8f 0004cc90 0004cc91 0004cc92 0004cc93 0004cc94
B[00033200] W[0000cc80] D 052352: 0000cc81 0000cc82 0000cc83 0000cc84 0000cc85 0000cc86 0000cc87 0000cc88 0000cc89 0000cc8a 0000cc8b 0000cc8c 0000cc8d 0000cc8e 0000cc8f 0000cc90
B[00033240] W[0000cc90] D 052368: 0000cc91 0000cc92 0000cc93 0000cc94 0000cc95 0000cc96 0000cc97 0000cc98 0000cc99 0000cc9a 0000cc9b 0000cc9c 0000cc9d 0000cc9e 0000cc9f 0000cca0
B[00033280] W[0000cca0] D 052384: 0000cca1 0000cca2 0000cca3 0000cca4 0000cca5 0000cca6 0000cca7 0000cca8 0000cca9 0000ccaa 0000ccab 0000ccac 0000ccad 0000ccae 0000ccaf 0000ccb0
B[000332c0] W[0000ccb0] D 052400: 0000ccb1 0000ccb2 0000ccb3 0000ccb4 0000ccb5 0000ccb6 0000ccb7 0000ccb8 0000ccb9 0000ccba 0000ccbb 0000ccbc 0000ccbd 0000ccbe 0000ccbf 0000ccc0
B[00033300] W[0000ccc0] D 052416: 0000ccc1 0000ccc2 0000ccc3 0000ccc4 0000ccc5 0000ccc6 0000ccc7 0000ccc8 0000ccc9 0000ccca 0000cccb 0000cccc 0000cccd 0000ccce 0000cccf 0000ccd0
B[00033340] W[0000ccd0] D 052432: 0000ccd1 0000ccd2 0000ccd3 0000ccd4 0000ccd5 0000ccd6 0000ccd7 0000ccd8 0000ccd9 0000ccda 0000ccdb 0000ccdc 0000ccdd 0000ccde 0000ccdf 0000cce0
B[00033380] W[0000cce0] D 052448: 0000cce1 0000cce2 0000cce3 0000cce4 0000cce5 0000cce6 0000cce7 0000cce8 0000cce9 0000ccea 0000cceb 0000ccec 0000cced 0000ccee 0000ccef 0000ccf0
B[000333c0] W[0000ccf0] D 052464: 0000ccf1 0000ccf2 0000ccf3 0000ccf4 0000ccf5 0000ccf6 0000ccf7 0000ccf8 0000ccf9 0000ccfa 0000ccfb 0000ccfc 0000ccfd 0000ccfe 0000ccff 0000cd00
B[00033400] W[0000cd00] D 052480: 0000cd01 0000cd02 0000cd03 0000cd04 0000cd05 0000cd06 0000cd07 0000cd08 0000cd09 0000cd0a 0000cd0b 0000cd0c 0000cd0d 0000cd0e 0000cd0f 0000cd10
B[00033440] W[0000cd10] D 052496: 0000cd11 0000cd12 0000cd13 0000cd14 0000cd15 0000cd16 0000cd17 0000cd18 0000cd19 0000cd1a 0000cd1b 0000cd1c 0000cd1d 0000cd1e 0000cd1f 0000cd20
B[00033480] W[0000cd20] D 052512: 0000cd21 0000cd22 0000cd23 0000cd24 0000cd25 0000cd26 0000cd27 0000cd28 0000cd29 0000cd2a 0000cd2b 0000cd2c 0000cd2d 0000cd2e 0000cd2f 0000cd30
B[000334c0] W[0000cd30] D 052528: 0000cd31 0000cd32 0000cd33 0000cd34 0000cd35 0000cd36 0000cd37 0000cd38 0000cd39 0000cd3a 0000cd3b 0000cd3c 0000cd3d 0000cd3e 0000cd3f 0000cd40
B[00033500] W[0000cd40] D 052544: 0000cd41 0000cd42 0000cd43 0000cd44 0000cd45 0000cd46 0000cd47 0000cd48 0000cd49 0000cd4a 0000cd4b 0000cd4c 0000cd4d 0000cd4e 0000cd4f 0000cd50
...
B[000ffc00] W[0003ff00] D 261888: 0003ff01 0003ff02 0003ff03 0003ff04 0003ff05 0003ff06 0003ff07 0003ff08 0003ff09 0003ff0a 0003ff0b 0003ff0c 0003ff0d 0003ff0e 0003ff0f 0003ff10
B[000ffc40] W[0003ff10] D 261904: 0003ff11 0003ff12 0003ff13 0003ff14 0003ff15 0003ff16 0003ff17 0003ff18 0003ff19 0003ff1a 0003ff1b 0003ff1c 0003ff1d 0003ff1e 0003ff1f 0003ff20
B[000ffc80] W[0003ff20] D 261920: 0003ff21 0003ff22 0003ff23 0003ff24 0003ff25 0003ff26 0003ff27 0003ff28 0003ff29 0003ff2a 0003ff2b 0003ff2c 0003ff2d 0003ff2e 0003ff2f 0003ff30
B[000ffcc0] W[0003ff30] D 261936: 0003ff31 0003ff32 0003ff33 0003ff34 0003ff35 0003ff36 0003ff37 0003ff38 0003ff39 0003ff3a 0003ff3b 0003ff3c 0003ff3d 0003ff3e 0003ff3f 0003ff40
B[000ffd00] W[0003ff40] D 261952: 0003ff41 0003ff42 0003ff43 0003ff44 0003ff45 0003ff46 0003ff47 0003ff48 0003ff49 0003ff4a 0003ff4b 0003ff4c 0003ff4d 0003ff4e 0003ff4f 0003ff50
B[000ffd40] W[0003ff50] D 261968: 0003ff51 0003ff52 0003ff53 0003ff54 0003ff55 0003ff56 0003ff57 0003ff58 0003ff59 0003ff5a 0003ff5b 0003ff5c 0003ff5d 0003ff5e 0003ff5f 0003ff60
B[000ffd80] W[0003ff60] D 261984: 0003ff61 0003ff62 0003ff63 0003ff64 0003ff65 0003ff66 0003ff67 0003ff68 0003ff69 0003ff6a 0003ff6b 0003ff6c 0003ff6d 0003ff6e 0003ff6f 0003ff70
B[000ffdc0] W[0003ff70] D 262000: 0003ff71 0003ff72 0003ff73 0003ff74 0003ff75 0003ff76 0003ff77 0003ff78 0003ff79 0003ff7a 0003ff7b 0003ff7c 0003ff7d 0003ff7e 0003ff7f 0003ff80
B[000ffe00] W[0003ff80] D 262016: 0003ff81 0003ff82 0003ff83 0003ff84 0003ff85 0003ff86 0003ff87 0003ff88 0003ff89 0003ff8a 0003ff8b 0003ff8c 0003ff8d 0003ff8e 0003ff8f 0003ff90
B[000ffe40] W[0003ff90] D 262032: 0003ff91 0003ff92 0003ff93 0003ff94 0003ff95 0003ff96 0003ff97 0003ff98 0003ff99 0003ff9a 0003ff9b 0003ff9c 0003ff9d 0003ff9e 0003ff9f 0003ffa0
B[000ffe80] W[0003ffa0] D 262048: 0003ffa1 0003ffa2 0003ffa3 0003ffa4 0003ffa5 0003ffa6 0003ffa7 0003ffa8 0003ffa9 0003ffaa 0003ffab 0003ffac 0003ffad 0003ffae 0003ffaf 0003ffb0
B[000ffec0] W[0003ffb0] D 262064: 0003ffb1 0003ffb2 0003ffb3 0003ffb4 0003ffb5 0003ffb6 0003ffb7 0003ffb8 0003ffb9 0003ffba 0003ffbb 0003ffbc 0003ffbd 0003ffbe 0003ffbf 0003ffc0
B[000fff00] W[0003ffc0] D 262080: 0003ffc1 0003ffc2 0003ffc3 0003ffc4 0003ffc5 0003ffc6 0003ffc7 0003ffc8 0003ffc9 0003ffca 0003ffcb 0003ffcc 0003ffcd 0003ffce 0003ffcf 0003ffd0
B[000fff40] W[0003ffd0] D 262096: 0003ffd1 0003ffd2 0003ffd3 0003ffd4 0003ffd5 0003ffd6 0003ffd7 0003ffd8 0003ffd9 0003ffda 0003ffdb 0003ffdc 0003ffdd 0003ffde 0003ffdf 0003ffe0
B[000fff80] W[0003ffe0] D 262112: 0003ffe1 0003ffe2 0003ffe3 0003ffe4 0003ffe5 0003ffe6 0003ffe7 0003ffe8 0003ffe9 0003ffea 0003ffeb 0003ffec 0003ffed 0003ffee 0003ffef 0003fff0
B[000fffc0] W[0003fff0] D 262128: 0003fff1 0003fff2 0003fff3 0003fff4 0003fff5 0003fff6 0003fff7 0003fff8 0003fff9 0003fffa 0003fffb 0003fffc 0003fffd 0003fffe 0003ffff 00040000
END BLOCK
 */
