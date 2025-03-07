/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 8 c-style: "K&R" -*- */

/***************************************************************************//**
 *   @file   ad463x.c
 *   @brief  Implementation of AD463x Driver.
 *   @author Antoniu Miclaus (antoniu.miclaus@analog.com)
********************************************************************************
 * Copyright 2021(c) Analog Devices, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES, INC. “AS IS” AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL ANALOG DEVICES, INC. BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "no_os_error.h"
//#include "no_os_delay.h"
//#include "no_os_units.h"
#include "ad463x.h"
//#include "no_os_print_log.h"
//#include "no_os_alloc.h"
//#include "no_os_spi.h"

#include "fpga_cfg.h"
#include "spmc_module_config_utils.h"

/******************************************************************************/
/********************** Macros and Constants Definitions **********************/
/******************************************************************************/
#define AD463x_TEST_DATA 0xAA

#define ADAQ4224_GAIN_MAX_NANO 6670000000ULL


#define DBG_V_LEVEL 1

/*
 * Gains computed as fractions of 1000 so they can be expressed by integers.
 */
static const int32_t ad463x_gains[4] = {
	330, 560, 2220, 6670
};

/*
 * Gains stored and computed as fractions to avoid introducing rounding errors.
 */
static const int ad4630_gains_frac[4][2] = {
	[AD463X_GAIN_0_33] = {1, 3},
	[AD463X_GAIN_0_56] = {5, 9},
	[AD463X_GAIN_2_22] = {20, 9},
	[AD463X_GAIN_6_67] = {20, 3},
};

/******************************************************************************/
/************************* Functions Definitions ******************************/
/******************************************************************************/

