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
 * Device: /dev/pcdsp 
 * look in dsp.h: #define PCDSP_DEVICE "/dev/pcdsp" and ..NAME
 *
 * making device: mknod -m 666 /dev/pcdsp c xxx 0
 * for major "xxx" look in log/messages while loading module !
 * in most cases: xxx = 254
 */

#define PCDSP_VERSION "V0.1 (C) P.Zahl 2000"

/*
 * Compile all versions with make
 *
 */

// is defined in Makefile via gcc -D option:
//#define EMULATE_A_SPALEED
//#define EMULATE_DSP_BB_SPALEED


#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>

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
#include <linux/malloc.h>
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

// Dbg Macros
#include "dbgstuff.h"
#include "pcsim.h"

/*
 * Driver Variables 
 */

static int pcdsp_major;

static int opened;

/* Hardware Location */
static char *pcdsp_dprambaseptr;

/* Process handling... */
static int wakeups = -1;
static struct wait_queue *waitq = NULL;

/* Thread Data Structure */
struct dspsim_thread_data {
    /* thread */
    struct task_struct  *thread;
    struct wait_queue   *wq;
    int                  active, rmmod;

    char *virtual_dpram;
    int  SrvReqAck;
    int  ReqDAck;
};

struct dspsim_thread_data dspsim_td;

#define ACKD_DSP dspsim_td.SrvReqAck
#define REQD_DSP dspsim_td.ReqDAck


/*
 * Timings in jiffies,  1 jiffie dauert ca. 10ms (HZ = #jiffies fue 1sec)
 */

#define TIMEOUT_TICKS    19      /* 200ms */
#define JIFFIES_SEM      2      /* timeout after 50ms */
#define MAXWAKEUPS_SEM   10
#define MAXWAKEUPS_WMBOX 10
#define MAXWAKEUPS_RMBOX 10

/* Prototypes */

static void timeout(unsigned long ignore);
void mysleep(unsigned long myjiffies);

int  InitEmu(struct dspsim_thread_data *dsp);
void ExitEmu(void);
void ServiceRequest(struct dspsim_thread_data *dsp);

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


int pcdsp_initialize(void);
inline void pcdsp_quit(void);

int init_module(void);
void cleanup_module(void);

/*
 * DSP Simulation -- Emulation of DSP Program
 */

static int dspsim_thread(void *data);
void start_dsp(struct dspsim_thread_data *dsp);
void stop_dsp(struct dspsim_thread_data *dsp);

#ifdef EMULATE_A_SPALEED
# include "spaleed_emu.c"
#else
# ifdef EMULATE_DSP_BB_SPALEED
#  include "spaleed_bb.c"
# else

// EmuDummy
int  InitEmu(struct dspsim_thread_data *dsp){ return 0; }
void ExitEmu(){}
void ServiceRequest(struct dspsim_thread_data *dsp){
    switch(((unsigned char) *dsp->virtual_dpram) & 0xff){
    case DSP_CMD_xxxxx: 
      ACKD_DSP = TRUE;
      break;
    default: 
      ACKD_DSP = TRUE;
      break;
    }
    REQD_DSP   = FALSE;
}

# endif
#endif

/*
 * A kernel thread for simulation a missing DSP (TMS320) card
 * -- we don't want to block the in the ioctl while doing the 
 * data aquisitaion
 */

static int dspsim_thread(void *data)
{
    struct dspsim_thread_data *dsp = data;
    
#ifdef __SMP__
    lock_kernel();
#endif

    exit_mm(current);
    current->session = 1;
    current->pgrp = 1;
    sigfillset(&current->blocked);
    current->fs->umask = 0;
    strcpy(current->comm,"dspemu");

    dsp->thread = current;
    dsp->active = -1;

#ifdef __SMP__
    unlock_kernel();
#endif

    KDEBUG("DSP Simulating daemon: started\n");
                
    for (;;) {
	dsp->active = 0;
	interruptible_sleep_on(&dsp->wq);
	dsp->active = 1;
	KDEBUG_L3("DSP Simulating daemon: wakeup\n");
	if (dsp->rmmod)
	    goto done;

	ServiceRequest(dsp);
    }

done:
    KDEBUG("DSP Simulating thread: exit\n");
    dsp->active = -1;
    dsp->thread = NULL;

    return 0;
}

