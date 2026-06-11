#include "s6.h"

unsigned char s6_i2c_addr_array[] = {0x2c,0x2d,0x2e,0x2f};

int I2c_s6_detect(int fd, unsigned char *addr)
{
    int ret=-1;
    unsigned char rev[2];
    for(int i =0;i<4;i++)
    {
        if(ioctl(fd,I2C_SLAVE_FORCE,s6_i2c_addr_array[i])<0)
        {
            continue;
        }
        ret= I2c_read_regs(fd,s6_i2c_addr_array[i],0xaa,rev,2);
        if(!ret)
        {
            printf("I2c addr 0x%02X is detected.\n",s6_i2c_addr_array[i]);
            *addr = s6_i2c_addr_array[i];
            return 1;
        }
    }
    return -1;
}

int jiance_s6(FUNC_PARAM *param)
{
//s6板在线状态检测
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
			
			if(I2c_s6_detect(i2c_fd, &i2c_addr) >= 0)
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
unsigned short distance(FUNC_PARAM param)
{
    unsigned char rev[2];
    unsigned short display;
    unsigned char  sts;
   sts = I2c_read_regs(param.i2c_fd, param.i2c_addr,0xaa,rev, 2);
    if(!sts)
    {
        display = (rev[0]<<8)|rev[1];
        return display;
    }
    return 0;
}