/**
 * @brief Read device register.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The data read from the register.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_spi_reg_read(struct ad463x_dev *dev,
			    uint16_t reg_addr,
			    uint8_t *reg_data,
			    int xverbose=0)
{
#if 0
	int32_t ret=0;
	uint8_t buf[3];

	buf[0] = AD463X_REG_READ | ((reg_addr >> 8) & 0x7F);
	buf[1] = (uint8_t)reg_addr;
	buf[2] = AD463X_REG_READ_DUMMY;

	ret = no_os_spi_write_and_read(dev->spi_desc, buf, 3); // ad463x_spi_reg_read PORTED BELOW
	*reg_data = buf[2];
#endif

	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_CONFIG | SPMC_AD463X_CONFIG_MODE_RW, MODULE_START_VECTOR); // SET CONFIG MODE for READ
	rp_spmc_module_config_uint32 (MODULE_SETUP, ((AD463X_REG_READ_B15) | (reg_addr & 0x7fff)) << 8, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24,                                      MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));
        unsigned int a,b;
        rp_spmc_module_read_config_data_u (SPMC_READBACK_AD463X_REG, &a, &b);
	
	*reg_data = a & 0xff;
	
	if (xverbose && verbose > DBG_V_LEVEL)
	  fprintf (stderr,"ad463x_spi_reg_read:  RA:%04x D:%02x => X:%08x [%08x]\n", reg_addr, a&0xff, ((AD463X_REG_READ_B15) | (reg_addr & 0x7fff)) << 8, a);
	
	return 0;
}

/**
 * @brief Write device register.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param reg_data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_spi_reg_write(struct ad463x_dev *dev,
			     uint16_t reg_addr,
			     uint8_t reg_data,
			    int xverbose=0)
{
#if 0
	uint8_t buf[3];

	buf[0] = ((reg_addr >> 8) & 0x7F);
	buf[1] = (uint8_t)reg_addr;
	buf[2] = reg_data;

	return no_os_spi_write_and_read(dev->spi_desc, buf, 3); // ad463x_spi_reg_write PORTED BELOW
#endif

	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_CONFIG | SPMC_AD463X_CONFIG_MODE_RW, MODULE_START_VECTOR); // SET CONFIG MODE for WRITE+data
	rp_spmc_module_config_uint32 (MODULE_SETUP, ((reg_addr & 0x7fff) << 8) | (reg_data & 0xff),  MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24,                                   MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));

	if (xverbose && verbose > DBG_V_LEVEL){
	  unsigned int a,b;
	  rp_spmc_module_read_config_data_u (SPMC_READBACK_AD463X_REG, &a, &b);
	  fprintf (stderr,"ad463x_spi_reg_write: WA:%04x D:%02x => X:%08x [%08x]\n", reg_addr, reg_data&0xff, ((reg_addr & 0x7fff) << 8) | (reg_data&0xff), a);
	}
	return 0;
}

/**
 * @brief Enter register configuration mode.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_enter_config_mode(struct ad463x_dev *dev)
{
#if 0
	uint8_t buf[3];

	if (!dev)
		return -EINVAL;

	buf[0] = 0xA0;
	buf[1] = 0x00;
	buf[2] = 0x00;

	return no_os_spi_write_and_read(dev->spi_desc, buf, 3); //  ad463x_enter_config_mode PORTED BELOW
#endif

	if (verbose > DBG_V_LEVEL)
	  fprintf (stderr,"ad463x_enter_config_mode. WA: 0xA000\n");

	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_CONFIG | SPMC_AD463X_CONFIG_MODE_RW, MODULE_START_VECTOR); // SET CONFIG MODE for WRITE+data
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0xA000 << 8,   MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));

	return 0;
}

/**
 * @brief SPI read device register using a mask.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param mask - The mask.
 * @param data - The data read from the register.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_spi_reg_read_masked(struct ad463x_dev *dev,
				   uint16_t reg_addr,
				   uint8_t mask,
				   uint8_t *data)
{
	uint8_t reg_data;
	int32_t ret;

	ret = ad463x_spi_reg_read(dev, reg_addr, &reg_data);
	if (ret != 0)
		return ret;

	*data = no_os_field_get(mask, reg_data);

	return ret;
}

/**
 * @brief SPI write device register using a mask.
 * @param dev - The device structure.
 * @param reg_addr - The register address.
 * @param mask - The mask.
 * @param data - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_spi_reg_write_masked(struct ad463x_dev *dev,
				    uint16_t reg_addr,
				    uint8_t mask,
				    uint8_t data)
{
	uint8_t reg_data;
	int32_t ret;

	ret = ad463x_spi_reg_read(dev, reg_addr, &reg_data);
	if (ret != 0)
		return ret;

	reg_data &= ~mask;
	reg_data |= data;

	return ad463x_spi_reg_write(dev, reg_addr, reg_data);
}

/**
 * @brief Set power mode.
 * @param dev - The device structure.
 * @param mode - The power mode.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_set_pwr_mode(struct ad463x_dev *dev, uint8_t mode)
{
	if ((mode != AD463X_NORMAL_MODE) && (mode != AD463X_LOW_POWER_MODE))
		return -1;

	return ad463x_spi_reg_write(dev, AD463X_REG_DEVICE_CONFIG, mode);
}

/**
 * @brief Set average frame length.
 * @param dev - The device structure.
 * @param mode - Average filter frame length mode.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_set_avg_frame_len(struct ad463x_dev *dev, uint8_t mode)
{
	int32_t ret;

	if (mode == AD463X_NORMAL_MODE) {
		ret = ad463x_spi_reg_write_masked(dev,
						  AD463X_REG_MODES,
						  AD463X_OUT_DATA_MODE_MSK,
						  AD463X_24_DIFF_8_COM);
		if (ret != 0)
			return ret;
		ret = ad463x_spi_reg_write(dev,
					   AD463X_REG_AVG,
					   AD463X_AVG_FILTER_RESET);
	} else {
		if (mode < 1 || mode > 16)
			return -EINVAL;
		ret = ad463x_spi_reg_write_masked(dev,
						  AD463X_REG_MODES,
						  AD463X_OUT_DATA_MODE_MSK,
						  AD463X_30_AVERAGED_DIFF);
		if (ret != 0)
			return ret;
		ret = ad463x_spi_reg_write(dev,
					   AD463X_REG_AVG,
					   mode);
	}

	return ret;
}

/**
 * @brief Set drive strength.
 * @param dev - The device structure.
 * @param mode - The register data.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_set_drive_strength(struct ad463x_dev *dev, uint8_t mode)
{
	if ((mode != AD463X_NORMAL_OUTPUT_STRENGTH)
	    && (mode != AD463X_DOUBLE_OUTPUT_STRENGTH))
		return -EINVAL;

	return ad463x_spi_reg_write_masked(dev, AD463X_REG_IO,
					   AD463X_DRIVER_STRENGTH_MASK, mode);
}

/**
 * @brief Exit register configuration mode.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_exit_reg_cfg_mode(struct ad463x_dev *dev)
{
	int32_t ret;

	ret = ad463x_spi_reg_write(dev, AD463X_REG_EXIT_CFG_MODE,
				   AD463X_EXIT_CFG_MODE);
	if (ret != 0)
		return ret;

#if 0
	if (dev->offload_enable) {
		ret = spi_engine_set_transfer_width(dev->spi_desc,
						    dev->capture_data_width);
		if (ret != 0)
			return ret;

		spi_engine_set_speed(dev->spi_desc, dev->spi_desc->max_speed_hz);
	}
#endif
	
	return ret;
}

/**
 * @brief Set channel gain
 * @param dev - The device structure.
 * @param ch_idx - The channel index.
 * @param gain - The gain value scaled by 10000.
 * 			Example: to set gain 1.5, use 150000
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_set_ch_gain(struct ad463x_dev *dev, uint8_t ch_idx,
			   uint64_t gain)
{
	int32_t ret;
	uint32_t g;

	if (gain < 0 || gain > AD463X_GAIN_MAX_VAL_SCALED)
		return -EINVAL;

	g = ((gain * 0xFFFF) / AD463X_GAIN_MAX_VAL_SCALED);

	ret = ad463x_spi_reg_write(dev,
				   AD463X_REG_CHAN_GAIN(ch_idx, AD463X_GAIN_LSB),
				   g);
	if (ret != 0)
		return ret;

	return ad463x_spi_reg_write(dev,
				    AD463X_REG_CHAN_GAIN(ch_idx, AD463X_GAIN_MSB),
				    g >> 8);
}

/**
 * @brief Set channel offset
 * @param dev - The device structure.
 * @param ch_idx - The channel index.
 * @param offset - The channel offset.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad463x_set_ch_offset(struct ad463x_dev *dev, uint8_t ch_idx,
			     uint32_t offset)
{
	int32_t ret;

	ret = ad463x_spi_reg_write(dev,
				   AD463X_REG_CHAN_OFFSET(ch_idx, AD463X_OFFSET_0),
				   offset);
	if (ret < 0)
		return ret;

	ret = ad463x_spi_reg_write(dev,
				   AD463X_REG_CHAN_OFFSET(ch_idx, AD463X_OFFSET_1),
				   offset >> 8);
	if (ret < 0)
		return ret;

	return ad463x_spi_reg_write(dev,
				    AD463X_REG_CHAN_OFFSET(ch_idx, AD463X_OFFSET_2),
				    offset >> 16);
}

#if 0
/**
 * @brief Initialize GPIO driver handlers for the GPIOs in the system.
 *        ad463x_init() helper function.
 * @param [out] dev - ad463x_dev device handler.
 * @param [in] init_param - Pointer to the initialization structure.
 * @return 0 in case of success, -1 otherwise.
 */
