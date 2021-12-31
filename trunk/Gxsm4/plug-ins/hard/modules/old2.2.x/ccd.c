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
 * CCD Kernel Module
 * CCD on ParPort
 *
 * Device: /dev/ccd
 *
 * making device: mknod -m 666 /dev/ccd c xxx 0
 * for major "xxx" look in log/messages while loading module !
 * in most cases: xxx = 254
 */
#define CCD_VERSION "V0.1 (C) P.Zahl 2000"
/*
 * Compile all versions with make
 *
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/major.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/fcntl.h>
#include <linux/sched.h>
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
#include <linux/smp_lock.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/bitops.h>

#include "ccd.h"
#include "dbgstuff.h"


#define my_iounmap(x, b)             (((long)x<0x100000)?0:vfree ((void*)x))

#define capable(x)                   suser()

#define queue_task                   queue_task_irq_off
#define tty_flip_buffer_push(tty)    queue_task(&tty->flip.tqueue, &tq_timer)
#define signal_pending(current)      (current->signal & ~current->blocked)
#define schedule_timeout(to)         do {current->timeout = jiffies + (to);schedule ();} while (0)


#define test_and_set_bit(nr, addr)   set_bit(nr, addr)
#define test_and_clear_bit(nr, addr) clear_bit(nr, addr)

/* Not yet implemented on 2.0 */
#define ASYNC_SPD_SHI  -1
#define ASYNC_SPD_WARP -1




/* Ugly hack: the driver_name doesn't exist in 2.0.x . So we define it
   to the "name" field that does exist. As long as the assignments are
   done in the right order, there is nothing to worry about. */
#define driver_name           name 

/* Should be in a header somewhere. They are in tty.h on 2.2 */
#define TTY_HW_COOK_OUT       14 /* Flag to tell ntty what we can handle */
#define TTY_HW_COOK_IN        15 /* in hardware - output and input       */


/*
 * Driver Variables 
 */

static int ccd_major;
static int opened;

/* Hardware Location */
static ccd_parport ccd_parport_io;
static ccd_parport *ccd_io;

/* Process handling... */
static struct wait_queue *waitq = NULL;

/*
 * Timings in jiffies,  1 jiffie dauert ca. 10ms (HZ = #jiffies fue 1sec)
 */

#define TIMEOUT_TICKS    19      /* 200ms */
#define JIFFIES_SEM      2      /* timeout after 50ms */
#define MAXWAKEUPS_SEM   10

/* Prototypes */

static void timeout(unsigned long ignore);
void mysleep(unsigned long myjiffies);

/* Device Service Functions */
static ssize_t ccd_read (struct file *, char *, size_t, loff_t *);
static int ccd_ioctl (struct inode *, struct file *, unsigned int, unsigned long);

int ccd_initialize(void);
inline void ccd_quit(void);

int init_module(void);
void cleanup_module(void);

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
 * Driver FS-Functionality Implementation
 */

/* Read CCD Data */
static ssize_t ccd_read(struct file *f, char *buf, size_t count, loff_t *ppos)
{
    /*
    KDEBUG_L2("%d bytes at %x\n",count, f->f_pos);
    tmp_buf=(char *)kmalloc(count, GFP_KERNEL );
    memcpy_fromio(tmp_buf, pcdsp_dprambaseptr + (unsigned int)f->f_pos, count);
    copy_to_user(buf, tmp_buf, count);
    kfree(tmp_buf);
    tmp_buf=NULL;
    free_dport();
    */
    return 0;
}

static int ccd_open(struct inode *inode, struct file *f){
  MOD_INC_USE_COUNT;
  opened++;  
  f->f_pos=0;
  KDEBUG("Opened %d times\n",opened);
  return 0;
}

static int ccd_release (struct inode *inode, struct file *f ){
  KDEBUG("release, still open:%d\n", opened-1);
  MOD_DEC_USE_COUNT;
  opened--;
  return 0;
}

