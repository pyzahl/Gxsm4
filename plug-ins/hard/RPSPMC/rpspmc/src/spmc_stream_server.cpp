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
int stream_server_control = -1;
static pthread_t stream_server_thread;

extern void *FPGA_SPMC_bram;

#define BRAM_SIZE 16384  // (14 bit address)
#define BRAM_ADRESS_MASK 0x3fff;

inline unsigned int stream_lastwrite_address(){
        return read_gpio_reg_int32 (11,0) & BRAM_ADRESS_MASK;
}

// int32_t x32 = *((int32_t *)((uint8_t*)FPGA_SPMC_bram+i)); i+=4;


/**
 * The telemetry server accepts connections and sends a message every second to
 * each client containing an integer count. This example can be used as the
 * basis for programs that expose a stream of telemetry data for logging,
 * dashboards, etc.
 *
 * This example uses the timer based concurrency method and is self contained
 * and singled threaded. Refer to telemetry client for an example of a similar
 * telemetry setup using threads rather than timers.
 *
 * This example also includes an example simple HTTP server that serves a web
 * dashboard displaying the count. This simple design is suitable for use 
 * delivering a small number of files to a small number of clients. It is ideal
 * for cases like embedded dashboards that don't want the complexity of an extra
 * HTTP server to serve static files.
 *
 * This design *will* fall over under high traffic or DoS conditions. In such
 * cases you are much better off proxying to a real HTTP server for the http
 * requests.
 */

class telemetry_server {
public:
    typedef websocketpp::connection_hdl connection_hdl;
    typedef websocketpp::server<websocketpp::config::asio> server;

