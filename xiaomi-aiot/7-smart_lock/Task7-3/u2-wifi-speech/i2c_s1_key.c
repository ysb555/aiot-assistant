#include "i2c_s1_key.h" 
 
/******************************************************************************************************************
 * 12键按键板
 *****************************************************************************************************************/
unsigned char s1_key_i2c_addr_array[] = {0x74,0x75,0x76,0x77};

/******************************************************************************************************************
 * 功能			: S1 detect
 * 输入参数		: int fd				: 设备文件描述符
 * 输出参数		: unsigned char	*addr	: 检测到的i2c地址
 *****************************************************************************************************************/
int I2c_s1_key_detect(int fd, unsigned char *addr)
{
	return I2c_detect(fd, s1_key_i2c_addr_array, sizeof(s1_key_i2c_addr_array), addr);
}

/******************************************************************************************************************
 * 功能			: S1 key初始化
 * 输入参数		: int fd				: i2c设备文件描述符
 *				: unsigned char addr	: i2c地址
 *****************************************************************************************************************/
void I2c_s1_key_init(int fd, unsigned char addr)
{
	Ht16k33_clear(fd, addr);
}

/******************************************************************************************************************
 * 功能			: S1 按键值读取
 * 输入参数		: int fd				: i2c设备文件描述符
 *				: unsigned char addr	: i2c地址
 *****************************************************************************************************************/
int I2c_s1_key_get(int fd, unsigned char addr, unsigned char *key)
{
	return Ht16k33_key_value_get(fd, addr, key);
}

int jiance_s1(FUNC_PARAM *param)
{
//s1板在线状态检测
    char				devname[16];
    int isOnline=0;
	for(int i=I2C_DEV_NO_START; i<(I2C_DEV_NO_START+I2C_DEV_NUM); i++)
	{
		unsigned char i2c_addr;
		sprintf(devname, "%s%d", I2C_DEV_NAME_COMMON, i);
		//open
		int i2c_fd = I2c_open(devname);
		if(i2c_fd > 0)
		{
			//memset(&param, 0, sizeof(param));
			//S1 key detect
			if(I2c_s1_key_detect(i2c_fd, &i2c_addr) >= 0)
			{
				isOnline = i;
				param->i2c_fd = i2c_fd;
				param->i2c_addr = i2c_addr;
                I2c_close(i2c_fd);
                return isOnline;
			}
			//close
			I2c_close(i2c_fd);
			//找到一块S1板
			if(isOnline)
				return i;
		}
	}
    return 0;
}