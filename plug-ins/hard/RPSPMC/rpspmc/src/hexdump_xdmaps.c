/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/mman.h>
#include <fcntl.h>

void memdump (void* virtual_address, int int_count, int off){
        unsigned int *p = (unsigned int *) virtual_address;
        int offset,k;

        for(offset = 0; offset < int_count; offset+=16){
                printf ("B[%08x] W[%08x] D %06d:", (offset+off)*4, offset+off, offset+off);
                for(k = 0; k < 16; k++)
                        printf (" %08x", p[offset+k]);
                printf ("\n");
        }
}

#define XIL_XDMAPS_ADDRESS_NS  0xF8004000 // DMAC0_ns (non secure (boot) mode)
#define XIL_XDMAPS_ADDRESS_S   0xF8003000 // DMAC0_s  (secure (boot) mode)
#define XIL_XDMAPS_ADDRESS     XIL_XDMAPS_ADDRESS_NS
#define XIL_XDMAPS_SIZE        0x00001000

#define DUMP_SIZE        XIL_XDMAPS_SIZE
#define DUMP_ADDR        XIL_XDMAPS_ADDRESS_NS

const char *FPGA_PACPLL_A9_name = "/dev/mem";

int main (int argc, char *argv[]){
        unsigned int* memory;
        int ns = 0;
        if (argc > 1 && argv[1])
                ns = 0x1000*atoi (argv[1]);

        int fd=0;
        if ((fd = open (FPGA_PACPLL_A9_name, O_RDWR|O_SYNC)) < 0) { // O_SYNC, O_DIRECT
                fprintf(stderr, "EE *** FPGA A9 /dev/mem FD OPEN FAILED ***\n");
                perror ("open");
                return -1;
        }
        printf ("\n**  FPGA A9 /dev/mem DMA MEMORY AREA DUMP **\n");

        printf ("\n** Zynq MEM Dump **\n");
        printf ("32bit word hexdump starting at 0x%08x:\n", DUMP_ADDR+ns);

        memory = (unsigned int*) mmap(NULL, DUMP_SIZE, PROT_READ, MAP_SHARED, fd, DUMP_ADDR+ns);

        memdump (memory, DUMP_SIZE>>2, 0);

        munmap (memory, DUMP_SIZE);

        exit (0);
}
