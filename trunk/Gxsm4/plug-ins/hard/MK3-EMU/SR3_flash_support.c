/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/* SRanger and Gxsm - Gnome X Scanning Microscopy Project
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 *
 * DSP tools for Linux
 *
 * Copyright (C) 1999,2000,2001,2002 Percy Zahl
 *
 * Authors: Percy Zahl <zahl@users.sf.net>
 * WWW Home:
 * DSP part:  http://sranger.sf.net
 * Gxsm part: http://gxsm.sf.net
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

/* code snippets from Alex, SoftdB 20140129 turned into function */

// Some defines:

#define Addr555				*( volatile short * ) 0x42000AAA //to obtain 0x0555 at the flash
#define Addr2AA				*( volatile short * ) 0x42000554 //to obtain 0x02AA at the flash
#define FB_INT6MASK			0x00000040
#define FlashSectorAddr 		0x420A0000 	// First free flash sector 0x420A0000-0x420BFFFE (128 kBytes)

// The first free flash sector is at 0x420A0000 and the sector size is 128 kbytes (0x420A0000 to 0x420BFFFE).
#define GXSM_FLASH_ADDRESS(OFFSET)      (FlashSectorAddr + (OFFSET))  // 128kB len sector

#ifdef DSPEMU
unsigned int ICR; // dummy
unsigned int IFR; // dummy

unsigned short dsp_emu_flash[65536];
int dsp_emu_flash_index_r=0;
int dsp_emu_flash_index_w=0;

#else
extern cregister volatile unsigned int ICR;
extern cregister volatile unsigned int IFR;
#endif

// local variables

unsigned short	*FB_ReadAddress  = (unsigned short *) FlashSectorAddr;
unsigned short	*FB_WriteAddress = (unsigned short *) FlashSectorAddr;


//  ************ Erase sector -- fixed sector 0x0030, 128kB len access only

#ifndef DSPEMU
#pragma CODE_SECTION(flash_erase, ".text:slow")
#endif

void flash_erase(){
#ifdef DSPEMU
        for (int i=0; i<65536; ++i)
                dsp_emu_flash[i++] = 0xFFFF;
        dsp_emu_flash_index_r=0;
        dsp_emu_flash_index_w=0;
#else
	ICR = FB_INT6MASK; 		// Clear the flag of the INT6 in IFR

	Addr555=0x00AA;			// 1 write cycle
	Addr2AA=0x0055;			// 2 write cycle
	Addr555=0x0080;			// 3 write cyle
	Addr555=0x00AA;			// 4 write cycle
	Addr2AA=0x0055;			// 5 write cycle
	
	*(volatile unsigned short *) FlashSectorAddr = 0x0030;		// 6 write cyle
	
	while ((IFR & FB_INT6MASK)==0); // Wait for INT6 interrupt --> Test the INT6 bit of IFR
#endif
}

// ************** Seek to offset within "this" sector for consecutive read/writes

#ifndef DSPEMU
#pragma CODE_SECTION(flash_seek, ".text:slow")
#endif

void flash_seek(unsigned short offset, unsigned short mode){
#ifdef DSPEMU
	switch (mode){
	case 'r': dsp_emu_flash_index_r = offset; break;
	case 'w': dsp_emu_flash_index_w = offset; break;
	}
#else
	switch (mode){
	case 'r': FB_ReadAddress  = (unsigned short *) GXSM_FLASH_ADDRESS (offset); break;
	case 'w': FB_WriteAddress = (unsigned short *) GXSM_FLASH_ADDRESS (offset); break;
	}
#endif
}

// ************* Read one 16-bit word
#ifndef DSPEMU
#pragma CODE_SECTION(flash_read16, ".text:slow")
#endif

unsigned short flash_read16(){
#ifdef DSPEMU
        return dsp_emu_flash[dsp_emu_flash_index_r++];
        dsp_emu_flash_index_r &= 0xffff;
#else
	return *(volatile unsigned short *) FB_ReadAddress++;
#endif
}

// ************* Write one 16-bit word
#ifndef DSPEMU
#pragma CODE_SECTION(flash_write16, ".text:slow")
#endif

void flash_write16(unsigned short data){
#ifdef DSPEMU
        dsp_emu_flash[dsp_emu_flash_index_w++] = data;
        dsp_emu_flash_index_w &= 0xffff;
#else

	ICR = FB_INT6MASK; 		// Clear the flag of the INT6 in IFR
	//	FB_WriteAddress=(unsigned short	*)FlashSectorAddr;
 
	Addr555=0x00AA;			// 1 write cycle
	Addr2AA=0x0055;			// 2 write cycle
	Addr555=0x00A0;			// 3 write cyle
	*(volatile unsigned short *) FB_WriteAddress = data;	// 4 write cycle
	
	while ((IFR & FB_INT6MASK)==0); // Wait for INT6 interrupt --> Test the INT6 bit of IFR
	
	FB_WriteAddress++; // Point to the next 16-bit word in the flash
#endif
}

#ifndef DSPEMU
#pragma CODE_SECTION(TestFlash_HL, ".text:slow")
#endif

void TestFlash_HL()
{
#ifdef DSPEMU
#else


	//  ************ Erase sector

	ICR = FB_INT6MASK; 		// Clear the flag of the INT6 in IFR

	Addr555=0x00AA;			// 1 write cycle
	Addr2AA=0x0055;			// 2 write cycle
	Addr555=0x0080;			// 3 write cyle
	Addr555=0x00AA;			// 4 write cycle
	Addr2AA=0x0055;			// 5 write cycle

	*(volatile unsigned short *) FlashSectorAddr=0x0030;		// 6 write cyle

	while ((IFR & FB_INT6MASK)==0); // Wait for INT6 interrupt --> Test the INT6 bit of IFR
	
	// ************* Write one 16-bit word

	ICR = FB_INT6MASK; 		// Clear the flag of the INT6 in IFR
	FB_WriteAddress=(unsigned short	*)FlashSectorAddr;

	Addr555=0x00AA;			// 1 write cycle
	Addr2AA=0x0055;			// 2 write cycle
	Addr555=0x00A0;			// 3 write cyle
	*(volatile unsigned short *) FB_WriteAddress=0x6518;	// 4 write cycle

	while ((IFR & FB_INT6MASK)==0); // Wait for INT6 interrupt --> Test the INT6 bit of IFR

	FB_WriteAddress++; // Point to the next 16-bit word in the flash
	
	// Acknowledge the PC
	
	// HPI_HPIC = Acknowledge;
#endif
}
