#include "pca9685.h" 


void pc9685_motor_control(unsigned int  i2c_periph, unsigned char i2c_addr,unsigned char num);
int I2c_e2_detect(int fd, unsigned char *addr);
int jiance_e2(FUNC_PARAM *param);