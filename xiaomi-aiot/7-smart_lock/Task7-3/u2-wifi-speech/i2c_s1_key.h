#ifndef I2C_S1_KEY
#define I2C_S1_KEY

#include "ht16k33_common.h" 

int I2c_s1_key_detect(int fd, unsigned char *addr);
void I2c_s1_key_init(int fd, unsigned char addr);
int I2c_s1_key_get(int fd, unsigned char addr, unsigned char *key);
int jiance_s1(FUNC_PARAM *param);

#endif
