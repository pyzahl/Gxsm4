#pragma once


typedef unsigned short guint16;
typedef short gint16;


#ifdef __cplusplus
extern "C" {
#endif

  uint8_t* cfg_reg_adr(int cfg_slot);
  void set_gpio_cfgreg_int32 (int cfg_slot, int value);
  void set_gpio_cfgreg_uint32 (int cfg_slot, unsigned int value);
  void set_gpio_cfgreg_int16_int16 (int cfg_slot, gint16 value_1, gint16 value_2);
  void set_gpio_cfgreg_uint16_uint16 (int cfg_slot, guint16 value_1, guint16 value_2);
  void set_gpio_cfgreg_int64 (int cfg_slot, long long value);
  void set_gpio_cfgreg_int48 (int cfg_slot, unsigned long long value);
  int32_t read_gpio_reg_int32_t (int gpio_block, int pos);
  int read_gpio_reg_int32 (int gpio_block, int pos);
  unsigned int read_gpio_reg_uint32 (int gpio_block, int pos);
  
#ifdef __cplusplus
}
#endif
