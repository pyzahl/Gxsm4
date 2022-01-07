// g++ `pkg-config --libs  --cflags glib-2.0` test.C

#include <locale.h>
#include <libintl.h>
#include <time.h>
#include <unistd.h>



#include <climits>    // for limits macrors
#include <iomanip>    // for setfill, setprecision, setw
#include <ios>        // for dec, hex, oct, showbase, showpos, etc.
#include <iostream>   // for cout
#include <sstream>    // for istringstream

#include <fcntl.h>
#include <sys/ioctl.h>

#include "string.h"
#include "glib.h"



#define FB_SPM_MAGIC_ADR  0x10F13F00 /**< Magic struct is at this fixed addess in external SRAM */
#define FB_SPM_MAGIC     0x3202EE01

#define SRDEV "/dev/sranger_mk2_2"

typedef gint32   DSP_INT;
typedef guint32  DSP_UINT;
typedef guint32  DSP_UINTA;
typedef gint16   DSP_INT16;
typedef guint16  DSP_UINT16;
typedef gint32   DSP_INT32;
typedef guint32  DSP_UINT32;
typedef gint64   DSP_INT64;
typedef guint64  DSP_UINT64;


/* read vendor and product IDs from the sranger */
#define SRANGER_MK2_IOCTL_VENDOR _IOR('U', 0x20, int)
#define SRANGER_MK2_IOCTL_PRODUCT _IOR('U', 0x21, int)
/* send/recv a control message to the sranger */
/* #define SRANGER_MK2_IOCTL_CTRLMSG _IOWR('U', 0x22, struct usb_ctrlrequest ) */

/* FIXME: These are NOT registered ioctls()'s */
#define SRANGER_MK2_IOCTL_ASSERT_DSP_RESET           73
#define SRANGER_MK2_IOCTL_RELEASE_DSP_RESET          74
#define SRANGER_MK23_IOCTL_ASSERT_DSP_RESET           73
#define SRANGER_MK23_IOCTL_RELEASE_DSP_RESET          74
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

#define SRANGER_MK23_IOCTL_QUERY_EXCLUSIVE_MODE      100
#define SRANGER_MK23_IOCTL_CLR_EXCLUSIVE_MODE        101
#define SRANGER_MK23_IOCTL_SET_EXCLUSIVE_AUTO        102
#define SRANGER_MK23_IOCTL_SET_EXCLUSIVE_UNLIMITED   103

#define SRANGER_MK23_SEEK_ATOMIC      1 // Bit indicates ATOMIC mode to be used
#define SRANGER_MK23_SEEK_DATA_SPACE  0 // DATA_SPACE, NON ATOMIC -- MK2: ONLY VIA USER PROVIDED NON ATOMIC KFUNC!!!!
#define SRANGER_MK23_SEEK_PROG_SPACE  2 // default mode
#define SRANGER_MK23_SEEK_IO_SPACE    4 // default mode

#define SRANGER_MK23_SEEK_EXCLUSIVE_AUTO 8 // request exclusive write access for device, auto released after following read access and always after close
#define SRANGER_MK23_SEEK_EXCLUSIVE_UNLIMITED 0x10 // request exclusive write access for device until reset by seek or after close

#define SRANGER_MK2_SEEK_DATA_SPACE  1
#define SRANGER_MK2_SEEK_PROG_SPACE  2
#define SRANGER_MK2_SEEK_IO_SPACE    3

#define SRANGER_ERROR(X) std::cerr << X