void start_dsp(struct dspsim_thread_data *dsp){
    /* startup control thread */
    dsp->wq     = NULL;
    dsp->thread = NULL;
    dsp->rmmod  = 0;
    kernel_thread(dspsim_thread, (void *)dsp, 0);
    //    wake_up_interruptible(&dsp->wq);
}

void stop_dsp(struct dspsim_thread_data *dsp){
    /* shutdown control thread */
    if (dsp->thread) 
        {
	    // wait while thread is busy
	    while( dsp->active )
		mysleep(100);
	    dsp->rmmod = 1;
	    wake_up_interruptible(&dsp->wq);
        }
    // wait until thread has finished
    KDEBUG("waiting until thread has finished...\n");
    while(dsp->thread){
	mysleep(5);
	if(dsp->thread)
	    KDEBUG("still waiting\n");
    }
}





static void timeout(unsigned long ignore){
  KDEBUG_L3("tmout wakeups %d\n", wakeups);
  wake_up_interruptible(&waitq);
}

void mysleep(unsigned long myjiffies){
  static struct timer_list timeout_tmr = { NULL, NULL, 0, 0, timeout };
  del_timer(&timeout_tmr);
  timeout_tmr.expires = jiffies + myjiffies;
  add_timer(&timeout_tmr);
  interruptible_sleep_on(&waitq);
}

/*
 * DSP Simulation -- Emulation of DSP, DPRAM, MAILBOX
 */

int BoxFull(int wait){
  static struct timer_list timeout_tmr = { NULL, NULL, 0, 0, timeout };
  KDEBUG_L2("BoxFull ?\n");
  wakeups=0;
  while(!(ACKD_DSP) && wakeups < MAXWAKEUPS_RMBOX){
      if(!wait)
	  return FALSE;
      del_timer(&timeout_tmr);
      timeout_tmr.expires = jiffies + TIMEOUT_TICKS;
      add_timer(&timeout_tmr);
      interruptible_sleep_on(&waitq);
      wakeups++;
  }
  KDEBUG_L2("BoxFull wakeups %d\n",wakeups);
  if(wakeups == MAXWAKEUPS_WMBOX){
      KDEBUG_L2("BoxFull timeout\n");
      return FALSE;
  }
  KDEBUG_L2("BoxFull OK\n");
  return TRUE;
}

int BoxEmpty(int wait){
  static struct timer_list timeout_tmr = { NULL, NULL, 0, 0, timeout };
  KDEBUG_L2("BoxEmpty ?\n");
  wakeups=0;
  while(REQD_DSP && wakeups < MAXWAKEUPS_RMBOX){
      if(!wait)
	  return FALSE;
      del_timer(&timeout_tmr);
      timeout_tmr.expires = jiffies + TIMEOUT_TICKS;
      add_timer(&timeout_tmr);
      interruptible_sleep_on(&waitq);
      wakeups++;
  }
  KDEBUG_L2("BoxEmpty wakeups %d\n",wakeups);
  if(wakeups == MAXWAKEUPS_WMBOX){
      KDEBUG_L2("BoxEmpty timeout\n");
      return FALSE;
  }
  KDEBUG_L2("BoxEmpty OK\n");
  return TRUE;
}

int ChkBoxEmpty(int wait){
    if(BoxEmpty(wait)){
	return TRUE;
    }
    return FALSE;
}

int WriteBox(unsigned long data, int wait){
    if(BoxEmpty(wait)){
	if( dspsim_td.active ){
	    KDEBUG("Warning: WriteBox called, but dspemu is busy, ignoreing this call!");
	    return FALSE;
	}else{
	    REQD_DSP   = TRUE;
	    ACKD_DSP = FALSE;
	    wake_up_interruptible(&dspsim_td.wq);
	    return TRUE;
	}
    }
    return FALSE;
}

int ChkBoxFull(int wait){
    if(BoxFull(wait)){
	return TRUE;
    }
    return FALSE;
}

