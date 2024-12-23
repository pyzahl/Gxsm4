/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
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

#include<stdio.h>
#include<string.h> //strlen
#include<stdlib.h>
// #include<sys/socket.h> 
// #include<arpa/inet.h> // inet_addr 
#include<unistd.h> //write 
#include<pthread.h> //for threading, link with lpthread 
#include<sys/mman.h>
#include <fcntl.h>
// #include <arpa/inet.h> //https://linux.die.net/man/3/htonl
#include <time.h> 
#include<math.h> 
#include<errno.h> 

// #include "server_library.h" 

#include "virt_phys.cpp"

// -------------------------------------------------------------------------------------------------------
// FPGA Address Mappings
// -------------------------------------------------------------------------------------------------------
// memory dump 0x01000000 ... +0x80000*2+1000:  $$dd bs=1 skip=16777216 count=1049576 if=/dev/mem of=/tmp/dump1
/*

// PS -----------------------------------------------------------------------------------------------------  
// AXI_DMA:
// axi_dma_spmc/Data_MM2S:
axi_dma_spmc							
/PS/axi_dma_spmc/Data_SG (32 address bits : 4G)							
/PS/processing_system7_Zynq/S_AXI_HP0	S_AXI_HP0	HP0_DDR_LOWOCM	0x0100_0000	2M	0x11F_FFFF	0x0100_0000	0x011F_FFFF
/PS/processing_system7_Zynq/S_AXI_ACP	S_AXI_ACP	ACP_DDR_LOWOCM	0x0000_000	2M	0x1F_FFFF	0x0000_0000	0x001F_FFFF
/PS/processing_system7_Zynq/S_AXI_ACP	S_AXI_ACP	ACP_QSPI_LINEAR	0xFC00_0000	2M	0xFC1F_FFFF	0xFC00_0000	0xFC1F_FFFF
/PS/axi_dma_spmc/Data_S2MM (32 address bits : 4G)							
/PS/processing_system7_Zynq/S_AXI_HP0	S_AXI_HP0	HP0_DDR_LOWOCM	0x0100_0000	2M	0x11F_FFFF	0x0100_0000	0x011F_FFFF
/PS/processing_system7_Zynq/S_AXI_ACP	S_AXI_ACP	ACP_DDR_LOWOCM	0x0000_0000	2M	0x1F_FFFF	0x0000_0000	0x001F_FFFF
/PS/processing_system7_Zynq/S_AXI_ACP	S_AXI_ACP	ACP_QSPI_LINEAR	0xFC00_0000	2M	0xFC1F_FFFF	0xFC00_0000	0xFC1F_FFFF

// Zynq ---------------------------------------------------------------------------------------------------
/PS/axi_dma_spmc/S_AXI_LITE
/PS/axi_bram_reader_scope/s00_axi	S_AXI	        reg0	0x4001_0000	64K	0x4001_FFFF

// ps_0 Data:
/PS/axi_dma_spmc/S_AXI_LITE	        S_AXI_LITE	Reg	0x4040_0000	64K	0x4040_FFFF // *** == AXI_DMA_ADDRESS

/PS/Configurations/axi_cfg_register_0/s_axi	S_AXI	reg0	0x8000_0000	32K	0x8000_7FFF
/PS/Configurations/axi_cfg_register_1/s_axi	S_AXI	reg0	0x4000_0000	64K	0x4000_FFFF
/PS/GPIOs/axi_gpio_0/S_AXI	S_AXI	Reg	0x4120_0000	64K	0x4120_FFFF
/PS/GPIOs/axi_gpio_1/S_AXI	S_AXI	Reg	0x4121_0000	64K	0x4121_FFFF
/PS/GPIOs/axi_gpio_2/S_AXI	S_AXI	Reg	0x4123_0000	64K	0x4123_FFFF
/PS/GPIOs/axi_gpio_3/S_AXI	S_AXI	Reg	0x4124_0000	64K	0x4124_FFFF
/PS/GPIOs/axi_gpio_4/S_AXI	S_AXI	Reg	0x4125_0000	64K	0x4125_FFFF
/PS/GPIOs/axi_gpio_5/S_AXI	S_AXI	Reg	0x4126_0000	64K	0x4126_FFFF
/PS/GPIOs/axi_gpio_6/S_AXI	S_AXI	Reg	0x4127_0000	64K	0x4127_FFFF
/PS/GPIOs/axi_gpio_7/S_AXI	S_AXI	Reg	0x4128_0000	64K	0x4128_FFFF
/PS/GPIOs/axi_gpio_8/S_AXI	S_AXI	Reg	0x4129_0000	64K	0x4129_FFFF
/PS/GPIOs/axi_gpio_9/S_AXI	S_AXI	Reg	0x412A_0000	64K	0x412A_FFFF
/PS/GPIOs/axi_gpio_10/S_AXI	S_AXI	Reg	0x4122_0000	64K	0x4122_FFFF

SPMC DMA: ****************************** BD
SPMC DMA: BD_0 @0x05000000:
SPMC DMA: - NEXT_DESC 0x05000040
SPMC DMA: - BUFF_ADDR 0x05010000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_1 @0x05000040:
SPMC DMA: - NEXT_DESC 0x05000080
SPMC DMA: - BUFF_ADDR 0x05030000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_2 @0x05000080:
SPMC DMA: - NEXT_DESC 0x050000c0
SPMC DMA: - BUFF_ADDR 0x05050000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_3 @0x050000c0:
SPMC DMA: - NEXT_DESC 0x05000000
SPMC DMA: - BUFF_ADDR 0x05070000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: ******************************
SPMC DMA: S2MM Tail Descriptor Address 0x050000c0

============================
ZYNQ DMA CONTROLLER
UG585
https://docs.xilinx.com/r/en-US/ug585-zynq-7000-SoC-TRM/Register-XDMAPS_DA_0_OFFSET-Details
=============================
need access to current/last DMA write position "DA_0" Offset 0x404
Destination Addr DMA Channel 0 
Absolute Address  dmac0_ns: 0xF8004404
Absolute Address  dmac0_ns: 0xF8004408

XDMAPS_DA_0_OFFSET  0x00000404
XDMAPS_CC_0_OFFSET  0x00000408 
                   
*/

