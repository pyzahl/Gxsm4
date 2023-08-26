#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <vector>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include <DataManager.h>
#include <CustomParameters.h>


#include "fpga_cfg.h"

#define FPGA_GPIO_SIZE    0x1000     // 0x1000

extern void *FPGA_PACPLL_cfg;
extern void *FPGA_PACPLL_gpio;
extern void *FPGA_PACPLL_bram;

extern int verbose;

inline uint8_t* cfg_reg_adr(int cfg_slot){
        size_t off;
        off = cfg_slot * 4;
        return ((uint8_t*)FPGA_PACPLL_cfg + off);
}

inline void set_gpio_cfgreg_int32 (int cfg_slot, int value){
        *((int32_t *)(cfg_reg_adr(cfg_slot))) = value;

        //size_t off = cfg_slot * 4;
        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = value;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value){
        *((int32_t *)(cfg_reg_adr(cfg_slot))) = value;
        //size_t off = cfg_slot * 4;

        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = value;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint32 %04x = %08x %d\n", cfg_slot, off, value, value);
}

inline void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2){
        //size_t off = cfg_slot * 4; // ** +0x8000 old
        union { struct { gint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = mem.ww;

        *((int32_t *)(cfg_reg_adr(cfg_slot))) = mem.ww;
        
        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] int16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2){
        //size_t off = cfg_slot * 4; // ** +0x8000 old
        union { struct { guint16 hi, lo; } hl; int ww; } mem;
        mem.hl.hi = value_1;
        mem.hl.lo = value_2;

        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off)) = mem.ww;
        *((int32_t *)(cfg_reg_adr(cfg_slot))) = mem.ww;

        //if (verbose > 2) fprintf(stderr, "set_gpio32[CFG%d] uint16|16 %04x = %04x | %04x\n", cfg_slot, off, value_1, value_2);
}

inline void set_gpio_cfgreg_int64 (int cfg_slot, long long value){
        size_t off_lo = cfg_slot * 4;
        size_t off_hi = off_lo + 4;
        unsigned long long uv = (unsigned long long)value;
        if (verbose > 2) fprintf(stderr, "set_gpio64[CFG%d] int64 %04x = %08x %d\n", cfg_slot, off_lo, value, value);
        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off_lo)) = uv & 0xffffffff;
        //*((int32_t *)((uint8_t*)FPGA_PACPLL_cfg + off_hi)) = uv >> 32;
        *((int32_t *)(cfg_reg_adr(cfg_slot)))   = uv & 0xffffffff;
        *((int32_t *)(cfg_reg_adr(cfg_slot+1))) = uv >> 32;
}

 
// 48bit in 64cfg reg from top -- eventual precision bits=0
inline void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value){
        set_gpio_cfgreg_int64 (cfg_slot, value << 16);
}
 

inline int32_t read_gpio_reg_int32_t (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;

        return *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset)); 
}

inline int read_gpio_reg_int32 (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;
        int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset));
        return x;
}

inline unsigned int read_gpio_reg_uint32 (int gpio_block, int pos){
        size_t offset = (gpio_block-1) * FPGA_GPIO_SIZE + pos * 8;
        unsigned int x = *((int32_t *)((uint8_t*)FPGA_PACPLL_gpio + offset));
        return x;
}