int ReadBox(unsigned long *data, int wait){
    if(BoxFull(wait)){
	*data = 0;
	// ACK;
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
    if( ((unsigned int)f->f_pos + count) < PCDSP_DPRAM_SIZE){
	KDEBUG_L2("%d bytes at %x\n", count, f->f_pos);
	copy_to_user(buf, pcdsp_dprambaseptr + (unsigned int)f->f_pos, count);
	return 0;
    }
    return -EINVAL;
}

/* Write Data to DSP */
static ssize_t pcdsp_write(struct file *f, const char *buf, size_t count, loff_t *ppos)
{
    if( ((unsigned int)f->f_pos + count) < PCDSP_DPRAM_SIZE){
	KDEBUG_L2("write %d bytes at %x\n", count, f->f_pos);
	copy_from_user(pcdsp_dprambaseptr + (unsigned int)f->f_pos, buf, count);
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
    //  KDEBUG("pcdsp_ioctl::%d\n", cmd);
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

  case PCDSP_PUT_SPEED: 
      return 0;
  case PCDSP_RESET:
    KDEBUG("reset\n");
    return 0;
  case PCDSP_HALT:   
    KDEBUG("halt\n");
    return 0;
  case PCDSP_RUN:   
    KDEBUG("run\n");
    return 0;

  case PCDSP_GETMODID:
    return MODID;

  default: 
    return -1;
  }
  return 0;
}

/*
 * init some module variables
 */
int pcdsp_initialize(){
  opened=0;

  dspsim_td.SrvReqAck=FALSE;
  dspsim_td.virtual_dpram=NULL;

  // check Memory availability
  dspsim_td.virtual_dpram = (void *)kmalloc (PCDSP_DPRAM_SIZE, GFP_KERNEL);
  if (dspsim_td.virtual_dpram == NULL){
    KDEBUG("Could not allocate Memory !\n");
    return 1;
  }

  memset(dspsim_td.virtual_dpram, 0, PCDSP_DPRAM_SIZE);
  pcdsp_dprambaseptr = dspsim_td.virtual_dpram;

  KDEBUG_L1("Driver %s\n", PCDSP_VERSION);

  start_dsp(&dspsim_td);
  dspsim_td.SrvReqAck=TRUE;
  dspsim_td.ReqDAck  =FALSE;

  if( InitEmu(&dspsim_td) )
      return 1;


  return 0;
}

inline void pcdsp_quit(){

  ExitEmu();

  stop_dsp(&dspsim_td);

  kfree(dspsim_td.virtual_dpram);
  dspsim_td.virtual_dpram=NULL;
  KDEBUG("Simulation Driver deinstalled\n");
}

/*
 * And now module code and kernel interface.
 */
/*
struct file_operations {
        loff_t (*llseek) (struct file *, loff_t, int);
        ssize_t (*read) (struct file *, char *, size_t, loff_t *);
        ssize_t (*write) (struct file *, const char *, size_t, loff_t *);
        int (*readdir) (struct file *, void *, filldir_t);
        unsigned int (*poll) (struct file *, struct poll_table_struct *);
        int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
        int (*mmap) (struct file *, struct vm_area_struct *);
        int (*open) (struct inode *, struct file *);
        int (*flush) (struct file *);
        int (*release) (struct inode *, struct file *);
        int (*fsync) (struct file *, struct dentry *);
        int (*fasync) (int, struct file *, int);
        int (*check_media_change) (kdev_t dev);
        int (*revalidate) (kdev_t dev);
        int (*lock) (struct file *, int, struct file_lock *);
};
*/

struct file_operations pcdsp_fops = {
  pcdsp_seek,   /* Set DPRAM R/W Position */
  pcdsp_read,   /* Read Data to DPRAM */
  pcdsp_write,  /* Write Data to DPRAM */
  NULL,         /* readdir */
  NULL,         /* poll */
  pcdsp_ioctl,  /* ioctl */
  NULL,         /* mmap */
  pcdsp_open,   /* open */
  NULL,         /* flush */
  pcdsp_release,/* release */
  NULL,         /* fsync */
  NULL,         /* fasync */
  NULL,         /* check_media_change */
  NULL,         /* revalidate */
  NULL          /* lock */
};

int init_module(void){
    KDEBUG("init_module PCDSP:\n");
    //    PCDSP_MAJOR, ...
    if ((pcdsp_major = register_chrdev (0, PCDSP_DEVICE_NAME, &pcdsp_fops)) == -EBUSY){
	KDEBUG("unable to get major for pcdsp device: %s\n", PCDSP_DEVICE_NAME);
	return -EIO;
    }
    KDEBUG("Major number :%i \n", pcdsp_major);

    if(pcdsp_initialize())
	return -1; /* some error occured */
    
    KDEBUG("init_module PCDSP done.\n");
    return 0; /* success */
}

void cleanup_module(void){
    KDEBUG("Module PCDSP cleanup:\n");
    pcdsp_quit();
    unregister_chrdev(pcdsp_major, PCDSP_DEVICE_NAME);
    KDEBUG("Module PCDSP unregistered.\n");
}
