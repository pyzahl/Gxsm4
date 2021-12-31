/*
 * Driver for Signal Ranger MK2 DSP boards (linux-2.6.18+)
 *
 * Copyright (C) 2002 Percy Zahl
 *
 * Percy Zahl <zahl@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */ 

/* read vendor and product IDs from the sranger */
#define SRANGER_MK2_IOCTL_VENDOR _IOR('U', 0x20, int)
#define SRANGER_MK2_IOCTL_PRODUCT _IOR('U', 0x21, int)
/* send/recv a control message to the sranger */
/* #define SRANGER_MK2_IOCTL_CTRLMSG _IOWR('U', 0x22, struct usb_ctrlrequest ) */

/* FIXME: These are NOT registered ioctls()'s */
#define SRANGER_MK2_IOCTL_ASSERT_DSP_RESET           73
#define SRANGER_MK2_IOCTL_RELEASE_DSP_RESET          74
#define SRANGER_MK2_IOCTL_INTERRUPT_DSP_FROM_HPI     75
#define SRANGER_MK2_IOCTL_W_LEDS                     76
#define SRANGER_MK2_IOCTL_HPI_MOVE_OUTREQUEST        77
#define SRANGER_MK2_IOCTL_HPI_MOVE_INREQUEST         78
#define SRANGER_MK2_IOCTL_HPI_CONTROL_REQUEST        79
#define SRANGER_MK2_IOCTL_SPEED_FAST                 83
#define SRANGER_MK2_IOCTL_SPEED_SLOW                 84
#define SRANGER_MK2_IOCTL_DSP_STATE_SET              85
#define SRANGER_MK2_IOCTL_DSP_STATE_GET              86
#define SRANGER_MK2_IOCTL_ERROR_COUNT                87
#define SRANGER_MK2_IOCTL_CLR_ERROR_COUNT            88
#define SRANGER_MK2_IOCTL_ABORT_KERNEL_OP            89

#define SRANGER_MK2_IOCTL_MBOX_K_WRITE               90
#define SRANGER_MK2_IOCTL_MBOX_K_READ                91
#define SRANGER_MK2_IOCTL_KERNEL_EXEC                92


#define SRANGER_MK2_SEEK_DATA_SPACE  1
#define SRANGER_MK2_SEEK_PROG_SPACE  2
#define SRANGER_MK2_SEEK_IO_SPACE    3


