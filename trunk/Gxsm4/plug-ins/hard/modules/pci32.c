/* Gxsm - Gnome X Scanning Microscopy
 * universal STM/AFM/SARLS/SPALEED/... controlling and
 * data analysis software
 * 
 * Copyright (C) 1999,2000,2001 Percy Zahl
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

/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */
/*
 * DSP Kernel Module
 * Master File for all kernel modules / device drivers
 * driver type is selected by -DCARD_PCI32, _PC31, _SIM !
 *
 * ##old##Device: /dev/pcdsp 
 * ##look in dsp.h: #define PCDSP_DEVICE "/dev/pcdsp" and ..NAME
 *
 * ##making device: mknod -m 666 /dev/pcdsp c xxx 0
 * ##for major "xxx" look in log/messages while loading module !
 * ##old##in most cases: xxx = 254
 * using devfs now!! => /dev/pcdsps/pci32|pc31 (major=240)
 */
#define PCDSP_VERSION "V0.2 (C) P.Zahl 1998,1999,2000,2001"
/*
 * Compile all versions with make
 *
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>

#include <linux/pci.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/devpts_fs.h>
#include <linux/file.h>
#include <linux/console.h>
#include <linux/timer.h>
#include <linux/ctype.h>
#include <linux/kd.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>

#ifdef __SMP__
#include <asm/pgtable.h>
#include <linux/smp_lock.h>
#endif

/* kernel_thread */
#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

#include "dbgstuff.h"
#include "dsp.h"

/* Ugly hack: the driver_name doesn't exist in 2.0.x . So we define it
   to the "name" field that does exist. As long as the assignments are
   done in the right order, there is nothing to worry about. */

#define driver_name           name 

/* Should be in a header somewhere. They are in tty.h on 2.2 */
#define TTY_HW_COOK_OUT       14 /* Flag to tell ntty what we can handle */
#define TTY_HW_COOK_IN        15 /* in hardware - output and input       */



/* Card special things */
#ifdef CARD_PCI32
 #include "pci32.h"
#else
 #ifdef CARD_PC31
  #include "pc31.h"
 #else
  #ifndef CARD_SIM
   #define CARD_SIM
  #endif
  #include "pcsim.h"
 #endif
#endif

/*
 * Driver Variables 
 */

static devfs_handle_t devfs_handle = NULL;

static int pcdsp_error;

static int opened;

static char *tmp_buf;
static int pcdsp_running=FALSE;

static unsigned long PutMem32Adr;

/* Hardware Location */
static int  pci_remapp_aktive;
static int  pcdsp_iobase;
static int  pcdsp_dpram;
static char *pcdsp_dprambaseptr;

/* Locations of SEM0..3 controlled Regions */
static unsigned long SemCrtlAdr[PCDSP_SEMANZ], SemCrtlLen[PCDSP_SEMANZ];

#include "mbox.h"

/* Process handling... */
static int wakeups = -1;
static DECLARE_WAIT_QUEUE_HEAD(waitq);

/*
 * Timings in jiffies,  1 jiffie dauert ca. 10ms (HZ = #jiffies fue 1sec)
 */

#define TIMEOUT_TICKS    19      /* 200ms */
#define JIFFIES_SEM      2      /* timeout after 50ms */
#define MAXWAKEUPS_SEM   10
#define MAXWAKEUPS_WMBOX 10
#define MAXWAKEUPS_RMBOX 10

/* Prototypes */

int SetSemCrtlAdr(int n, unsigned long arg);
int SetSemCrtlLen(int n, unsigned long arg);
int FindSem(unsigned long adr);
static void timeout(unsigned long ignore);
void mysleep(unsigned long myjiffies);

void pcdsp_interrupt_0(void);
void pcdsp_interrupt_1(void);
void free_dport_range(int semno);
int get_dport_range(int wait, int semno);
void pcdsp_halt(void);
void pcdsp_run(void);
void pcdsp_reset(void);

int BoxFull(int wait);
int BoxEmpty(int wait);
int ChkBoxEmpty(int wait);
int WriteBox(unsigned long data, int wait);
int ChkBoxFull(int wait);
int ReadBox(unsigned long *data, int wait);