static int32_t ad463x_init_gpio(struct ad463x_dev *dev,
				struct ad463x_init_param *init_param)
{
	int32_t ret;
	/* configure reset pin */
	ret = no_os_gpio_get_optional(&dev->gpio_resetn, init_param->gpio_resetn);
	if (ret != 0)
		return ret;

	/** Reset to configure pins */
	ret = no_os_gpio_direction_output(dev->gpio_resetn, NO_OS_GPIO_LOW);
	if (ret != 0)
		goto error_gpio_reset;

	no_os_mdelay(100);
	ret = no_os_gpio_set_value(dev->gpio_resetn, NO_OS_GPIO_HIGH);
	if (ret != 0)
		goto error_gpio_reset;

	no_os_mdelay(100);

	ret = no_os_gpio_get_optional(&dev->gpio_cnv, init_param->gpio_cnv);
	if (ret != 0)
		goto error_gpio_reset;

	ret = no_os_gpio_direction_output(dev->gpio_cnv, NO_OS_GPIO_LOW);
	if (ret != 0)
		goto error_gpio_cnv;

	/** Configure PGIA pins, if available */
	if (dev->has_pgia) {
		ret = no_os_gpio_get_optional(&dev->gpio_pgia_a0, init_param->gpio_pgia_a0);
		if (ret != 0)
			goto error_gpio_cnv;
		ret = no_os_gpio_get_optional(&dev->gpio_pgia_a1, init_param->gpio_pgia_a1);
		if (ret != 0)
			goto error_gpio_pgia0;
		/* set default values */
		ret = no_os_gpio_direction_output(dev->gpio_pgia_a0, NO_OS_GPIO_LOW);
		if (ret != 0)
			goto error_gpio_pgia1;
		ret = no_os_gpio_direction_output(dev->gpio_pgia_a1, NO_OS_GPIO_LOW);
		if (ret != 0)
			goto error_gpio_pgia1;
	}

	return 0;

error_gpio_pgia1:
	no_os_gpio_remove(dev->gpio_pgia_a1);
error_gpio_pgia0:
	no_os_gpio_remove(dev->gpio_pgia_a0);
error_gpio_cnv:
	no_os_gpio_remove(dev->gpio_cnv);
error_gpio_reset:
	no_os_gpio_remove(dev->gpio_resetn);

	return ret;
}


/**
 * @brief Read from device.
 *        Enter register mode to read/write registers
 * @param [in] dev - ad469x_dev device handler.
 * @param [out] buf - data buffer.
 * @param [in] samples - sample number.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t ad463x_read_data_offload(struct ad463x_dev *dev,
				 uint32_t *buf,
				 uint16_t samples)
{
	int32_t ret;
	uint32_t commands_data[1] = {0};
	struct spi_engine_offload_message msg;
	uint32_t spi_eng_msg_cmds[3] = {
		CS_LOW,
		READ(dev->read_bytes_no),
		CS_HIGH
	};

	ret = no_os_pwm_enable(dev->trigger_pwm_desc);
	if (ret != 0)
		return ret;

	ret = spi_engine_offload_init(dev->spi_desc, dev->offload_init_param);
	if (ret != 0)
		return ret;

	msg.commands = spi_eng_msg_cmds;
	msg.no_commands = NO_OS_ARRAY_SIZE(spi_eng_msg_cmds);
	msg.rx_addr = (uint32_t)buf;
	msg.commands_data = commands_data;

	if (dev->dcache_invalidate_range)
		dev->dcache_invalidate_range(msg.rx_addr, samples * 2 * sizeof(buf[0]));

	/* both channels are read with a single transfer */
	ret = spi_engine_offload_transfer(dev->spi_desc, msg, samples);
	if (ret != 0)
		return ret;

	if (dev->dcache_invalidate_range)
		dev->dcache_invalidate_range(msg.rx_addr, samples * 2 * sizeof(buf[0]));

	if (dev->lane_mode == AD463X_SHARED_TWO_CH) {
		for (int i = 0; i < samples * 2; i++)
			buf[i] = no_os_get_unaligned_be32(&buf[i]);
	}

	return ret;
}
#endif

