#include "e1.h"


unsigned char e1_led_i2c_addr_array[] = {0x60,0x61,0x62,0x63};
unsigned char e1_lcd_i2c_addr_array[] = {0x70,0x71,0x72,0x73};


int I2c_e1_led_detect(int fd, unsigned char *addr)
{
	return I2c_detect(fd, e1_led_i2c_addr_array, sizeof(e1_led_i2c_addr_array), addr);
}
int I2c_e1_lcd_detect(int fd, unsigned char *addr)
{
	return I2c_detect(fd, e1_lcd_i2c_addr_array, sizeof(e1_lcd_i2c_addr_array), addr);
}
void e1_init(FUNC_PARAM param)
{
	pc9685_init(param.i2c_fd,param.i2c_addr);
}
void pc9685_rgb_led_control(unsigned int i2c_periph,unsigned char  i2c_addr,unsigned char num)
{	
		
		switch(num)
		{
		case 0:
			setPWM_off(i2c_periph,i2c_addr);
			break;
		case 1:
			setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0000,0x0010);
			setPWM(i2c_periph,i2c_addr,(unsigned char)1,0x0199,0x0C00);
			setPWM(i2c_periph,i2c_addr,(unsigned char)2,0x0000,0x0010);
			break;
		case 2:
			setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0x0800);
			setPWM(i2c_periph,i2c_addr,(unsigned char)1,0x0199,0x0800);
			setPWM(i2c_periph,i2c_addr,(unsigned char)2,0x0000,0x0010);
			break;		
		case 3:
			setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0000,0x0010);
			setPWM(i2c_periph,i2c_addr,(unsigned char)1,0x0000,0x0010);
			setPWM(i2c_periph,i2c_addr,(unsigned char)2,0x0199,0x0C00);
			break;  
		default:
			setPWM_off(i2c_periph,i2c_addr);
			break;
		}
}

int  jiance_e1_led(FUNC_PARAM *param)
{
//e1板在线状态检测bing zhi xing
    char				devname[16];
    int isOnline=0;
	unsigned char i2c_addr;
	for(int i=I2C_DEV_NO_START; i<(I2C_DEV_NO_START+I2C_DEV_NUM); i++)
	{
		sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, i);
		//open
		int i2c_fd = I2c_open(devname);
		if(i2c_fd > 0)
		{
			if(I2c_e1_led_detect(i2c_fd, &i2c_addr) >= 0)
			{
				isOnline = i;
				param->i2c_fd = i2c_fd;
				param->i2c_addr = i2c_addr;

                I2c_close(i2c_fd);
                return isOnline;
			}
			//close
			I2c_close(i2c_fd);

			if(isOnline)
				return i;
		}
	}
    return 0;
}
int  jiance_e1_lcd(FUNC_PARAM *param)
{
//e1板在线状态检测bing zhi xing
    char				devname[16];
    int isOnline=0;
	unsigned char i2c_addr;
	for(int i=I2C_DEV_NO_START; i<(I2C_DEV_NO_START+I2C_DEV_NUM); i++)
	{
		sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, i);
		//open
		int i2c_fd = I2c_open(devname);
		if(i2c_fd > 0)
		{

			if(I2c_e1_lcd_detect(i2c_fd, &i2c_addr) >= 0)
			{
				isOnline = i;
				param->i2c_fd = i2c_fd;
				param->i2c_addr = i2c_addr;

                I2c_close(i2c_fd);
                return isOnline;
			}
			//close
			I2c_close(i2c_fd);
			if(isOnline)
				return i;
		}
	}
    return 0;
}

int xianshi(unsigned int  i2c_periph, unsigned char i2c_addr,char* context)
{
	return Ht16k33_led_display(i2c_periph, i2c_addr, context);
}