/* Device Service Functions */
static loff_t pcdsp_seek (struct file *, loff_t, int);
static ssize_t pcdsp_read (struct file *, char *, size_t, loff_t *);
static ssize_t pcdsp_write (struct file *, const char *, size_t, loff_t *);
static int pcdsp_release (struct inode *, struct file *);
static int pcdsp_ioctl (struct inode *, struct file *, unsigned int, unsigned long);

#ifdef NO_INTERRUPT_SUPPORT_UNTIL_NOW
static void pcdsp_interrupt(int irq, void *dev_id, struct pt_regs *regs);
#endif
int pcdsp_initialize(void);
inline void pcdsp_quit(void);

int init_module(void);
void cleanup_module(void);

// Timeout Fkt.

static void timeout(unsigned long ignore){
  KDEBUG_L3("tmout wakeups %d\n", wakeups);
  wake_up_interruptible(&waitq);
}

#define MAKE_MY_TIMEOUT_TMR(tmr) \
  struct timer_list tmr; \
  init_timer(&tmr); \
  tmr.function = timeout; \
  tmr.data = 0

void mysleep(unsigned long myjiffies){
  MAKE_MY_TIMEOUT_TMR(timeout_tmr);

  del_timer(&timeout_tmr);
  timeout_tmr.expires = jiffies + myjiffies;
  add_timer(&timeout_tmr);
  interruptible_sleep_on(&waitq);
}


/* Semaphore Control */

int SetSemCrtlAdr(int n, unsigned long arg){
  if(arg > 0 && arg < PCDSP_DPRAM_SIZE && n >= 0 && n < PCDSP_SEMANZ){
    SemCrtlAdr[n] = arg;
    SemCrtlLen[n] = 0;
    return 0;
  }
  return -1;
}

int SetSemCrtlLen(int n, unsigned long arg){
  if(arg > 0 && (SemCrtlAdr[n]+arg) < PCDSP_DPRAM_SIZE && n >= 0 && n < PCDSP_SEMANZ){
    SemCrtlLen[n] = arg;
    return(0);
  }
  return(-1);
}

int inline FindSem(unsigned long adr){
#ifdef CARD_PCI32
  int i;
  for(i=0; i<PCDSP_SEMANZ; i++)
    if(adr >= SemCrtlAdr[i] && adr < (SemCrtlAdr[i]+SemCrtlLen[i]))
      return i;
#endif
#ifdef CARD_PC31
#endif
#ifdef CARD_SIM
#endif
  return 0;
}

/* Fire DSP INT0 */
void inline pcdsp_interrupt_0(void){
  CLR_IRQ0;
  SET_IRQ0;
  CLR_IRQ0;
}

/* Fire DSP INT1 */
void inline pcdsp_interrupt_1(void){
  CLR_IRQ1;
  SET_IRQ1;
  CLR_IRQ1;
}

/* Release DPRAM */
#define free_dport() free_dport_range(0)

void inline free_dport_range(int semno){
  FREE_SEM(semno);
}

/* Get DPRAM */
#define get_dport(W) get_dport_range(W, 0)

int get_dport_range(int wait, int semno){
    unsigned long timeoutpoint;
    MAKE_MY_TIMEOUT_TMR(timeout_tmr);

    KDEBUG_L3("get SEM%d\n", semno);
    GET_SEM(semno);
    wakeups=0;
    while(pcdsp_running && !SEM(semno) && wakeups < MAXWAKEUPS_SEM){ /* SEM DPRAM access OK ? */
	timeoutpoint = jiffies + JIFFIES_SEM;
	while(!SEM(semno) && timeoutpoint >= jiffies)
	    ;
	if(SEM(semno))
	    break;
	if(!wait){
	    free_dport_range(semno);
	    return FALSE;
	}
	del_timer(&timeout_tmr);
	timeout_tmr.expires = jiffies + TIMEOUT_TICKS;
	add_timer(&timeout_tmr);
	interruptible_sleep_on(&waitq);
	wakeups++;
    }
    KDEBUG_L3("wakeups for SEM%d: %d\n", semno, wakeups);
    if(wakeups == MAXWAKEUPS_SEM){
	free_dport_range(semno);
	return FALSE;
    }
    return TRUE;
}

/* Set Reset Bit: halts DSP */
void inline pcdsp_halt(void){
  PCDSP_HALT_X;
  pcdsp_running=FALSE;
  CLR_IRQ0;
  CLR_IRQ1;
  free_dport_range(0);
  free_dport_range(1);
  free_dport_range(2);
  free_dport_range(3);
  mysleep(50);
}