/**
 * @brief Parallel Bits Extract
 * @param in0 - fist byte of interleaved data
 * @param in1 - second byte of interleaved data
 * @param out0 - unscrambled byte 0
 * @param out1 - unscrambled byte 1
 * @return none
 */
static void ad463x_pext(uint8_t in0, uint8_t in1,
			uint8_t *out0, uint8_t *out1)
{
	uint8_t high0, high1, low0, low1;

	high0 = in0;
	low1 = in1;
	high1 = high0 << 1;
	low0 = low1 >> 1;

	high0 &= 0xAA;
	high1 &= 0xAA;
	low0 &= 0x55;
	low1 &= 0x55;

	high0 = (high0 | high0 << 001) & 0xCC;
	high0 = (high0 | high0 << 002) & 0xF0;

	high1 = (high1 | high1 << 001) & 0xCC;
	high1 = (high1 | high1 << 002) & 0xF0;

	low0 = (low0 | low0 >> 001) & 0x33;
	low0 = (low0 | low0 >> 002) & 0x0F;

	low1 = (low1 | low1 >> 001) & 0x33;
	low1 = (low1 | low1 >> 002) & 0x0F;

	*out0 = high0 | low0;
	*out1 = high1 | low1;
}

/**
 * @brief Parallel Bits Extract for sample
 * @param buf - buffer of interleaved data
 * @param size - number of bytes in the buffer
 * @param ch0_out - unscrambled byte 0
 * @param ch1_out - unscrambled byte 1
 * @return none
 */
static void ad463x_pext_sample(struct ad463x_dev *dev,
			       uint8_t *buf, int size,
			       uint32_t *ch0_out,
			       uint32_t *ch1_out)
{
	uint8_t data[8] = {0};
	uint8_t ch0[4];
	uint8_t ch1[4];
	int shift;

	memcpy(data, buf, size);
	ad463x_pext(data[0], data[1], &ch0[0], &ch1[0]);
	ad463x_pext(data[2], data[3], &ch0[1], &ch1[1]);
	ad463x_pext(data[4], data[5], &ch0[2], &ch1[2]);
	ad463x_pext(data[6], data[7], &ch0[3], &ch1[3]);

	*ch0_out = no_os_get_unaligned_be32(ch0);
	*ch1_out = no_os_get_unaligned_be32(ch1);

	shift = 32 - dev->real_bits_precision;
	if (shift) {
		*ch1_out >>= shift;
		*ch0_out >>= shift;
	}
}

/**
 * @brief read a single sample of data
 * @param dev - ad469x_dev device handler.
 * @param ch0_out - pointer to store channel 0 data
 * @param ch1_out - pointer to store channel 1 data
 * @stream_mode =0 normal immediate read, wait for result, =1 read last value while conversion
 * 
 * @return 0 in case of success, negative value otherwise.
 */
static int32_t ad463x_read_single_sample(struct ad463x_dev *dev, uint32_t *ch0_out,  uint32_t *ch1_out, int stream_mode=0, int xverbose=1, int bits=24)
{
#if 0
	uint8_t data[8] = {0};
	int ret;

	if (!dev)
		return -EINVAL;

	ret = no_os_gpio_set_value(dev->gpio_cnv, NO_OS_GPIO_HIGH);
	if (ret)
		return ret;

	ret = no_os_gpio_set_value(dev->gpio_cnv, NO_OS_GPIO_LOW);
	if (ret)
		return ret;

	ret = no_os_spi_write_and_read(dev->spi_desc, data, dev->read_bytes_no); // ad463x_read_single_sample PORTED BELOW
	if (ret)
		return ret;

	ad463x_pext_sample(dev, data, dev->read_bytes_no, ch0_out, ch1_out);
#endif

	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_CONFIG | SPMC_AD463X_CONFIG_MODE_CNV | (stream_mode ? SPMC_AD463X_CONFIG_MODE_STREAM:0),
				      MODULE_START_VECTOR); // SET CONFIG MODE for CNV
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, bits, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));
        rp_spmc_module_read_config_data_u (SPMC_READBACK_AD463X_REG, ch0_out, ch1_out);

	if (xverbose && verbose > DBG_V_LEVEL)
	  fprintf (stderr,"ad463x_read_single_sample. CNV: %08x  %08x\n", *ch0_out, *ch1_out);
	
	return 0;
}