    telemetry_server() : m_count(0) {
        // set up access channels to only log interesting things
        m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
        m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
        m_endpoint.set_access_channels(websocketpp::log::alevel::app);

        // Initialize the Asio transport policy
        m_endpoint.init_asio();

        // Bind the handlers we are using
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::bind;
        m_endpoint.set_open_handler(bind(&telemetry_server::on_open,this,_1));
        m_endpoint.set_close_handler(bind(&telemetry_server::on_close,this,_1));
        m_endpoint.set_http_handler(bind(&telemetry_server::on_http,this,_1));
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

    void set_timer() {
        m_timer = m_endpoint.set_timer(
            1000,
            websocketpp::lib::bind(
                &telemetry_server::on_timer,
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

        double darray[32];
        for (int i=0; i<32; ++i)
                darray[i] = (double)(i-16)*m_count;
        
        val << "** Test Telemetry Server: count is " << m_count++ << " Last Write Address: " << stream_lastwrite_address() << " **\n";
        
        // Broadcast count to all connections
        con_list::iterator it;
        for (it = m_connections.begin(); it != m_connections.end(); ++it) {
            m_endpoint.send(*it,val.str(),websocketpp::frame::opcode::text);
            m_endpoint.send(*it, darray, 32*sizeof(double), websocketpp::frame::opcode::binary);
            m_endpoint.send(*it, FPGA_SPMC_bram, 512, websocketpp::frame::opcode::binary);
        }

        // set timer for next telemetry check
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
};



// stream server thread
void *thread_stream_server(void *arg) {
#ifdef PLAIN_SOCKET
        int fd, sock_server, sock_client;
        int position, limit, offset;
        volatile uint32_t *rx_addr, *rx_cntr;
        volatile uint16_t *rx_rate;
        volatile uint8_t *rx_rst;
        volatile void *cfg, *sts, *ram;
        cpu_set_t mask;
        struct sched_param param;
        struct sockaddr_in addr;
        uint32_t size;
        int yes = 1;
#endif
        
        fprintf(stderr, "Starting thread: Telemetry stream_server test\n");
        
        telemetry_server s;
        std::string docroot;
        uint16_t port = TCP_PORT;
        
        docroot = std::string("spmcstream");
        s.run(docroot, TCP_PORT);

#ifdef PLAIN_SOCKET
        char message[256];


        
        if ((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                fprintf(stderr, "E: socket init error\n");
                //perror("socket");
                return NULL;
        }

#if 0
        ram = mmap(NULL, 128*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

        rx_rst = (uint8_t *)(cfg + 0);
        rx_rate = (uint16_t *)(cfg + 2);
        rx_addr = (uint32_t *)(cfg + 4);
        
        rx_cntr = (uint32_t *)(sts + 0);
        
        *rx_addr = size;
#endif
  
        setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

        /* setup listening address */
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = htons(TCP_PORT);

        if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
                fprintf(stderr, "E: socket bind error\n");
                //perror("bind");
                return NULL;
        }

        listen(sock_server, 1024);

        while (!interrupted && stream_server_control){

                usleep(100); // init...

                fprintf(stderr, "Stream server listening for connection...\n");
                if((sock_client = accept(sock_server, NULL, NULL)) < 0) {
                        fprintf(stderr, "Stream server: Error accepting connection.\n");
                        //perror("accept");
                        return NULL;
                }

                signal(SIGINT, signal_handler);

                limit = 2*1024;

                int count=0;
                while (!interrupted && stream_server_control) {
#if 0
                        /* read ram writer position */
                        position = *rx_cntr;

                        /* send 256 kB if ready, otherwise sleep 0.1 ms */
                        if((limit > 0 && position > limit) || (limit == 0 && position < 2*1024)) {
                                offset = limit > 0 ? 0 : 256*1024;
                                limit = limit > 0 ? 0 : 2*1024;
                                if(send(sock_client, ram + offset, 256*1024, MSG_NOSIGNAL) < 0) break;
                        } else {
                                usleep(100);
                        }
#endif

                        sprintf (message,"** Test Message [%06d] STOP **\n", count++);
                        if(send(sock_client, message, strlen(message), MSG_NOSIGNAL) < 0) break;

                        fprintf(stderr, message);

                        usleep (1000000);
                }

                signal(SIGINT, SIG_DFL);
                close(sock_client);
        
                pthread_mutex_lock (&stream_server_mutexsum);
                // ...
                pthread_mutex_unlock (&stream_server_mutexsum);
        }

        // finish up/reset mode
        
        close (sock_server);
#endif
        
        fprintf(stderr, "Stream server exiting.\n");
        return NULL;
}


#if 0
int main ()
{
  int fd, sock_server, sock_client;
  int position, limit, offset;
  volatile uint32_t *rx_addr, *rx_cntr;
  volatile uint16_t *rx_rate;
  volatile uint8_t *rx_rst;
  volatile void *cfg, *sts, *ram;
  cpu_set_t mask;
  struct sched_param param;
  struct sockaddr_in addr;
  uint32_t size;
  int yes = 1;

  memset(&param, 0, sizeof(param));
  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &param);

  CPU_ZERO(&mask);
  CPU_SET(1, &mask);
  sched_setaffinity(0, sizeof(cpu_set_t), &mask);

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  size = 128*sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0)
  {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  ram = mmap(NULL, 128*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_rate = (uint16_t *)(cfg + 2);
  rx_addr = (uint32_t *)(cfg + 4);

  rx_cntr = (uint32_t *)(sts + 0);

  *rx_addr = size;

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

  /* setup listening address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(TCP_PORT);

  if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(!interrupted)
  {
    /* enter reset mode */
    *rx_rst &= ~1;
    usleep(100);
    *rx_rst &= ~2;
    /* set default sample rate */
    *rx_rate = 4;

    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *rx_rst |= 2;
    usleep(100);
    *rx_rst |= 1;

    limit = 2*1024;

    while(!interrupted)
    {
      /* read ram writer position */
      position = *rx_cntr;

      /* send 256 kB if ready, otherwise sleep 0.1 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 2*1024))
      {
        offset = limit > 0 ? 0 : 256*1024;
        limit = limit > 0 ? 0 : 2*1024;
        if(send(sock_client, ram + offset, 256*1024, MSG_NOSIGNAL) < 0) break;
      }
      else
      {
        usleep(100);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sock_client);
  }

  /* enter reset mode */
  *rx_rst &= ~1;
  usleep(100);
  *rx_rst &= ~2;

  close(sock_server);

  return EXIT_SUCCESS;
}
#endif