#define XIL_XDMAPS_ADDRESS_NS  0xF8004000 // DMAC0_ns (non secure (boot) mode)
#define XIL_XDMAPS_ADDRESS_S   0xF8003000 // DMAC0_s  (secure (boot) mode)
#define XIL_XDMAPS_ADDRESS     XIL_XDMAPS_ADDRESS_NS
#define XIL_XDMAPS_SIZE        0x00001000
#define XIL_XDMAPS_DA_0_OFFSET 0x00000404
#define XIL_XDMAPS_CC_0_OFFSET 0x00000408

// **** from xdmaps_hw.h
#define 	XDMAPS_DS_OFFSET   0x000 /* DMA Status Register */
#define 	XDMAPS_DPC_OFFSET   0x004 /* DMA Program Counter Rregister */
#define 	XDMAPS_INTEN_OFFSET   0X020 /* DMA Interrupt Enable Register */
#define 	XDMAPS_ES_OFFSET   0x024 /* DMA Event Status Register */
#define 	XDMAPS_INTSTATUS_OFFSET 0x00000028 
#define 	XDMAPS_INTCLR_OFFSET   0x02c /* DMA Interrupt Clear Register */
#define 	XDMAPS_FSM_OFFSET 0x00000030 
#define 	XDMAPS_FSC_OFFSET 0x00000034
#define 	XDMAPS_FTM_OFFSET   0x038 /* DMA Fault Type DMA Manager Register */
#define 	XDMAPS_FTC0_OFFSET   0x040 /* DMA Fault Type for DMA Channel 0 */
#define 	XDmaPs_FTCn_OFFSET(ch)   (XDMAPS_FTC0_OFFSET + (ch) * 4)
#define 	XDMAPS_CS0_OFFSET   0x100 /* Channel Status for DMA Channel 0 */
#define 	XDmaPs_CSn_OFFSET(ch)   (XDMAPS_CS0_OFFSET + (ch) * 8)
#define 	XDMAPS_CPC0_OFFSET 0x00000104 
#define 	XDmaPs_CPCn_OFFSET(ch)   (XDMAPS_CPC0_OFFSET + (ch) * 8)
#define 	XDMAPS_SA_0_OFFSET 0x00000400
#define 	XDmaPs_SA_n_OFFSET(ch)   (XDMAPS_SA_0_OFFSET + (ch) * 0x20)
#define 	XDMAPS_DA_0_OFFSET 0x00000404
#define 	XDmaPs_DA_n_OFFSET(ch)   (XDMAPS_DA_0_OFFSET + (ch) * 0x20)
#define 	XDMAPS_CC_0_OFFSET 0x00000408
#define 	XDmaPs_CC_n_OFFSET(ch)   (XDMAPS_CC_0_OFFSET + (ch) * 0x20)
#define 	XDMAPS_LC0_0_OFFSET   0x40C /* Loop Counter 0 for DMA Channel 0 */
#define 	XDmaPs_LC0_n_OFFSET(ch)   (XDMAPS_LC0_0_OFFSET + (ch) * 0x20)
#define 	XDMAPS_LC1_0_OFFSET   0x410 /* Loop Counter 1 for DMA Channel 0 */
#define 	XDmaPs_LC1_n_OFFSET(ch)   (XDMAPS_LC1_0_OFFSET + (ch) * 0x20)
#define 	XDMAPS_DBGSTATUS_OFFSET   0xD00 /* Debug Status Register */
#define 	XDMAPS_DBGCMD_OFFSET   0xD04 /* Debug Command Register */
#define 	XDMAPS_DBGINST0_OFFSET   0xD08 /* Debug Instruction 0 Register */
#define 	XDMAPS_DBGINST1_OFFSET   0xD0C /* Debug Instruction 1 Register */
#define 	XDMAPS_CR0_OFFSET   0xE00 /* Configuration Register 0 */
#define 	XDMAPS_CR1_OFFSET   0xE04 /* Configuration Register 1 */
#define 	XDMAPS_CR2_OFFSET   0xE08 /* Configuration Register 2 */
#define 	XDMAPS_CR3_OFFSET   0xE0C /* Configuration Register 3 */
#define 	XDMAPS_CR4_OFFSET   0xE10 /* Configuration Register 4 */
#define 	XDMAPS_CRDN_OFFSET   0xE14 /* Configuration Register Dn */
#define 	XDMAPS_DS_DMA_STATUS   0x0F /* DMA status mask */
#define 	XDMAPS_DS_DMA_STATUS_STOPPED   0x00 /* debug status busy mask */
#define 	XDMAPS_DBGSTATUS_BUSY   0x01 /* debug status busy mask */
#define 	XDMAPS_CR1_I_CACHE_LEN_MASK   0x07 /* i_cache_len mask */
#define 	XDmaPs_DBGINST0(b1, b0, ch, dbg_th)   (((b1) << 24) | ((b0) << 16) | (((ch) & 0x7) << 8) | ((dbg_th & 0x1)))