static int ccd_ioctl(struct inode *inode, struct file *f, unsigned int cmd, unsigned long arg){
    unsigned int cycles=arg/10;
    int pixelvalue;
    //    KDEBUG("ccd_ioctl::%d\n", cmd);
    switch(cmd){
    case CCD_CMD_GETPIXEL:
	pixelvalue = (int)CCD_PixWert(ccd_io);
	CCD_Next(ccd_io);
	CCD_Lesen(ccd_io);
	return pixelvalue;

    case CCD_CMD_INITLESEN:
	CCD_Lesen(ccd_io);
	return 0;
	
    case CCD_CMD_MONITORENABLE:
	// Switch in Monitoring Mode as default (free fast videomode run @ ~20ms)
	CCD_Monitoring(ccd_io);
	return 0;
	
    case CCD_CMD_CLEAR:
	CCD_Monitoring(ccd_io);
	mysleep(6);
	CCD_Sammeln(ccd_io);
	mysleep(4);
	CCD_Move2Mem(ccd_io);
	return 0;
	
    case CCD_CMD_EXPOSURE:
	if(cycles < 2) cycles = 2;
	CCD_Sammeln(ccd_io);
	mysleep(4);
	CCD_Move2Mem(ccd_io);
	CCD_Sammeln(ccd_io);
	mysleep(cycles);
	CCD_Move2Mem(ccd_io);
	CCD_Sammeln(ccd_io);
	mysleep(4);
	CCD_Move2Mem(ccd_io);
	CCD_Sammeln(ccd_io);
	return 0;
	
    default: 
	return -1;
    }
    return 0;
}

/*
 * init some module variables
 */
int ccd_initialize(){
  opened=0;

  ccd_io = &ccd_parport_io;

  KDEBUG_L1("Driver %s\n", CCD_VERSION);

  ccd_io->base   = LPT1_BASE;
  ccd_io->basehi = ccd_io->base + 0x400;
  KDEBUG("iobase: 0x%03x\n", (int)ccd_io->base);

  ccd_io->save_status = inw_p(PARP_DATA(ccd_io));
  ccd_io->save_mode   = inb_p(PARP_CONTROL(ccd_io));
  ccd_io->save_ecmode = inb_p(PARP_ECPCONTROL(ccd_io));
  // Setup for 8bit reading
  outw_p(0xffff, PARP_DATA(ccd_io));
  outb_p(0x20,   PARP_ECPCONTROL(ccd_io));

  // Switch in Monitoring Mode as default (free fast videomode run @ ~20ms)
  CCD_Monitoring(ccd_io);

  KDEBUG("Init CCD module OK.\n");

  return 0;
}

inline void ccd_quit(){
  // Switch in Monitoring Mode as default (free fast videomode run @ ~20ms)
  CCD_Monitoring(ccd_io);

  // Restore previous mode
  outw_p(ccd_io->save_status, PARP_DATA(ccd_io));
  outb_p(ccd_io->save_mode, PARP_CONTROL(ccd_io));
  outb_p(ccd_io->save_ecmode, PARP_ECPCONTROL(ccd_io));

  KDEBUG("CCD Driver deinstalled\n");
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

struct file_operations ccd_fops = {
  NULL,         /* Set DPRAM R/W Position */
  ccd_read,   /* Read Data to DPRAM */
  NULL,         /* Write Data to DPRAM */
  NULL,         /* readdir */
  NULL,         /* poll */
  ccd_ioctl,  /* ioctl */
  NULL,         /* mmap */
  ccd_open,   /* open */
  NULL,         /* flush */
  ccd_release,/* release */
  NULL,         /* fsync */
  NULL,         /* fasync */
  NULL,         /* check_media_change */
  NULL,         /* revalidate */
  NULL          /* lock */
};

int init_module(void){
  KDEBUG("init_module CCD\n");
  if ((ccd_major = register_chrdev (0, CCD_DEVICE_NAME, &ccd_fops))== -EBUSY){
      KDEBUG("unable to get major for ccd device: %s\n", CCD_DEVICE_NAME);
      return -EIO;
    }
  KDEBUG("Major number :%i \n", ccd_major);

  if(ccd_initialize())
    return -1; /* some error occured */

  return 0; /* success */
}

void cleanup_module(void){
  KDEBUG("Module CCD unregistered\n");
  unregister_chrdev(ccd_major, CCD_DEVICE_NAME);

  ccd_quit();

  KDEBUG("cleanup_module\n");
}


