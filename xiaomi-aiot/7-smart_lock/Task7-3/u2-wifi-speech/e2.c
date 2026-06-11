#include "e2.h"
unsigned char e2_i2c_addr_array[] = {0x64,0x65,0x66,0x67};

int I2c_e2_detect(int fd, unsigned char *addr)
{
    return I2c_detect(fd, e2_i2c_addr_array, sizeof(e2_i2c_addr_array), addr);
}
void pc9685_motor_control(unsigned int  i2c_periph, unsigned char i2c_addr,unsigned char num)
{
    I2c_write_reg(i2c_periph,i2c_addr,PCA9685_MODE1,0x0);
    switch (num)
    {
    case 0:
        setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0x1000);
        break;
     case 1:
        setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0x0665);
        break;
         case 2:
        setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0xb32);
        break;
         case 3:
        setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0x0fff);
        break;
    default:
        setPWM(i2c_periph,i2c_addr,(unsigned char)0,0x0199,0x1000);
        break;
    }
}
int jiance_e2(FUNC_PARAM *param)
{
//e2板在线状态检测
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
			
			if(I2c_e2_detect(i2c_fd, &i2c_addr) >= 0)
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