// 2MB length = 0x0020_0000  (2M)
// Descriptor = 0x0001_0000  (64k)

#define SPMC_DMA_AXI_DMA_ADDRESS	  0x40400000 
#define SPMC_DMA_AXI_DMA_ADDRESS	  0x40400000
#define SPMC_DMA_HP0_ADDRESS 		  0x01000000 // reserved cache-coherent memory region (linux kernel does not use this memory!)

#if 1 // OK ==============================
#define SPMC_DMA_N_DESC 2
#define SPMC_DMA_TRANSFER_BYTES 0x00080000 // 0.5M ****MAX WINDOW: 2M - DESCRIPTOR_BLOCK


#define SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE 0x1000 // 4k  **64 KiB 
#define SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH  0x1000 // 4k  **64 KiB 
#define SPMC_DMA_MEMBLOCK_SIZE             0x00200000 // Size of memory used by S2MM and MM2S (2 M total 0x0100_000 - 0x011F_FFFF)
#define SPMC_DMA_BUFFER_BLOCK_SIZE         0x00080000 // Size of memory block per descriptor in bytes: 0.5 M -- two blocks, cyclic

#else // FOR TESTING =====================

#define SPMC_DMA_N_DESC 1
#define SPMC_DMA_TRANSFER_BYTES 0x00100000 // 1M ****MAX WINDOW: 2M - DESCRIPTOR_BLOCK


#define SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE 0x1000 // 4k  **64 KiB 
#define SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH  0x1000 // 4k  **64 KiB 
#define SPMC_DMA_MEMBLOCK_SIZE             0x00200000 // Size of memory used by S2MM and MM2S (2 M total 0x0100_000 - 0x011F_FFFF)
#define SPMC_DMA_BUFFER_BLOCK_SIZE         0x00100000 // Size of memory block per descriptor in bytes: 0.5 M -- two blocks, cyclic
#endif

//#define SPMC_DMA_MM2S_BASE_DESC_ADDR	  SPMC_DMA_HP0_ADDRESS
#define SPMC_DMA_S2MM_BASE_DESC_ADDR	  SPMC_DMA_HP0_ADDRESS
//#define SPMC_DMA_MM2S_SOURCE_ADDRESS 	  (SPMC_DMA_HP0_ADDRESS + SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH + SPMC_DMA_N_DESC*SPMC_DMA_BUFFER_BLOCK_WIDTH)
#define SPMC_DMA_S2MM_TARGET_ADDRESS 	  (SPMC_DMA_HP0_ADDRESS + SPMC_DMA_SG_DMA_DESCRIPTORS_WIDTH)


#define SPMC_DMA_START_FRAME 	0x08000000 //TXSOF = 1 TXEOF = 0 
#define SPMC_DMA_END_FRAME 	0x04000000 //TXSOF = 0 TXEOF = 1
#define SPMC_DMA_COMPLETE_FRAME 0x0C000000 //TXSOF = TXEOF = 1
#define SPMC_DMA_MID_FRAME 	0x00000000 //TXSOF = TXEOF = 0 

//For DMACR (MM2S/S2MM Control register)
#define SPMC_DMA_RUN             0x0001 // RUN
#define SPMC_DMA_CYCLIC_ENABLE   0x0010 // BIT 4
#define SPMC_DMA_IOC_IRqEn       0x0800 //Interrupt on Complete Interrupt Enable 




// -------------------------------------------------------------------------------------------------------
// Offsets                          
// -------------------------------------------------------------------------------------------------------

// MM2S CONTROL
#define SPMC_DMA_MM2S_CONTROL_REGISTER       0x00    // MM2S_DMACR
#define SPMC_DMA_MM2S_STATUS_REGISTER        0x04    // MM2S_DMASR
#define SPMC_DMA_MM2S_CURDESC                0x08    // must align 0x40 addresses
#define SPMC_DMA_MM2S_CURDESC_MSB            0x0C    // unused with 32bit addresses
#define SPMC_DMA_MM2S_TAILDESC               0x10    // must align 0x40 addresses
#define SPMC_DMA_MM2S_TAILDESC_MSB           0x14    // unused with 32bit addresses

#define SPMC_DMA_SG_CTL                      0x2C    // CACHE CONTROL

// S2MM CONTROL
#define SPMC_DMA_S2MM_CONTROL_REGISTER       0x30    // S2MM_DMACR
#define SPMC_DMA_S2MM_STATUS_REGISTER        0x34    // S2MM_DMASR
#define SPMC_DMA_S2MM_CURDESC                0x38    // must align 0x40 addresses
#define SPMC_DMA_S2MM_CURDESC_MSB            0x3C    // unused with 32bit addresses
#define SPMC_DMA_S2MM_TAILDESC               0x40    // must align 0x40 addresses
#define SPMC_DMA_S2MM_TAILDESC_MSB           0x44    // unused with 32bit addresses
#define SPMC_DMA_S2MM_DESTINATION_ADDRESS    0x48    // 

//Scatter/Gather Control 
#define SPMC_DMA_NEXT_DESC 	0x00 //Set to address of next descriptor in chain 
#define SPMC_DMA_BUFF_ADDR 	0x08 //Set to address to read from (MM2S) or write to (S2MM) 
#define SPMC_DMA_CONTROL 	0x18 //Set transfer length, T/RXSOF and T/RXEOF 
#define SPMC_DMA_STATUS 	0x1C //Descriptor status

#define INFO_PRINTF(ARGS...)  if (verbose > 1) fprintf(stderr, "SPMC DMA: " ARGS)
#define X_PRINTF(ARGS...)  if (verbose > 1) fprintf(stderr, ARGS)

