#ifndef HT16K33_COMMON
#define HT16K33_COMMON
#include "i2c_common.h" 

void Ht16k33_clear(int fd, unsigned char addr);
//ÊıÂë¹Ü
int Ht16k33_led_display_bits(int fd, unsigned char addr, int startidx, char *content);
int Ht16k33_led_display(int fd, unsigned char addr, char *content);
//°´¼ü
int Ht16k33_key_value_get(int fd, unsigned char addr, unsigned char *key);

#endif