typedef struct {
	DSP_UINT32 magic;         /**< 0: Magic Number to validate entry =RO */
	DSP_UINT32 version;       /**< 1: SPM soft verion, BCD =RO */
	DSP_UINT32 year,mmdd;     /**< 2,3: year, date, BCD =RO */
	DSP_UINT32 dsp_soft_id;   /**< 4: Unique SRanger DSP software ID =RO */
	/* -------------------- Magic Symbol Table ------------------------------ */
	DSP_UINTA statemachine;  /**< 5: Address of statemachine struct =RO */
	DSP_UINTA AIC_in;        /**< 6: AIC in buffer =RO */
	DSP_UINTA AIC_out;       /**< 7: AIC out buffer =RO */
	DSP_UINTA AIC_reg;       /**< 8: AIC register (AIC control struct) =RO */
	/* -------------------- basic magic data set ends ----------------------- */
	DSP_UINTA feedback;      /**<  9: Address of feedback struct =RO */
	DSP_UINTA analog;        /**< 10: Address of analog struct =RO */
	DSP_UINTA scan;          /**< 11: Address of ascan struct =RO */
	DSP_UINTA move;          /**< 12: Address of move struct =RO */
	DSP_UINTA probe;         /**< 13: Address of probe struct =RO */
	DSP_UINTA autoapproach;  /**< 14: Address of autoapproch struct =RO */
	DSP_UINTA datafifo;      /**< 15: Address of datafifo struct =RO */
	DSP_UINTA probedatafifo; /**< 16: Address of probe datafifo struct =RO */
	DSP_UINTA scan_event_trigger;/**< 17: Address of scan_event_trigger struct =RO */
	DSP_UINTA feedback_mixer; /**< 18: Address of DFM FUZZY mix control parameter struct =RO */
	DSP_UINTA CR_out_puls;   /**< 19: Address of CoolRunner IO Out Puls action struct =RO */
	DSP_UINTA external;      /**< 20: Address of external control struct =RO */
	DSP_UINTA CR_generic_io; /**< 21: Address of CR_generic_io struct =RO */
	DSP_UINTA a810_config;   /**< 22: Address of a810_config struct =RO */
	DSP_UINTA watch;         /**< 23: Address of watch struct if available =RO */
	DSP_UINTA dummy1, dummy2; /**< --: Address of xxx struct =RO */
	DSP_UINT32 dummyl;         /**< --: Address of xxx struct =RO */
} SPM_MAGIC_DATA_LOCATIONS;


void p32(const gchar *l, gint32 x) {
	gchar *info = g_strdup_printf ("0x%08x", x);
	std::cout << l << " = " << info << " [32] = " << x << std::endl;
}

void p16(const gchar *l, gint32 x) {
	gchar *info = g_strdup_printf ("0x%08x", x);
	std::cout << l << " = " << info << " [16] = " << x << std::endl;
}

void pu32(const gchar *l, guint32 x) {
	gchar *info = g_strdup_printf ("0x%08u", x);
	std::cout << l << " = " << info << " [u32] = " << x << std::endl;
}

void dumpbuffer(unsigned char *buffer, int bi, int bf){
	int i, j;
	unsigned char tmp[16];
	for(i=bi; i<=bf; i+=16){
		memset (tmp, 0, 16);
		for(j=0; (i+j) <= bf && j<16; ++j) tmp[j] = buffer[i+j];
		g_print("%06X: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n", i>>1,
		       tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],
		       tmp[8],tmp[9],tmp[10],tmp[11],tmp[12],tmp[13],tmp[14],tmp[15]);
	}
}

void test(){
	unsigned char x=0;
	unsigned char y=0;
	for (int i=0; i<300; ++i){
		y=x+1;
		g_print ("%d %d %d\n", i, x++, y);
	}
}

int main (){
	int dsp;
	SPM_MAGIC_DATA_LOCATIONS magic;

	test ();
	exit (0);
	
	magic.magic = 0; // set to zero, this means all data is invalid!

	if((dsp = open (SRDEV, O_RDWR)) <= 0){
		std::cerr << "Can not open " << SRDEV << std::endl;
		return -1;
	}
	else{
		int ret;
		unsigned int vendor, product;
		if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_VENDOR, (unsigned long)&vendor)) < 0){
			SRANGER_ERROR(strerror(ret) << " cannot query VENDOR" << std::endl);
			close (dsp);
			dsp = 0;
			return -1;
		}
		if ((ret=ioctl(dsp, SRANGER_MK2_IOCTL_PRODUCT, (unsigned long)&product)) < 0){
			SRANGER_ERROR(strerror(ret) << " cannot query PRODUCT" << std::endl);
			close (dsp);
			dsp = 0;
			return -1;
		}

		gchar *info = g_strdup_printf ("SignalRanger Vendor 0x%04x : Product 0x%04x", vendor, product);
		std::cout << info << std::endl;

		p32 ("Reading DSP Code Magic, size", sizeof (magic));

		// now read magic struct data
		lseek (dsp, FB_SPM_MAGIC_ADR, SRANGER_MK2_SEEK_DATA_SPACE);
		if ((ret=read (dsp, &magic, sizeof (magic))) < 0){
			SRANGER_ERROR(strerror(ret) << " READ ERROR" << std::endl);
		}

		dumpbuffer ( (unsigned char*)&magic, 0, sizeof (magic)-1);

		p32 ("magic", magic.magic);
//		p32 ("magic", GUINT32_FROM_LE (magic.magic));

		p32 ("version", magic.version);
		p32 ("year", magic.year);
		p32 ("mmdd", magic.mmdd);
		p32 ("softid", magic.dsp_soft_id);
		p32 ("statemachine", magic.statemachine);

		close (dsp);
	}
	
	return 0;
}