/* Clr Reset Bit: starts DSP */
void inline pcdsp_run(void){
  PCDSP_RUN_X;
  pcdsp_running=TRUE;
  mysleep(50);
}

/* Fire DSP Reset */
void inline pcdsp_reset(void){
  PCDSP_HALT_X;
  mysleep(50);
  PCDSP_RUN_X;
  mysleep(50);
  pcdsp_running=TRUE;
}

/*
 * Data Acknowledged by DSP / BoxFull ? 
 * Achtung: ist Box full, dann bleibt DPORT aktiv !
 */
int BoxFull(int wait){
    MAKE_MY_TIMEOUT_TMR(timeout_tmr);
    
    KDEBUG_L2("BoxFull ?\n");
    wakeups=0;
    if(get_dport(wait)){
	while(!(ACKD_DSP) && wakeups < MAXWAKEUPS_RMBOX){
	    free_dport();
	    if(!wait)
		return FALSE;
	    del_timer(&timeout_tmr);
	    timeout_tmr.expires = jiffies + TIMEOUT_TICKS;
	    add_timer(&timeout_tmr);
	    interruptible_sleep_on(&waitq);
	    wakeups++;
	    if(!get_dport(wait)){
		KDEBUG_L2("BoxFull !SEM\n");
		return FALSE;
	    }
	}
	KDEBUG_L2("BoxFull wakeups %d\n",wakeups);
	if(wakeups == MAXWAKEUPS_WMBOX){
	    free_dport();
	    KDEBUG_L2("BoxFull timeout\n");
	    return FALSE;
	}
	KDEBUG_L2("BoxFull OK\n");
	return TRUE;
    }
    KDEBUG_L2("BoxFull !SEM\n");
    return FALSE;
}

/*
 * Data received by DSP / BoxEmpty ?
 * Achtung: ist Box empty, dann bleibt DPORT aktiv !
 */
int BoxEmpty(int wait){
    MAKE_MY_TIMEOUT_TMR(timeout_tmr);
    KDEBUG_L2("BoxEmpty ?\n");
    wakeups=0;
    if(get_dport(wait)){
	while(REQD_DSP && wakeups < MAXWAKEUPS_WMBOX){
	    free_dport();
	    if(!wait)
		return FALSE;
	    del_timer(&timeout_tmr);
	    timeout_tmr.expires = jiffies + TIMEOUT_TICKS;
	    add_timer(&timeout_tmr);
	    interruptible_sleep_on(&waitq);
	    wakeups++;
	    if(!get_dport(wait)){
		KDEBUG_L2("BoxEmpty !SEM\n");
		return FALSE;
	    }
	}
	KDEBUG_L2("BoxEmpty wakeups %d\n",wakeups);
	if(wakeups == MAXWAKEUPS_WMBOX){
	    free_dport();
	    KDEBUG_L2("BoxEmpty timeout\n");
	    return FALSE;
	}
	KDEBUG_L2("BoxEmpty OK\n");
	return TRUE;
    }
    KDEBUG_L2("BoxEmpty !SEM\n");
    return FALSE;
}

/* 
 * ChkBoxEmpty:  wurde Mail von DSP abgeholt ?
 * WriteBox   :  REQD_DSP ?  ->  XMT_DSP(value), REQ_DSP  ->  [ REQD_DSP ? ] === BoxEmpty()
 *               warten auf Box leer und senden
 */

int ChkBoxEmpty(int wait){
  if(BoxEmpty(wait)){
    free_dport();
    return TRUE;
  }
  return FALSE;
}

int WriteBox(unsigned long data, int wait){
  if(BoxEmpty(wait)){
    KDEBUG_L3("WriteBox* %08x %08x %08x %08x\n",
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	    );
    XMT_DSP(data);
    REQ_DSP;
    KDEBUG_L3("WriteBox# %08x %08x %08x %08x\n",
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	    );
    free_dport();
    return TRUE;
  }
  return FALSE;
}

/*
 * ChkBoxFull:   Neue Mail von DSP empfangen ?
 * ReadBox:      ACKD_DSP ?  ->  value = RCV_DSP, ACK_DSP  ->  [ ACKD_DSP ? ] === BoxFull()
 *               warten auf Mail und lesen
 */

int ChkBoxFull(int wait){
  if(BoxFull(wait)){
    free_dport();
    return TRUE;
  }
  return FALSE;
}