#if 0
/**
 * @brief Read from device using dma
 * @param dev - ad469x_dev device handler.
 * @param buf - data buffer
 * @param samples - sample number.
 * @return 0 in case of success, negative otherwise.
 */
static int32_t ad463x_read_data_dma(struct ad463x_dev *dev,
				    uint32_t *buf,
				    uint16_t samples)
{
	struct no_os_spi_msg spi_msg;
	uint32_t *p_buf = buf;
	uint8_t tx_buf = 0;
	uint8_t *rx_buf, *rx_sample;
	int ret, i;

	if (!dev)
		return -EINVAL;

	//rx_buf = no_os_calloc(1, samples * dev->read_bytes_no);
	rx_buf = calloc(1, samples * dev->read_bytes_no);
	if (!rx_buf)
		return -ENOMEM;

	spi_msg.tx_buff = &tx_buf,
	spi_msg.bytes_number = samples * dev->read_bytes_no,
	spi_msg.rx_buff = rx_buf,

	ret = no_os_pwm_enable(dev->trigger_pwm_desc);
	if (ret != 0)
		goto out;

	ret = no_os_spi_transfer_dma_sync(dev->spi_desc, &spi_msg, 1);
	if (ret)
		goto out;

	ret = no_os_pwm_disable(dev->trigger_pwm_desc);
	if (ret != 0)
		goto out;

	rx_sample = rx_buf;
	for (i = 0; i < samples; i++) {
		ad463x_pext_sample(dev, rx_sample, 6, p_buf, p_buf + 1);
		rx_sample += dev->read_bytes_no;
		p_buf += 2;
	}
out:
	//no_os_free(rx_buf);
	free(rx_buf);
	return ret;
}
#endif

/**
 * @brief Read from device.
 *        Enter register mode to read/write registers
 * @param dev - ad469x_dev device handler.
 * @param buf - data buffer.
 * @param samples - sample number.
 * @return 0 in case of success, negative otherwise.
 */
int32_t ad463x_read_data(struct ad463x_dev *dev,
			 uint32_t *buf,
			 uint16_t samples)
{
	uint32_t *p_buf;
	int ret, i;

	if (!dev)
		return -EINVAL;

	//if (dev->offload_enable)
	//	return ad463x_read_data_offload(dev, buf, samples);

	//if (dev->spi_dma_enable)
	//	return ad463x_read_data_dma(dev, buf, samples);

	for (i = 0, p_buf = buf; i < samples; i++, p_buf += 2) {
		ret = ad463x_read_single_sample(dev, p_buf, p_buf + 1);
		if (ret)
			return ret;
	}
	return 0;
}

/**
 * @brief Fill Scales table based on the available PGIA gains.
 * @param dev - Pointer to the device handler.
 * @return None.
 */

// FIXME
#define MILLI 1 // ???? undef
#define MICRO 1 // ???? undef
#define NANO  1 // ???? undef

static int ad463x_fill_scale_tbl(struct ad463x_dev *dev)
{
	int val, val2, tmp0;
	int32_t tmp1;
	unsigned int i;
	int64_t tmp2;

	if (!dev)
		return -EINVAL;

	val2 = dev->real_bits_precision;
	for (i = 0; i < NO_OS_ARRAY_SIZE(ad463x_gains); i++) {
		val = (dev->vref * 2) / MILLI;
		val = val * ad4630_gains_frac[i][1] * MILLI;
		val = val / ad4630_gains_frac[i][0];

		tmp2 = no_os_shift_right((int64_t)val * MICRO, val2);
		tmp0 = (int)no_os_div_s64_rem(tmp2, NANO, &tmp1);
		dev->scale_table[i][0] = tmp0; /* Integer part */
		dev->scale_table[i][1] = abs(tmp1); /* Fractional part */
	}
}

void ad463x_reset (struct ad463x_dev *dev)
{
	/** Perform Hardware Reset of AD463x */
	// release RESET
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_START_VECTOR); // SET CONFIG MODE for CNV
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));

	usleep (1);

	// assert RESET
	rp_spmc_module_config_uint32 (MODULE_SETUP, SPMC_AD463X_CONFIG_MODE_RESET, MODULE_START_VECTOR); // SET CONFIG MODE for CNV
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));

	usleep (1);
	
	// release RESET
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_START_VECTOR); // SET CONFIG MODE for CNV
	rp_spmc_module_config_uint32 (MODULE_SETUP, 0,             MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_WR_DATA));
	rp_spmc_module_config_uint32 (MODULE_SETUP, dev->spi_clock_divider, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_DECII));
	rp_spmc_module_config_uint32 (SPMC_AD463X_CONTROL_REG, 24, MODULE_SETUP_VECTOR(SPMC_AD463X_CONFIG_N_BITS));
}