//root@rp-f09296:/opt/redpitaya/www/apps/rpspmc# cat /proc/2878/maps 
//b6f9e000-b6f9f000 rw-s 01000000 00:06 16         /dev/mem
//b6f9f000-b6fa0000 rw-s 40400000 00:06 16         /dev/mem
//b6fa0000-b6fa1000 rw-s 43000000 00:06 16         /dev/mem
//b6fa1000-b6fa2000 rw-s 42000000 00:06 16         /dev/mem

extern void *FPGA_PACPLL_bram;

class spmc_dma_support {
public:
        spmc_dma_support (int fd){ // int fd = open("/dev/mem",O_RDWR|O_SYNC);

                INFO_PRINTF ("spmc_dma_support class init\n");
                
                // Create 64K AXI DMA Memory Map at AXI_DMA_ADDRESS
                axi_dma = (unsigned int*) mmap(NULL, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_AXI_DMA_ADDRESS);
                if (axi_dma == MAP_FAILED){
                        INFO_PRINTF ("axi_dma mmap failed.\n");
                } else
                        fprintf(stderr, "RP FPGA RPSPMC AXI_DMA          mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  => *0x%08lx\n", (unsigned long)SPMC_DMA_AXI_DMA_ADDRESS, (unsigned long)(SPMC_DMA_AXI_DMA_ADDRESS + SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE-1), (unsigned long)(SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE), (unsigned long)axi_dma);
#if 0
                // Create 64k mm2s descriptors memory map at HP0_MM2S_DMA_DESCRIPTORS_ADDRESS
                mm2s_descriptors = (unsigned int*) mmap(NULL, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_MM2S_BASE_DESC_ADDR); //Formerly mm2s_descriptor_register_mmap
                if ( mm2s_descriptors == MAP_FAILED){
                        INFO_PRINTF (" mm2s_descriptors mmap failed.\n");
                } else
                        fprintf(stderr, "RP FPGA RPSPMC DMA MM2S_DESC    mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  => *0x%08lx\n", (unsigned long)SPMC_DMA_MM2S_BASE_DESC_ADDR, (unsigned long)(SPMC_DMA_MM2S_BASE_DESC_ADDR + SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE-1), (unsigned long)(SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE), (unsigned long)mm2s_descriptors);
#endif
                // Create 64k s2mm descriptors memory map at HP0_S2MM_DMA_DESCRIPTORS_ADDRESS
                s2mm_descriptors = (unsigned int*) mmap(NULL, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_S2MM_BASE_DESC_ADDR); //Formerly s2mm_descriptor_register_mmap
                if ( s2mm_descriptors == MAP_FAILED){
                        INFO_PRINTF (" s2mm_descriptors mmap failed.\n");
                } else
                        fprintf(stderr, "RP FPGA RPSPMC DMA S2MM_DESC    mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  => *0x%08lx\n", (unsigned long)SPMC_DMA_S2MM_BASE_DESC_ADDR, (unsigned long)(SPMC_DMA_S2MM_BASE_DESC_ADDR + SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE-1), (unsigned long)(SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE), (unsigned long)s2mm_descriptors);

#if 0
                // Create ~1Mb x Num_Descriptors source memory map
                source_memory = (unsigned int*) mmap(NULL, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_MM2S_SOURCE_ADDRESS); //Formerly source_mmap
                if (source_memory == MAP_FAILED){
                        INFO_PRINTF ("source_memory mmap failed.\n");
                } else
                        fprintf(stderr, "RP FPGA RPSPMC DMA SOURCE MEM   mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  => *0x%08lx\n", (unsigned long)SPMC_DMA_MM2S_SOURCE_ADDRESS, (unsigned long)(SPMC_DMA_MM2S_SOURCE_ADDRESS + SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC-1), (unsigned long)(SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC), (unsigned long)source_memory);
#endif
                // Create ~1Mb x Num_Descriptors target memory map
                dest_memory = (unsigned int*) mmap(NULL, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC, PROT_READ|PROT_WRITE, MAP_SHARED, fd, SPMC_DMA_S2MM_TARGET_ADDRESS); //Formerly dest_mmap 
                if (dest_memory == MAP_FAILED){
                        INFO_PRINTF ("dest_memory mmap failed.\n");
                } else {
                        uintptr_t paddr = 0;
                        uintptr_t vaddr = (uintptr_t) dest_memory;
                        int ret = virt_to_phys_user(&paddr, vaddr);

                        fprintf(stderr, "RP FPGA RPSPMC DMA DEST MEM     mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  PHYS ADDR[pid=%d, ret=%d]: Paddr: 0x%08ux for Vaddr: 0x%08lx  ##[0x%08lx]\n",
                                (unsigned long)SPMC_DMA_S2MM_TARGET_ADDRESS,
                                (unsigned long)(SPMC_DMA_S2MM_TARGET_ADDRESS + SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC-1),
                                (unsigned long)(SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC),
                                getpid(), ret, paddr,  (unsigned long)vaddr,  (unsigned long)SPMC_DMA_S2MM_TARGET_ADDRESS);
                }

                xil_xdmaps = (unsigned int*) mmap(NULL, XIL_XDMAPS_SIZE, PROT_READ, MAP_SHARED, fd, XIL_XDMAPS_ADDRESS); 
                if (xil_xdmaps == MAP_FAILED){
                        INFO_PRINTF ("XIL_XDMAPS mmap failed. (ZYNQ DMA PS MAPS)\n");
                } else {
                        uintptr_t paddr = 0;
                        uintptr_t vaddr = (uintptr_t) xil_xdmaps;
                        int ret = virt_to_phys_user(&paddr, vaddr);

                        fprintf(stderr, "XIL XDMAPS MEM     mapped 0x%08lx - 0x%08lx   block length: 0x%08lx  PHYS ADDR[pid=%d, ret=%d]: Paddr: 0x%08ux for Vaddr: 0x%08lx  ##[0x%08lx]\n",
                                (unsigned long)XIL_XDMAPS_ADDRESS,
                                (unsigned long)(XIL_XDMAPS_ADDRESS + XIL_XDMAPS_SIZE),
                                (unsigned long)(XIL_XDMAPS_SIZE),
                                getpid(), ret, paddr,  (unsigned long)vaddr,  (unsigned long)XIL_XDMAPS_ADDRESS);
                }
                 
                INFO_PRINTF ("spmc_dma_support class init -- mmaps completed.\n");
                // head -c 25165824 /dev/mem > /tmp/dump0
                // hexdump --skip 0x1000000 -v -e '"%08_ax: "' -e ' 16/4 "%08x_L[red:0] " " \n"' dump0 | less
        
                // Clear mm2s descriptors 
                //memset(mm2s_descriptors, 0, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE); 

                // Clear s2mm descriptors 
                memset(s2mm_descriptors, 0, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE); 

                // Clear Target Memory 
                memset(dest_memory, 0xdd, SPMC_DMA_N_DESC*SPMC_DMA_BUFFER_BLOCK_SIZE); 
                
                INFO_PRINTF ("spmc_dma_support class init -- clear mem completed.\n");

                // Reset and halt all DMA operations
                reset_all_dma ();

                INFO_PRINTF ("spmc_dma_support class init -- Reset DMA completed.\n");
        };
        
