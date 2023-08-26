#pragma once


typedef unsigned short guint16;
typedef short gint16;


#ifdef __cplusplus
extern "C" {
#endif

  void rp_spmc_set_bias (double bias);
  void rp_spmc_set_zservo_controller (double setpoint, double cp, double ci, double upper, double lower);


#ifdef __cplusplus
}
#endif
