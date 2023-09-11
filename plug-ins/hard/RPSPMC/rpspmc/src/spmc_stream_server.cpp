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

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}


// stream server thread
void *thread_stream_server(void *arg) {
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

        char message[256];

        fprintf(stderr, "Starting thread: stream_server\n");

        
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

#if 0
        while (stream_server_control){
                fprintf(stderr, "Stream server: Test Loop\n");
                usleep (1000000);
        }
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