        ~spmc_dma_support (){
                reset_all_dma ();

                munmap (xil_xdmaps, XIL_XDMAPS_SIZE); 

                munmap (dest_memory, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC);
                //munmap (source_memory, SPMC_DMA_BUFFER_BLOCK_SIZE*SPMC_DMA_N_DESC);
                munmap (s2mm_descriptors, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE);
                //munmap (mm2s_descriptors, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE);
                munmap (axi_dma, SPMC_DMA_DESCRIPTOR_REGISTERS_SIZE);
                INFO_PRINTF ("spmc_dma_support class destructor -- reset DMA and mumap completed.\n");
        };

        void reset_all_dma (){
                // Reset and halt all DMA operations 
                halt_dma();
                reset_dma();

                //mm2s_status = get_offset (axi_dma, SPMC_DMA_MM2S_STATUS_REGISTER); 
                s2mm_status = get_offset (axi_dma, SPMC_DMA_S2MM_STATUS_REGISTER); 
                //print_status (mm2s_status, "MM2S"); 
                print_status (s2mm_status, "S2MM"); 
        };
        
        void setup_dma(){        
                /*********************************************************************/
                /*           build mm2s and s2mm Buffer Descriptor Chain             */
                /*********************************************************************/
                //Save the current/base descriptor address 
                s2mm_base_descriptor_address = SPMC_DMA_S2MM_BASE_DESC_ADDR; 
                INFO_PRINTF("S2MM Base Descriptor Address 0x%08x\n", s2mm_base_descriptor_address);	

                s2mm_tail_descriptor_address = build_descriptors(s2mm_descriptors, SPMC_DMA_S2MM_BASE_DESC_ADDR, SPMC_DMA_S2MM_TARGET_ADDRESS);
                INFO_PRINTF("S2MM Tail Descriptor Address 0x%08x\n", s2mm_tail_descriptor_address);	

                /*********************************************************************/
                /*                          start transfer                           */
                /*           and start dma operations (S2MM_DMACR.RS = 1)            */
                /*                 (by setting the tail descriptors)                 */
                /* -----------------------------------------------------------------
                  A DMA operation for the S2MM channel is set up and
                  started by using the following sequence:

                  1. Write the address of the starting descriptor to
                  the Current Descriptor register. If AXI DMA is
                  configured for an address space greater than 32,
                  then also program the MSB 32 bits of the current
                  descriptor.

                  2. Start the S2MM channel running by setting the
                  run/stop bit to 1 (S2MM_DMACR.RS =1). The halted bit
                  (DMASR.Halted) should deassert indicating the S2MM
                  channel is running.

                  3. If desired, enable interrupts by writing a 1 to
                  S2MM_DMACR.IOC_IrqEn and S2MM_DMACR.Err_IrqEn.

                  4. Write a valid address to the Tail Descriptor
                  register. If AXI DMA is configured for an address
                  space greater than 32, then also program the MSB 32
                  bits of the current descriptor.

                  5. Writing to the Tail Descriptor register triggers
                  the DMA to start fetching the descriptors from the
                  memory.

                  6. The fetched descriptors are processed and any
                  data received from the S2MM streaming channel is
                  written to the memory.  Cyclic DMA Mode

                  AXI DMA can be run in cyclic mode by making certain
                  changes to the Buffer Descriptor ( BD) chain
                  setup. In cyclic mode, DMA fetches and processes the
                  same BDs without interruption. The DMA continues to
                  fetch and process until it is stopped or reset.
                -------------------------------------------------------------- */
                /*********************************************************************/	

                /*********************************************************************/
                /*                 set current/base descriptor addresses             */
                /*********************************************************************/
                INFO_PRINTF ("SET CURRENT DESCRIPTOR ADDRESS\n");
                print_check_dma_all();

                // 1) SET POINTER TO INITIAl DESCRIPTOR ADDRESS
                set_offset (axi_dma, SPMC_DMA_S2MM_CURDESC, s2mm_base_descriptor_address); 
                print_check_dma_all();

                // 2) START RS=1
#if 0
                INFO_PRINTF ("SET START DMA IN CONTROL RS=1\n");
                set_offset (axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, SPMC_DMA_RUN); 
                print_check_dma_all();
#endif
                INFO_PRINTF ("SET START DMA IN CONTROL RS=1 + CYCLIC\n");
                set_offset (axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, SPMC_DMA_RUN | SPMC_DMA_CYCLIC_ENABLE); 
                print_check_dma_all();

                // 3) IRQ EN if desired
                INFO_PRINTF ("SET START DMA IN CONTROL RS=1 + CYCLIC + ~IRqEn -- ** no IRQ\n");
                //set_offset (axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, SPMC_DMA_RUN | SPMC_DMA_IOC_IRqEn); 
                print_check_dma_all();

                // 4...) Write Tail Descriptor register -- this starts DMA descriptor fetching

                /* Ensure that the cyclic bit in the control register is set.
                   Program the Tail Descriptor register with some value which is not a part of the BD chain. Say
                   for example 0x50.
                   After the Tail Descriptor register is programmed, the DMA starts fetching and processing
                   the BDs (which are set up in a ring fashion) until the DMA is stopped or reset.
                */
                INFO_PRINTF ("SET TAIL DESCRIPTOR ADDRESS (CYCLIC: any value not part of BD chain!)\n");
                //set_offset (axi_dma, SPMC_DMA_S2MM_TAILDESC, s2mm_tail_descriptor_address); 
                //##set_offset (axi_dma, SPMC_DMA_S2MM_TAILDESC, 0x50); // 0x50
                set_offset (axi_dma, SPMC_DMA_S2MM_TAILDESC, s2mm_tail_descriptor_address+0x50); 
                print_check_dma_all();
	};

