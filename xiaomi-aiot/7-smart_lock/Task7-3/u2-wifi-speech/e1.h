#include "pca9685.h" 
#include "ht16k33_common.h"

typedef struct 
{
    /* data */
    unsigned char flag;
    unsigned int periph;
    unsigned char addr;
}i2c_addr_def;

/*typedef struct func_param
{
	int				i2c_fd;
	unsigned char	i2c_addr;
}FUNC_PARAM;
*/

// i2c_addr_def e1_rgb_led_addr;


void pc9685_rgb_led_control(unsigned int  i2c_periph, unsigned char i2c_addr,unsigned char num);
void e1_init(FUNC_PARAM param);

int I2c_e1_led_detect(int fd, unsigned char *addr);
int I2c_e1_lcd_detect(int fd, unsigned char *addr);

int jiance_e1_led(FUNC_PARAM *param);
int jiance_e1_lcd(FUNC_PARAM *param);

int xianshi(unsigned int  i2c_periph, unsigned char i2c_addr,char* context);

