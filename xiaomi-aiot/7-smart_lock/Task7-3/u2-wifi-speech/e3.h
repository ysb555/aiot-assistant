#include "i2c_common.h"
int I2c_e3_detect(int fd, unsigned char *addr);
int jiance_e3(FUNC_PARAM *param);
void curtain_control(unsigned int  i2c_periph, unsigned char i2c_addr,unsigned char num);