int ReadBox(unsigned long *data, int wait){
  if(BoxFull(wait)){
    KDEBUG_L3("ReadBox* %08x %08x %08x %08x\n",
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	    );
    *data = RCV_DSP;
    ACK_DSP;
    KDEBUG_L3("ReadBox# %08x %08x %08x %08x\n",
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	    readl(pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	    );
    free_dport();
    return TRUE;
  }
  return FALSE;
}


/*
 * Driver FS-Functionality Implementation
 */
/* 
  defined in /usr/include/unistd.h, but trouble to include it here..., 
  so I define it myself here.
  If someone can fix this, I'll be happy!
*/

#define SEEK_SET        0       /* Seek from beginning of file.  */
#define SEEK_CUR        1       /* Seek from current position.  */
#define SEEK_END        2       /* Set file pointer to EOF plus "offset" */


static loff_t pcdsp_seek(struct file *f, loff_t offset, int orig)
{
  switch(orig){
  case SEEK_SET: f->f_pos  = offset; break;
  case SEEK_CUR: f->f_pos += offset; break;
  case SEEK_END: return EBADF;
  }
  if(   f->f_pos < 0 
     || f->f_pos >= PCDSP_DPRAM_SIZE)
    f->f_pos = 0;

  KDEBUG("seek to: %04x\n", (int)(f->f_pos));
  return f->f_pos;
}

/* Read Data from DSP */
static ssize_t pcdsp_read(struct file *f, char *buf, size_t count, loff_t *ppos)
{
  int semno;
  semno = FindSem(f->f_pos);

  if(get_dport_range(TRUE, semno)){
    KDEBUG_L2("%d bytes at %x\n",count, f->f_pos);
    tmp_buf=(char *)kmalloc(count, GFP_KERNEL );
    memcpy_fromio(tmp_buf, pcdsp_dprambaseptr + (unsigned int)f->f_pos, count);
    copy_to_user(buf, tmp_buf, count);
    kfree(tmp_buf);
    tmp_buf=NULL;
    free_dport();
    return 0;
  }
  return -EINVAL;
}

/* Write Data to DSP */
static ssize_t pcdsp_write(struct file *f, const char *buf, size_t count, loff_t *ppos)
{
  int semno;
  semno = FindSem(f->f_pos);

  if(get_dport_range(TRUE, semno)){
    KDEBUG_L2("write %d bytes at %x\n", count, f->f_pos);
    tmp_buf=(char *)kmalloc(count, GFP_KERNEL );
    copy_from_user(tmp_buf, buf, count);
    memcpy_toio(pcdsp_dprambaseptr + (unsigned int)f->f_pos, tmp_buf, count);
    kfree(tmp_buf);
    tmp_buf=NULL;
    free_dport();
    return 0;
  }
  return -EINVAL;
}

static int pcdsp_open(struct inode *inode, struct file *f){
  MOD_INC_USE_COUNT;
  opened++;  
  f->f_pos=0;
  KDEBUG("Opened %d times\n",opened);
  return 0;
}

static int pcdsp_release (struct inode *inode, struct file *f ){
  KDEBUG("release, still open:%d\n", opened-1);
  MOD_DEC_USE_COUNT;
  opened--;
  return 0;
}

static int pcdsp_ioctl(struct inode *inode, struct file *f, unsigned int cmd, unsigned long arg){
  KDEBUG("pcdsp_ioctl::%d\n", cmd);
  switch(cmd){
  case PCDSP_MBOX_FULL:
    return ChkBoxFull((int)arg);
  case PCDSP_MBOX_READ_WAIT: 
    ReadBox(&arg, TRUE);
    return (int)(arg&0x00ffffff);
  case PCDSP_MBOX_READ_NOWAIT: 
    ReadBox(&arg, FALSE);
    return (int)(arg&0x00ffffff);

  case PCDSP_MBOX_EMPTY:
    return ChkBoxEmpty((int)arg);
  case PCDSP_MBOX_WRITE_WAIT: 
    return WriteBox(arg, TRUE);
  case PCDSP_MBOX_WRITE_NOWAIT: 
    return WriteBox(arg, FALSE);

  case PCDSP_SET_MBOX_LOCATION: 
    if(arg >= 0 && arg <= (0x07ff-4))
      mbox_location = arg << 2;
    else
      KDEBUG("illegal Mailbox Location");
    return 0;
  case PCDSP_PUT_SPEED: 
    if(get_dport(TRUE)){
      writel(arg, pcdsp_dprambaseptr+mbox_location-4);
      free_dport();
      return 0;
    }
    return -EINVAL;
  case PCDSP_PUTMEM32: 
    if(get_dport(TRUE)){
      writel(arg, pcdsp_dprambaseptr+PutMem32Adr);
      free_dport();
      return 0;
    }
    return -EINVAL;
  case PCDSP_PUTMEM32INC: 
    if(get_dport(TRUE)){
      writel(arg, pcdsp_dprambaseptr+PutMem32Adr);
      free_dport();
      PutMem32Adr += 4;
      if(PutMem32Adr >= (2048*2))
	PutMem32Adr = 0;
      return 0;
    }
    return -EINVAL;
  case PCDSP_ADDRESS: 
    if(arg > (2048*2)){ KDEBUG("invalid address %x\n",(int)arg); return 0; }
    PutMem32Adr = arg;
    return 0;
  case PCDSP_RESET:
    pcdsp_reset();
    KDEBUG("reset\n");
    return 0;
  case PCDSP_HALT:   
    pcdsp_halt();
    KDEBUG("halt\n");
    return 0;
  case PCDSP_RUN:   
    pcdsp_run();
    KDEBUG("run\n");
    return 0;
  case PCDSP_SEM0START:
    return SetSemCrtlAdr(0, arg);
  case PCDSP_SEM0LEN:
    return SetSemCrtlLen(0, arg);
  case PCDSP_SEM1START:
    return SetSemCrtlAdr(1, arg);
  case PCDSP_SEM1LEN:
    return SetSemCrtlLen(1, arg);
  case PCDSP_SEM2START:
    return SetSemCrtlAdr(2, arg);
  case PCDSP_SEM2LEN:
    return SetSemCrtlLen(2, arg);
  case PCDSP_SEM3START:
    return SetSemCrtlAdr(3, arg);
  case PCDSP_SEM3LEN:
    return SetSemCrtlLen(3, arg);

#ifdef CARD_PC31
  case PCDSP_PC31SRQ:   PC31_SRQ((short)arg&1); return 1;
  case PCDSP_PC31SRQED: return(PC31_SRQED);
  case PCDSP_PC31ACKED: return(PC31_ACKED);
  case PCDSP_PC31ACK:   PC31_ACK; return 1;
  case PCDSP_PC31ADDR:
    outw_p((arg >> 16)&0xffff, DSP_ADR_H);
    outw_p( arg       &0xffff, DSP_ADR_L);
    return 1;
  case PCDSP_PC31STORE:
    outw_p((arg >> 16)&0xffff, DSP_DAT_H);
    outw_p( arg       &0xffff, DSP_DAT_L);
    return 1;
  case PCDSP_PC31FETCH:
    return(((unsigned long)inw_p(DSP_DAT_H) << 16) | (unsigned long)inw_p(DSP_DAT_L));
  case PCDSP_PC31DPENABLE:
    ENABLE_PC31_DPRAM;
  case PCDSP_PC31CONTROL:
    PC31_CONTROL((short)(arg&0xff));
    return 1;
#endif

  case PCDSP_GETMODID:
    return MODID;

  default: 
    KDEBUG_L3("MBox: %08x %08x %08x %08x\n",
	    (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	    (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	    (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	    (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	    );
    if(get_dport(TRUE)){
      KDEBUG("MBox:[%08X]: %08x %08x %08x %08x\n",
	     (int)(PCDSP_TALK_RCV),
	     (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_RCV),
	     (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_ACK),
	     (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_XMT),
	     (int)readl(arg+pcdsp_dprambaseptr+PCDSP_TALK_REQ)
	     );
      free_dport();
    }else{
      KDEBUG("MBox: no SEM acess\n");
    }
    /*
      KDEBUG("ioctl with unknown cmd\n",cmd,arg); 
     */
    return -1;
  }
  return 0;
}

#ifdef NO_INTERRUPT_SUPPORT_UNTIL_NOW
static void pcdsp_interrupt(int irq, void *dev_id, struct pt_regs *regs){  
  ;
}
#endif

/*
 * init some module variables
 */
int pcdsp_initialize(){
  PutMem32Adr=0L;
  pci_remapp_aktive=FALSE;
  pcdsp_iobase=0;
  pcdsp_dpram=0;
  pcdsp_dprambaseptr=NULL;
  opened=0;

  tmp_buf=NULL;

  // check Memory availability
  tmp_buf = (void *)kmalloc (10, GFP_KERNEL);
  if (tmp_buf == NULL){
    KDEBUG("Could not allocate Memory !\n");
    pcdsp_error = 2;
  }
  kfree(tmp_buf);
  tmp_buf=NULL;

  KDEBUG_L1("Driver %s\n", PCDSP_VERSION);
#ifdef CARD_PCI32
  if(pcibios_present()){
    unsigned char pci_irq_line;
    int pci_ioaddr, pci_memaddr;
    int pci_index;
    unsigned short pci_command, new_command;
    pci_irq_line = pci_ioaddr = pci_memaddr = 0;

    for(pci_index = 0; pci_index < 8; pci_index++){
      unsigned char pci_bus, pci_device_fn;
      unsigned int pci_ioaddr;
      if(pcibios_find_device(PCI_VENDOR_ID_INNOVATIVE, PCI_DEVICE_ID_PCI32DSP,
			     pci_index, &pci_bus, &pci_device_fn))
	break;
      pcibios_read_config_byte(pci_bus, pci_device_fn, PCI_INTERRUPT_LINE, &pci_irq_line);

      /* Reading PCI-Info */
      pcibios_read_config_dword(pci_bus, pci_device_fn, PCI_BASE_ADDRESS_2, &pci_ioaddr);
      pci_ioaddr &= PCI_BASE_ADDRESS_IO_MASK;
      pcibios_read_config_dword(pci_bus, pci_device_fn, PCI_BASE_ADDRESS_1, &pci_memaddr);
      pci_memaddr &= PCI_BASE_ADDRESS_IO_MASK;
      KDEBUG("PCI32 card #%d found at I/O:%x mem:%x IRQ %d\n", 
	     pci_index, pci_ioaddr, pci_memaddr, pci_irq_line);
      pcdsp_dpram  = pci_memaddr;
      pcdsp_iobase = pci_ioaddr;
      pcdsp_dprambaseptr = ioremap(pcdsp_dpram, PCDSP_DPRAM_SIZE);
      if(pcdsp_dprambaseptr){
	KDEBUG("ioremapping 0x%08x to 0x%08x\n", (int)pcdsp_dpram, (int)pcdsp_dprambaseptr);
	pci_remapp_aktive=TRUE;
      }else{
	KDEBUG("no ioremap, using 0x%08x\n", (int)pcdsp_dpram);
	pci_remapp_aktive=FALSE;
	pcdsp_dprambaseptr = (char*)pcdsp_dpram;
      }

      KDEBUG("checking PCI_COMMAND\n");

      pcibios_read_config_word(pci_bus, pci_device_fn, PCI_COMMAND, &pci_command);
      if(pci_command & PCI_COMMAND_MEMORY)
	KDEBUG("PCI_CMD_MEMORY\n");
      if(pci_command & PCI_COMMAND_IO)
	KDEBUG("PCI_CMD_IO\n");
      if(pci_command & PCI_COMMAND_MASTER)
	KDEBUG("PCI_CMD_MASTER\n");
      if(pci_command & PCI_COMMAND_INVALIDATE)
	KDEBUG("PCI_CMD_INVALIDATE\n");
      if(pci_command & PCI_COMMAND_WAIT)
	KDEBUG("PCI_CMD_WAIT\n");

      /* Activate the card. */
      //      new_command = pci_command | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;      
      new_command = PCI_COMMAND_IO | PCI_COMMAND_MEMORY;      
      //      new_command = pci_command | PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_INVALIDATE | PCI_COMMAND_WAIT;
      if (pci_command != new_command) {
	KDEBUG("The PCI BIOS has not enabled this"
	       " device!  Updating PCI command %4.4x->%4.4x.\n",
	       pci_command, new_command);
	pcibios_write_config_word(pci_bus, pci_device_fn, PCI_COMMAND, new_command);
	
      }

      pcdsp_halt();

      /* Setup default SEM - regions */
      SetSemCrtlAdr(0, PCDSP_MBOX);
      SetSemCrtlLen(0, PCDSP_MBOX_SIZE);
      SetSemCrtlAdr(1, PCDSP_DATA1);
      SetSemCrtlLen(1, PCDSP_DATA1_SIZE);
      SetSemCrtlAdr(2, PCDSP_DATA2);
      SetSemCrtlLen(2, PCDSP_DATA2_SIZE);
      SetSemCrtlAdr(3, PCDSP_DATA3);
      SetSemCrtlLen(3, PCDSP_DATA3_SIZE);

      KDEBUG_L1("Init PCDSP::CARD_PCI32 module OK.\n");
    }
    if(!pci_index){
      KDEBUG("no PCI32 card found this system\n");
      return -2;
    }
  }else{
    KDEBUG("no PCI support with this system\n");
    return -1;
  }
#endif
#ifdef CARD_PC31
  pcdsp_iobase = BASEPORT;
  pcdsp_dpram  = DPRAMBASE;
  pci_remapp_aktive=FALSE;
  pcdsp_dprambaseptr = (char*)pcdsp_dpram;
  ENABLE_PC31_DPRAM;
  KDEBUG("iobase, using port 0x%03x\n", pcdsp_iobase);
  KDEBUG("dprambase: 0x%08x\n", pcdsp_dpram);

  KDEBUG("Init PCDSP::CARD_PC31 module OK.\n");
#endif
#ifdef CARD_SIM
  KDEBUG_L1("Init PCDSP::CARD_SIM module OK.\n");
#endif

  return 0;
}

inline void pcdsp_quit(){
#ifdef CARD_PCI32
  if(pci_remapp_aktive)
    iounmap(pcdsp_dprambaseptr);

  KDEBUG("PCI32 Driver deinstalled\n");
#endif
#ifdef CARD_PC31
  KDEBUG("PC31 Driver deinstalled\n");
#endif
#ifdef CARD_SIM
  KDEBUG("Simulation Driver deinstalled\n");
#endif
}

/*
 * And now module code and kernel interface.
 */

struct file_operations pcdsp_fops = {
  llseek:    pcdsp_seek,   /* Set DPRAM R/W Position */
  read:    pcdsp_read,   /* Read Data to DPRAM */
  write:   pcdsp_write,  /* Write Data to DPRAM */
  //  NULL,         /* readdir */
  //  NULL,         /* poll */
  ioctl:   pcdsp_ioctl,  /* ioctl */
  //  NULL,         /* mmap */
  open:    pcdsp_open,   /* open */
  //  NULL,         /* flush */
  release: pcdsp_release,/* release */
  //  NULL,         /* fsync */
  //  NULL,         /* fasync */
  //  NULL,         /* check_media_change */
  //  NULL,         /* revalidate */
  //  NULL          /* lock */
};

int init_module(void){
    KDEBUG("init_module PCDSP\n");
    if ( devfs_register_chrdev (PCDSP_MAJOR, PCDSP_DEVICE_NAME, &pcdsp_fops) ){
	KDEBUG("unable create devfs entry for pcdsp device: %s, major: %d\n", PCDSP_DEVICE_NAME, PCDSP_MAJOR);
	return -EIO;
    }
    KDEBUG("Major number :%i \n", PCDSP_MAJOR);

    devfs_handle = devfs_mk_dir (NULL, PCDSP_DEVFS_DIR, NULL);

    devfs_register (devfs_handle, PCDSP_DEVICE_NAME,
                        DEVFS_FL_DEFAULT, PCDSP_MAJOR, 0,
                        S_IFCHR | S_IRUGO | S_IWUGO,
                        &pcdsp_fops, NULL);



    if(pcdsp_initialize())
	return -1; /* some error occured */

    // int request_irq(unsigned int irq,
    //              void (*handler)(int, void *, struct pt_regs *),
    //              unsigned long irqflags, const char *devname,
    //              void *dev_id);
    //       void free_irq(unsigned int irq, void *dev_id);
  
    return 0; /* success */
}

void cleanup_module(void){
    KDEBUG("Module PCDSP unregistered\n");
    
    pcdsp_quit();

    devfs_unregister (devfs_handle);
    devfs_unregister_chrdev(PCDSP_MAJOR, PCDSP_DEVICE_NAME);
    
    //  if(pcdsp_busy) ...
    //  release_region (0x280, 15);
    //  free_irq(pcdsp_interrupt,NULL);
    
    //  disable_irq(pcdsp_interrupt);
    KDEBUG("Module PCDSP unregistered.\n");
}