        void clear_buffer (){
                // Clear Target Memory 
                //INFO_PRINTF ("CLEAR TARGET MEMORY with 0xDDDD DDDD\n");
                memset(dest_memory, 0xdd, SPMC_DMA_N_DESC*SPMC_DMA_BUFFER_BLOCK_SIZE); 
        };

        void start_dma(){
                //INFO_PRINTF ("RE-START DMA\n");
                INFO_PRINTF ("(RE)STARTING DMA: RESET DMA ** HALT ** INIT0xDD ** SETUP-DMA\n");

                // Full reset DMA
                reset_all_dma ();
                halt_dma (); // out of reset, halt

                // Clear Target Memory 
                //INFO_PRINTF ("CLEAR TARGET MEMORY with 0xDDDD DDDD\n");
                memset(dest_memory, 0xdd, SPMC_DMA_N_DESC*SPMC_DMA_BUFFER_BLOCK_SIZE); 
   
                //print_check_dma_all();

                //INFO_PRINTF ("SETUP DMA\n");
                // Build Descriptor Chain
                setup_dma();

                INFO_PRINTF ("DMA ACTIVE\n");
                print_check_dma_all();
        };

        unsigned int get_dmaps_DA0_offset() { return xil_xdmaps [XIL_XDMAPS_DA_0_OFFSET>>2]; };
        unsigned int get_dmaps_CC0_offset() { return xil_xdmaps [XIL_XDMAPS_CC_0_OFFSET>>2]; };

        unsigned int get_dmaps_DAn_offset(int ch) { return xil_xdmaps [(XDmaPs_DA_n_OFFSET(ch))>>2]; };
        unsigned int get_dmaps_CCn_offset(int ch) { return xil_xdmaps [(XDmaPs_CC_n_OFFSET(ch))>>2]; };

        
        unsigned int get_s2mm_status(){
                return get_offset (axi_dma, SPMC_DMA_S2MM_STATUS_REGISTER);
        };
        int get_s2mm_running(){
                return ! (get_s2mm_status() & 0x02);
        };
        unsigned int get_s2mm_nth_status(int i){
                return get_offset (s2mm_descriptors, SPMC_DMA_STATUS + i*0x40);
        };

        unsigned int get_s2mm_destination_address(){
                return get_offset (axi_dma, SPMC_DMA_S2MM_DESTINATION_ADDRESS);
        };
        
        void print_check_dma_all(){
                uint32_t S2MM_i_status; 	
                if(1){ //!((s2mm_status = get_s2mm_status())&0x2)){
                        for(int i = 0; i < SPMC_DMA_N_DESC; i++){ 
                                S2MM_i_status = get_s2mm_nth_status(i);
                                INFO_PRINTF("Descriptor %d Status: 0x%08x DA: ", i, S2MM_i_status);
                                for(int j = 0; j < 8; ++j) { X_PRINTF(" 0x%06x", get_dmaps_DAn_offset(j)); }
                                X_PRINTF("\n");
                        }
                }
                s2mm_status = get_s2mm_status();
                print_status(s2mm_status, "S2MM"); 
                //memdump(dest_memory, 0x80); //SPMC_DMA_TRANSFER_INTS);
        };
        
        void wait_test_dma(){
                int i = 0; 	
                uint32_t S2MM_i_status; 	
                while(!((s2mm_status = get_s2mm_status())&0x2)){
                        for(i = 0; i < SPMC_DMA_N_DESC; i++){ 
                                S2MM_i_status = get_s2mm_nth_status(i);
                                INFO_PRINTF("Descriptor %d Status: 0x%08x\n", i, S2MM_i_status);		
                        }
                }
                s2mm_status = get_s2mm_status();
                print_status(s2mm_status, "S2MM"); 

                memdump(dest_memory, 0x1000); //SPMC_DMA_TRANSFER_INTS);

                //halt_dma();
                //reset_dma();
        };