/**
 * @brief Initialize the device.
 * @param [out] device - The device structure.
 * @param [in] init_param - The structure that contains the device initial
 * 		parameters.
 * @return 0 in case of success, -1 otherwise.
 */
int32_t ad463x_init(struct ad463x_dev **device,
		    struct ad463x_init_param *init_param)
{
	struct ad463x_dev *dev;
	int32_t ret;
	uint8_t data = 0;
	uint8_t data2 = 0;
	uint8_t pat[4];
	uint8_t sample_width;

	if (!init_param || !device)
		return -1;
#if 0
	if (init_param->clock_mode == AD463X_SPI_COMPATIBLE_MODE &&
	    init_param->data_rate == AD463X_DDR_MODE) {
		pr_err("DDR_MODE not available when clock mode is SPI_COMPATIBLE\n");
		return -1;
	}
#endif
	
	fprintf(stderr,"AD463x_init -- setting up device\n");

	//dev = (struct ad463x_dev *)no_os_calloc(1, sizeof(*dev));
	dev = (struct ad463x_dev *)calloc(1, sizeof(*dev));
	if (!dev)
		return -ENOMEM;

	dev->vref = init_param->vref;
	dev->pgia_idx = 0;
	dev->offload_init_param = init_param->offload_init_param;
	dev->spi_dma_enable = init_param->spi_dma_enable;
	dev->offload_enable = init_param->offload_enable;
	dev->reg_access_speed = init_param->reg_access_speed;
	dev->reg_data_width = init_param->reg_data_width;
	dev->output_mode = init_param->output_mode;
	dev->lane_mode = init_param->lane_mode;
	dev->clock_mode = init_param->clock_mode;
	dev->data_rate = init_param->data_rate;
	dev->device_id = init_param->device_id;
	dev->dcache_invalidate_range = init_param->dcache_invalidate_range;
	dev->spi_clock_divider = init_param->spi_clock_divider;

	fprintf(stderr,"AD463x_init -- performing reset\n");
	ad463x_reset (dev);

#if 0
	/** Perform Hardware Reset and configure pins */
	ret = ad463x_init_gpio(dev, init_param);
	if (ret != 0)
		goto error_dev;

	if (dev->offload_enable) {
		ret = axi_clkgen_init(&dev->clkgen, init_param->clkgen_init);
		if (ret != 0) {
			pr_err("error: %s: axi_clkgen_init() failed\n",
			       init_param->clkgen_init->name);
			goto error_gpio;
		}

		ret = axi_clkgen_set_rate(dev->clkgen, init_param->axi_clkgen_rate);
		if (ret != 0) {
			pr_err("error: %s: axi_clkgen_set_rate() failed\n",
			       init_param->clkgen_init->name);
			goto error_clkgen;
		}
	}

	ret = no_os_spi_init(&dev->spi_desc, init_param->spi_init);
	if (ret != 0)
		goto error_clkgen;

#endif
	
	if (dev->output_mode > AD463X_16_DIFF_8_COM)
		sample_width = 32;
	else
		sample_width = 24;

	switch (dev->lane_mode) {
	case AD463X_ONE_LANE_PER_CH:
		dev->capture_data_width = sample_width;
		break;

	case AD463X_TWO_LANES_PER_CH:
		dev->capture_data_width = sample_width / 2;
		break;

	case AD463X_FOUR_LANES_PER_CH:
		dev->capture_data_width = sample_width / 4;
		break;

	case AD463X_SHARED_TWO_CH:
		dev->capture_data_width = sample_width * 2;
		break;
	default:
		goto error_spi;
		break;
	}

	switch (dev->output_mode) {
	case AD463X_24_DIFF:
		dev->real_bits_precision = 24;
		break;

	case AD463X_16_DIFF_8_COM:
		dev->real_bits_precision = 16;
		break;

	case AD463X_24_DIFF_8_COM:
		dev->real_bits_precision = 24;
		break;

	case AD463X_30_AVERAGED_DIFF:
		dev->real_bits_precision = 30;
		break;
	case AD463X_32_PATTERN:
		dev->real_bits_precision = 32;
		break;
	default:
		goto error_spi;
		break;
	}
	if (dev->data_rate == AD463X_DDR_MODE)
		dev->capture_data_width /= 2;

	dev->read_bytes_no = dev->capture_data_width / 8;

	if (dev->device_id == ID_ADAQ4224) {
		dev->has_pgia = true;
		ad463x_fill_scale_tbl(dev);
	} else {
		dev->has_pgia = false;
	}

#if 0
	if (dev->offload_enable) {
		ret = spi_engine_set_transfer_width(dev->spi_desc, dev->reg_data_width);
		if (ret != 0)
			goto error_spi;

		spi_engine_set_speed(dev->spi_desc, dev->reg_access_speed);
	}
#endif

	ad463x_enter_config_mode (dev); // added
	
