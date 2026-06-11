#include "e3.h"

unsigned char e3_i2c_addr_array[] = {0x1c,0x1d,0x1e,0x1f};

int I2c_e3_detect(int fd, unsigned char *addr)
{
    int ret=-1;
    unsigned char rev;
    for(int i =0;i<4;i++)
    {
        if(ioctl(fd,I2C_SLAVE_FORCE,e3_i2c_addr_array[i])<0)
        {
            continue;
        }
        ret= I2c_read_regs(fd,e3_i2c_addr_array[i],0x01,&rev,1);
        if(!ret)
        {
            printf("I2c addr 0x%02X is detected.\n",e3_i2c_addr_array[i]);
            *addr = e3_i2c_addr_array[i];
            return 1;
        }
    }
    return -1;
}
int jiance_e3(FUNC_PARAM *param)
{
//e3板在线状态检测
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
			
			if(I2c_e3_detect(i2c_fd, &i2c_addr) >= 0)
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
void curtain_control(unsigned int  i2c_periph, unsigned char i2c_addr,unsigned char num)
{
    I2c_write_reg(i2c_periph,i2c_addr,0x03,num);
}