        unsigned int* dma_memdest(){
                return dest_memory;
        };
        
        void halt_dma(){
                //Halt DMA 
                set_offset(axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, 0x0); // Bit0 <= 0 ==> STOP DMA
                set_offset(axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, 0x0);
                usleep (10000);
                s2mm_status = get_offset(axi_dma, SPMC_DMA_S2MM_STATUS_REGISTER); 
                print_status(s2mm_status,"S2MM");
        };

        void reset_dma(){
                //Reset DMA 
                set_offset(axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, 0x4); // Bit2 <= 1 (=4) ==> PUT DMA IN RESET
                set_offset(axi_dma, SPMC_DMA_S2MM_CONTROL_REGISTER, 0x4); 
                usleep (10000);
                s2mm_status = get_offset(axi_dma, SPMC_DMA_S2MM_STATUS_REGISTER); 
                print_status(s2mm_status,"S2MM");   
        };
        
        
        void set_offset(unsigned int* virtual_address, int offset, unsigned int value){
                virtual_address[offset>>2] = value;
        };

        unsigned int get_offset(unsigned int* virtual_address, int offset) {    
                return virtual_address[offset>>2];
        };

        void print_offset(unsigned int* virtual_address, int offset){    
                unsigned int value = get_offset(virtual_address,offset); 
                INFO_PRINTF("0x%08x\n",value); 
        };

#if 0
        void memsend(void* virtual_address, int int_count, int socket_desc)
        {
                unsigned int *p = virtual_address;
                int offset; 
                for(offset = 0; offset<int_count;offset++){
                        // INFO_PRINTF("0x%08x\n",p[offset]); 
                        write(socket_desc,&p[offset],sizeof(p[offset])); 
                }
        };
#endif
        
        void memdump(void* virtual_address, int int_count)
        {    
                unsigned int *p = (unsigned int *) virtual_address;
                int offset,k;
                for(offset = 0; offset < int_count; offset+=16){
                        X_PRINTF("[%08x] %04d:", offset, offset);
                        for(k = 0; k < 16; k++) { X_PRINTF(" %08x", p[offset+k]); }
                        X_PRINTF("\n");
                }
        };

        void memdump_from(void* virtual_address, int int_count, int from)
        {    
                unsigned int *p = (unsigned int *) virtual_address;
                int offset,k;
                int_count += from;
                for(offset = from; offset < int_count; offset+=16){
                        X_PRINTF("[%08x] %04d:", offset, offset);
                        for(k = 0; k < 16; k++) { X_PRINTF(" %08x", p[offset+k]); }
                        X_PRINTF("\n");
                }
        };

        void print_status(uint32_t status, const char *type){	
                if(!strcmp(type,"S2MM")){
                        INFO_PRINTF("S2MM status 0x%08x @0x%08x [%s %s %s %s %s %s %s %s %s %s %s %s]\n",
                                    status, SPMC_DMA_AXI_DMA_ADDRESS + SPMC_DMA_S2MM_STATUS_REGISTER,
                                    status & 0x00000001? "Halted" : "Running",
                                    status & 0x00000002? "Idle":"",
                                    status & 0x00000008? "SGIncld":"",
                                    status & 0x00000010? "DMAIntErr":"",
                                    status & 0x00000020? "DMASlvErr":"",
                                    status & 0x00000040? "DMADecErr":"",
                                    status & 0x00000100? "SGIntErr":"",
                                    status & 0x00000200? "SGSlvErr":"",
                                    status & 0x00000400? "SGDecErr":"",
                                    status & 0x00001000? "IOC_Irq":"",
                                    status & 0x00002000? "Dly_Irq":"",
                                    status & 0x00004000? "Err_Irq":""
                                    ); 		
                }
                if(!strcmp(type,"MM2S")){
                        INFO_PRINTF("MM2S status 0x%08x @0x%08x [%s %s %s %s %s %s %s %s %s %s %s %s]\n",
                                    status, SPMC_DMA_AXI_DMA_ADDRESS + SPMC_DMA_MM2S_STATUS_REGISTER,
                                    status & 0x00000001? "Halted" : "Running",
                                    status & 0x00000002? "Idle":"",
                                    status & 0x00000008? "SGIncld":"",
                                    status & 0x00000010? "DMAIntErr":"",
                                    status & 0x00000020? "DMASlvErr":"",
                                    status & 0x00000040? "DMADecErr":"",
                                    status & 0x00000100? "SGIntErr":"",
                                    status & 0x00000200? "SGSlvErr":"",
                                    status & 0x00000400? "SGDecErr":"",
                                    status & 0x00001000? "IOC_Irq":"",
                                    status & 0x00002000? "Dly_Irq":"",
                                    status & 0x00004000? "Err_Irq":""
                                    ); 		
                }
        };

