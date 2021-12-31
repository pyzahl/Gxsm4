/*
 *
 *   data structures common to kernel modules and applications
 *
 */

#define HERE NAME, __FUNCTION__
#define MMAP_REGIONS 4

#if DEBUG == 1
#define dprintk(...) printk (__VA_ARGS__);
#else
#define dprintk(...)    /* suppress this messages */
#endif

/* data structure for events */

struct event {
	struct timespec time[2];        /* service and exit times */
	s16 adc[8];                     /* ADC readings */
	s16 dac[8];                     /* values written to DAC */
	u8 byte[2];                     /* digital line values */
	u16 samples;                    /* number of processed ADC */
	u32 adc_time;                   /* ADC read time */
};

#define SEC_DIVISOR 1000000000
#define SLOT sizeof(struct event)
#define PAD (SLOT-1)

/* data structure for scan parameters */

struct scan_params {
	char code[2];
	u16 mask;

	u32 lines_per_frame;
	u32 points_per_line;
	u32 cadence_usec;
	u32 buffer_size;
	u32 buffer_size_min;
	u32 high_water;
	u32 timeout;

	char pay_load[64];
};

/* data structure for scan parameters */

#define DEFAULT_CADENCE_USEC 200

#define LINES_PER_FRAME  0x001
#define POINTS_PER_LINE  0x002
#define CADENCE_USEC     0x004
#define BUFFER_SIZE      0x008
#define BUFFER_SIZE_MIN  0x010
#define HIGH_WATER       0x020
#define TIMEOUT          0x040
#define PAY_LOAD         0x080
#define INIT_IRQ_PARAMETERS {NULL, Idle, DEFAULT_CADENCE_USEC, Irq_None, Again}

struct irq_parameters {

        struct event * unibuf;
        enum {Idle, Go_Process} action;
        int cadence_usec;

        enum {Irq_None, Irq_Overrun, Irq_Stopped} completion;
        enum {Again, Go_Idle} request;
        int buffer_size;    /* total buffer size (in slots) */
        int write_pointer;  /* in slots */
        int read_pointer;   /* in bytes */
        int awake_in;
        int buffer_count;   /* slots (or part of a slot) in buffer */

        int points;         /* points in current line to be processed */
        int lines;          /* lines to be processed */
        int points_per_line;
        int lines_per_frame;
        int sync;

        char pay_load[64];
};

void feedback_code (struct event *, struct irq_parameters *);