	fprintf(stderr,"AD463x READ CONFIG_TIMING\n");
	ret = ad463x_spi_reg_read(dev, AD463X_CONFIG_TIMING, &data);
	if (ret != 0)
		goto error_spi;

	fprintf(stderr,"AD463x WRITE TEST TO SCRATCH PAD\n");
	ret = ad463x_spi_reg_write(dev, AD463X_REG_SCRATCH_PAD, AD463x_TEST_DATA);
	if (ret != 0)
		goto error_spi;

	ret = ad463x_spi_reg_read(dev, AD463X_REG_SCRATCH_PAD, &data, 1);
	if (ret != 0)
		goto error_spi;

	if (data != AD463x_TEST_DATA) {
	  // pr_err("Test Data read failed.\n");
	  fprintf(stderr,"AD463x Test Data Read: fail: %08x expected: %08x\n", data, AD463x_TEST_DATA);
	  goto error_spi;
	}
	else
	  fprintf(stderr,"AD463x Test Data Read: pass.\n");


	ret = ad463x_spi_reg_read(dev, AD463X_REG_CHIP_TYPE, &data);
	fprintf(stderr,"AD463x AD463X_REG_CHIP_TYPE: %02x\n", data);

	ret = ad463x_spi_reg_read(dev, AD463X_REG_PRODUCT_ID_L, &data);
	ret = ad463x_spi_reg_read(dev, AD463X_REG_PRODUCT_ID_H, &data2);
	fprintf(stderr,"AD463x AD463X_REG_CHIP_TYPE_L,H: %02x %02x\n", data, data2);

	ret = ad463x_spi_reg_read(dev, AD463X_REG_CHIP_GRADE, &data);
	fprintf(stderr,"AD463x AD463X_REG_CHIP_GRADE: %02x, Rev: %d, Grade: %s\n", data, data&7, (data>>3)&2? "AD4632-24":"AD4630-24");

	ret = ad463x_spi_reg_read(dev, AD463X_REG_VENDOR_L, &data);
	ret = ad463x_spi_reg_read(dev, AD463X_REG_VENDOR_H, &data2);
	fprintf(stderr,"AD463x AD463X_REG_VENDOR_L,H: %02x %02x\n", data, data2);

	ret = ad463x_spi_reg_read(dev, AD463X_REG_AVG, &data);
	fprintf(stderr,"AD463x AD463X_REG_AVG: %02x\n", data);

	ret = ad463x_spi_reg_read(dev, AD463X_REG_STREAM_MODE, &data);
	fprintf(stderr,"AD463x AD463X_REG_STREAM_MODE: %02x\n", data);
	
	ret = ad463x_spi_reg_read(dev, AD463X_REG_PAT0, &pat[0]);
	ret = ad463x_spi_reg_read(dev, AD463X_REG_PAT1, &pat[1]);
	ret = ad463x_spi_reg_read(dev, AD463X_REG_PAT2, &pat[2]);
	ret = ad463x_spi_reg_read(dev, AD463X_REG_PAT3, &pat[3]);
	fprintf(stderr,"AD463x AD463X_REG_TEST_PAT: %02x %02x %02x %02x\n", pat[3], pat[2], pat[1], pat[0]);

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_LANE_MODE_MSK, dev->lane_mode);
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_CLK_MODE_MSK, dev->clock_mode);
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_DDR_MODE_MSK, dev->data_rate);
	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_OUT_DATA_MODE_MSK, AD463X_32_PATTERN);
	fprintf(stderr,"AD463x AD463X_REG_TEST_PAT ENABLED NOW\n");

	ad463x_exit_reg_cfg_mode(dev); // exit CFG mode to do test conversion reads

	fprintf(stderr,"AD463x 4 TEST PAT READINGS, 32BIT mode:\n");
	{
	  uint32_t ch0, ch1;
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	  ad463x_read_single_sample (dev, &ch0, &ch1, 0, 1, 32);
	}
	
	// ***
	ad463x_enter_config_mode (dev);
	
	fprintf(stderr,"AD463x CONFIGURING MODES FOR NORMAL OPERATION\n");

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_LANE_MODE_MSK,
					  dev->lane_mode);
	if (ret != 0)
		return ret;

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_CLK_MODE_MSK,
					  dev->clock_mode);
	if (ret != 0)
		return ret;

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES, AD463X_DDR_MODE_MSK,
					  dev->data_rate);
	if (ret != 0)
		return ret;

	ret = ad463x_spi_reg_write_masked(dev, AD463X_REG_MODES,
					  AD463X_OUT_DATA_MODE_MSK, dev->output_mode);
	if (ret != 0)
		return ret;

#if 0
	if (dev->spi_dma_enable || dev->offload_enable) {
		ret = no_os_pwm_init(&dev->trigger_pwm_desc,
				     init_param->trigger_pwm_init);
		if (ret != 0)
			goto error_spi;
	}