        uint32_t build_descriptors(unsigned int* descriptor_chain, uint32_t BASE_DESC_ADDR, uint32_t TARGET_ADDRESS){		
                INFO_PRINTF("****************************** BD\n");
                int i; 	
                for (i = 0; i <= SPMC_DMA_N_DESC; i++){
                        // *********************************************************************
                        // Next Descriptor Pointer: 0x00 (Set to location of next descriptor) 
                        // Buffer Address: 0x08 (Set to location to read from (MM2S) or write data (S2MM) to)
                        // Control: 0x18 (Specify TXSOF(b27),TXEOF(b26), and transfer length(b22:b0) )
                        // *********************************************************************
                        //The ith descriptor, BD_i, in the chain, is located at (BASE_DESC_ADDR + i*0x40).
                        //If the current descriptor, BD_i, is not the last in the chain
                      
                        if (i != SPMC_DMA_N_DESC){ 
                                //Then it must point to the next descriptor in the chain, BD_{i+1}, 
                                //located at (BASE_DESC_ADDR+(i+1)*0x40)
                                set_offset (descriptor_chain, SPMC_DMA_NEXT_DESC + i*0x40, BASE_DESC_ADDR+(i+1)*0x40); 					
                                //BD_i reads from/writes to TARGET_ADDRESS + (i*TRANSFER_BYTES) 
                                set_offset (descriptor_chain, SPMC_DMA_BUFF_ADDR + i*0x40, TARGET_ADDRESS+i*SPMC_DMA_TRANSFER_BYTES); 		
                                //BD_i performs a transfer of TRANSFER_BYTES to/from the AXI stream 
                                //from/to the location TARGET_ADDRESS+i*TRANSFER_BYTES
                                set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, SPMC_DMA_TRANSFER_BYTES);
                                INFO_PRINTF("BD_%d @0x%08x:\n", i, BASE_DESC_ADDR+i*0x40);
                        } 
                        //Otherwise, i.e. if BD_i is the tail descriptor in the chain of cyclic mode, make point to first
                        else{ 
                                //Then it must point to the first descriptor in the chain, BD_0, 
                                //located at (BASE_DESC_ADDR+0*0x40)
                                //At least in cyclic mode, which is what we are using for the moment. 
                                set_offset (descriptor_chain, SPMC_DMA_NEXT_DESC + i*0x40, BASE_DESC_ADDR); //+0*0x40);
                                // no other function (remains zero)
                                set_offset (descriptor_chain, SPMC_DMA_BUFF_ADDR + i*0x40, TARGET_ADDRESS+i*SPMC_DMA_TRANSFER_BYTES); // taildescr, not used data, but non zero		
                                //set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, SPMC_DMA_TRANSFER_BYTES);
                                //set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, 0x00); // not cycling
                                //set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, 0x04); // one missing word 
                                set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, 0x04); //
                                //##set_offset (descriptor_chain, SPMC_DMA_CONTROL + i*0x40, 0x50); // taildescr, not used data, but non zero
                                INFO_PRINTF ("CYCLIC TAIL BD_%d @0x%08x:\n", i, BASE_DESC_ADDR+i*0x40);
                        }

                        INFO_PRINTF ("- NEXT_DESC 0x%08x\n", get_offset(descriptor_chain, SPMC_DMA_NEXT_DESC+i*0x40)); 
                        INFO_PRINTF ("- BUFF_ADDR 0x%08x\n", get_offset(descriptor_chain, SPMC_DMA_BUFF_ADDR+i*0x40)); 
                        INFO_PRINTF ("- CONTROL   0x%08x\n", get_offset(descriptor_chain, SPMC_DMA_CONTROL+i*0x40)); 
                }
                INFO_PRINTF ("******************************\n");
                //Return the address of the tail descriptor, located at 
                return BASE_DESC_ADDR+(SPMC_DMA_N_DESC)*0x40; //-- for cyclic mode, programm any address NOT part of chain!
        };

private:
        int mm2s_status, s2mm_status; 			
        //uint32_t mm2s_base_descriptor_address; //Formerly mm2s_current_descriptor_address
        uint32_t s2mm_base_descriptor_address; //Formerly s2mm_current_descriptor_address
        //uint32_t mm2s_tail_descriptor_address;
        uint32_t s2mm_tail_descriptor_address;

        unsigned int* axi_dma;
        //unsigned int* mm2s_descriptors;
        unsigned int* s2mm_descriptors;
        //unsigned int* source_memory;
        unsigned int* dest_memory;

        unsigned int* xil_xdmaps;
};

/*

SPMC DMA: RESET, SETUP and START DMA
SPMC DMA: MM2S status 0x00010009 @0x40400004 [Halted  SGIncld         ]
SPMC DMA: S2MM status 0x00010009 @0x40400034 [Halted  SGIncld         ]
SPMC DMA: S2MM Base Descriptor Address 0x05000000
SPMC DMA: ****************************** BD
SPMC DMA: BD_0 @0x05000000:
SPMC DMA: - NEXT_DESC 0x05000040
SPMC DMA: - BUFF_ADDR 0x05010000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_1 @0x05000040:
SPMC DMA: - NEXT_DESC 0x05000080
SPMC DMA: - BUFF_ADDR 0x05030000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_2 @0x05000080:
SPMC DMA: - NEXT_DESC 0x050000c0
SPMC DMA: - BUFF_ADDR 0x05050000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: BD_3 @0x050000c0:
SPMC DMA: - NEXT_DESC 0x05000000
SPMC DMA: - BUFF_ADDR 0x05070000
SPMC DMA: - CONTROL   0x0c020000
SPMC DMA: ******************************
SPMC DMA: S2MM Tail Descriptor Address 0x050000c0
SPMC DMA: * START DMA *
SPMC DMA: Descriptor 0 Status: 0x00000000
SPMC DMA: Descriptor 1 Status: 0x00000000
SPMC DMA: Descriptor 2 Status: 0x00000000
SPMC DMA: Descriptor 3 Status: 0x00000000
SPMC DMA: S2MM status 0x00010009 @0x40400034 [Halted  SGIncld         ]

*/
