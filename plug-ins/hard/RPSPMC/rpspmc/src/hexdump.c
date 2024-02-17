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

#define SPMC_DMA_BUFFER_BLOCK_SIZE         0x00080000 // Size of memory block per descriptor in bytes: 0.5 M -- two blocks, cyclic
#define SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE 0x1000 // 4k  **64 KiB 
#define SPMC_DMA_N_DESC                    2
#define SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH  0x1000 // 4k  **64 KiB 
#define SPMC_DMA_HP0_ADDRESS 		   0x01000000 // reserved cache-coherent memory region (linux kernel does not use this memory!)
#define SPMC_DMA_S2MM_TARGET_ADDRESS 	   (SPMC_DMA_HP0_ADDRESS + SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH)
#define SPMC_DMA_S2MM_BASE_DESC_ADDR  	   SPMC_DMA_HP0_ADDRESS

const char *FPGA_PACPLL_A9_name = "/dev/mem";

int main (int argc, char *argv[]){
        unsigned int* s2mm_descriptors;
        unsigned int* dest_memory;
        int count = SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC/4;
        if (argc > 1 && argv[1])
                count = atoi (argv[1]);

        int fd=0;
        if ((fd = open (FPGA_PACPLL_A9_name, O_RDWR|O_SYNC)) < 0) { // O_SYNC, O_DIRECT
                fprintf(stderr, "EE *** FPGA A9 /dev/mem FD OPEN FAILED ***\n");
                perror ("open");
                return -1;
        }
        printf ("\n**  FPGA A9 /dev/mem DMA MEMORY AREA DUMP **\n");

        printf ("\n** DESCRIPTORS FOR S2MM **\n");
        printf ("32bit word hexdump starting at 0x%08x:\n", SPMC_DMA_S2MM_TARGET_ADDRESS);
        s2mm_descriptors = (unsigned int*) mmap(NULL, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_S2MM_BASE_DESC_ADDR); //Formerly s2mm_descriptor_register_mmap
        memdump (s2mm_descriptors, 4*16, 0); //SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE/4);

        printf ("\n** TARGET MEM FOR S2MM **\n");
        printf ("BLOCK AB: 32bit word hexdump starting at 0x%08x:\n", SPMC_DMA_S2MM_TARGET_ADDRESS);
        dest_memory = (unsigned int*) mmap(NULL, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_S2MM_TARGET_ADDRESS);

        if (count >= (SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC/4)/2)
                memdump (dest_memory, count, 0);
        else if (count > 0 && count <= SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC){
                memdump (dest_memory, count, 0);
                printf("..\n");
                memdump (dest_memory+SPMC_DMA_BUFFER_BLOCK_SIZE/4-32, 32, SPMC_DMA_BUFFER_BLOCK_SIZE/4-32);

                printf ("\nBLOCK B: 32bit word hexdump starting at 0x%08x:\n", SPMC_DMA_S2MM_TARGET_ADDRESS+SPMC_DMA_BUFFER_BLOCK_SIZE);
                memdump (dest_memory+SPMC_DMA_BUFFER_BLOCK_SIZE/4, count, SPMC_DMA_BUFFER_BLOCK_SIZE/4);
                printf("..\n");
                memdump (dest_memory+2*SPMC_DMA_BUFFER_BLOCK_SIZE/4-32, 32, 2*SPMC_DMA_BUFFER_BLOCK_SIZE/4-32);
        } else
                printf ("invalid count");


        munmap (dest_memory, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC);
        munmap (s2mm_descriptors, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE);

        exit (0);
}