#endif
	fprintf(stderr,"** AD463x CONFIGURING COMPLETED **\n");

	fprintf(stderr,"** AD463x TEST READS **\n");
        for (int i=0; i<10; ++i){
                uint32_t ch0, ch1;
                ad463x_read_single_sample (dev, &ch0, &ch1, 1, 0);
                fprintf (stderr, "%03d: %10d  %10d  \t %10g mV \t %10g mV\n", i, ch0, ch1, 2*5000.*((int32_t)ch0)/(1<<23), 2*5000.0*((int32_t)ch1)/(1<<23));
        }

	
	*device = dev;

	return ret;

error_spi:
	fprintf(stderr,"AD463x SPI ERROR\n");
	//no_os_spi_remove(dev->spi_desc);
	*device = dev;
	return ret;
error_clkgen:
	//fprintf(stderr,"AD463x CLKGEN ERROR\n");
	//if (dev->offload_enable)
	//	axi_clkgen_remove(dev->clkgen);
error_gpio:
	//fprintf(stderr,"AD463x GPIO ERROR\n");
	//no_os_gpio_remove(dev->gpio_resetn);
	//no_os_gpio_remove(dev->gpio_cnv);
	//no_os_gpio_remove(dev->gpio_pgia_a0);
	//no_os_gpio_remove(dev->gpio_pgia_a1);
error_dev:
	fprintf(stderr,"AD463x DEV ERROR\n");
	//no_os_free(dev);
	free(dev);

	return -1;
}

#if 0
/**
 * @brief Calculate the PGIA gain.
 * @param gain_int - Interger part of the gain.
 * @param gain_fract - Fractional part of the gain.
 * @param vref - Reference Voltage.
 * @param precision - Precision  value shifter.
 * @param gain_idx - Index of gain resulting from the calculation.
 * @return Zero in case of success, negative number otherwise
 */
int32_t ad463x_calc_pgia_gain(int32_t gain_int, int32_t gain_fract,
			      int32_t vref,
			      int32_t precision,
			      enum ad463x_pgia_gain *gain_idx)
{
	uint64_t gain_nano, tmp;

	if (!gain_idx)
		return -EINVAL;

	gain_nano = (uint64_t)gain_int * NANO + gain_fract;
	if (gain_nano > ADAQ4224_GAIN_MAX_NANO)
		return -EINVAL;
	tmp = gain_nano << precision;
	tmp = NO_OS_DIV_ROUND_CLOSEST_ULL(tmp, NANO);
	gain_nano = NO_OS_DIV_ROUND_CLOSEST_ULL(vref * 2, tmp);
	*gain_idx = no_os_find_closest((int)gain_nano, // !!!
				       ad463x_gains,
				       NO_OS_ARRAY_SIZE(ad463x_gains));
	return 0;
}

#endif

/**
 * @brief Set the PGIA gain.
 * @param dev - Pointer to the device handler.
 * @param gain_idx - Gain control index.
 * @return 0 in case of success, -1 otherwise
 */
int32_t ad463x_set_pgia_gain(struct ad463x_dev *dev,
			     enum ad463x_pgia_gain gain_idx)
{
	int32_t ret;

	if (!dev)
		return -EINVAL;
	/** Check if gain available in the ADC */
	if (!dev->has_pgia)
		return -EINVAL;
	/** Check if gain gain_idx is in the valid range */
	if (gain_idx > 3)
		return -EINVAL;

#if 0
	
	/** update gain index value in the device handler */
	dev->pgia_idx = gain_idx;
	/** Set A0 and A1 pins according to gain index value */
	ret = no_os_gpio_set_value(dev->gpio_pgia_a0, no_os_field_get(NO_OS_BIT(0),
				   gain_idx));
	if (ret != 0)
		return ret;
	return no_os_gpio_set_value(dev->gpio_pgia_a1, no_os_field_get(NO_OS_BIT(1),
				    gain_idx));

#endif
}

/**
 * @brief Free the memory allocated by ad463x_init().
 * @param [in] dev - Pointer to the device handler.
 * @return 0 in case of success, -1 otherwise
 */
int32_t ad463x_remove(struct ad463x_dev *dev)
{
	int32_t ret;

	if (!dev)
		return -1;
#if 0
	ret = no_os_pwm_remove(dev->trigger_pwm_desc);
	if (ret != 0)
		return ret;

	ret = no_os_spi_remove(dev->spi_desc);
	if (ret != 0)
		return ret;

	ret = no_os_gpio_remove(dev->gpio_resetn);
	if (ret != 0)
		return ret;

	ret = no_os_gpio_remove(dev->gpio_cnv);
	if (ret != 0)
		return ret;

	ret = no_os_gpio_remove(dev->gpio_pgia_a0);
	if (ret != 0)
		return ret;

	ret = no_os_gpio_remove(dev->gpio_pgia_a1);
	if (ret != 0)
		return ret;

	if (dev->offload_enable) {
		ret = axi_clkgen_remove(dev->clkgen);
		if (ret != 0)
			return ret;
	}
#endif
	//no_os_free(dev);
	free(dev);

	return ret;
}
