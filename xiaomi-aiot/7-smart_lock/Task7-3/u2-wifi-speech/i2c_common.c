#include <errno.h>

#include "i2c_common.h"  
 
int I2c_open(char *devname)
{
   return open(devname, O_RDWR);
}

void I2c_close(int fd)
{
	close(fd);
}

__s32 i2c_smbus_access(int file, char read_write, unsigned char command,
		       int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;
	__s32 err;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;

	err = ioctl(file, I2C_SMBUS, &args);
	if (err == -1)
		err = -errno;
	return err;
}


__s32 i2c_smbus_write_quick(int file, unsigned char value)
{
	return i2c_smbus_access(file, value, 0, I2C_SMBUS_QUICK, NULL);
}

__s32 i2c_smbus_read_byte(int file)
{
	union i2c_smbus_data data;
	int err;

	err = i2c_smbus_access(file, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data);
	if (err < 0)
		return err;

	return 0x0FF & data.byte;
}

/*
 * 功能			: i2c地址扫描
 * 输入参数		: int fd					: 设备文件描述符
 *				: unsigned char *addrArray	: 要扫描的i2c地址列表
 *				: int num					: 要扫描的i2c地址数量
 * 输出参数		: unsigned char *addr		: 总线上存在设备的i2c地址
 * 返回值		: -1						: 总线上不存在地址列表里的地址
 *				: >=0						: 扫描到了一个i2c地址
 */
int I2c_detect(int fd, unsigned char *addrArray, int num, unsigned char *addr)
{
	int		ret = -1;

	for(int i=0; i<num; i++)
	{
		if (ioctl(fd, I2C_SLAVE_FORCE, addrArray[i]) < 0)
		{
			continue;
		}
#if 0
		ret = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);
#else
		ret = i2c_smbus_read_byte(fd);
#endif
		if (ret >= 0)
		{
			printf("I2c addr 0X%02X is detected.\n",addrArray[i]);
			*addr = addrArray[i];
			break;
		}
	}

	return ret;
}

/*
 * 功能			: i2c写命令
 */
int I2c_write_cmd(int fd, unsigned char addr, unsigned char reg)
{
 
    unsigned char				outbuf[1];
    struct i2c_rdwr_ioctl_data	packets;
    struct i2c_msg				messages[1];
 
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;
 
    /* The first byte indicates which register we'll write */
    outbuf[0] = reg;
  
    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(fd, I2C_RDWR, &packets) < 0) {
        perror("Unable to send data");
        return -1;
    }
 
    return 0;
}

/*
 * 功能			: i2c写单个寄存器（8bits寄存器地址）
 */
int I2c_write_reg(int fd, unsigned char addr, unsigned char reg, unsigned char value)
{
    unsigned char				outbuf[2];
    struct i2c_rdwr_ioctl_data	packets;
    struct i2c_msg				messages[1];
 
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;
 
    /* The first byte indicates which register we'll write */
    outbuf[0] = reg;
 
    /* 
     * The second byte indicates the value to write.  Note that for many
     * devices, we can write multiple, sequential registers at once by
     * simply making outbuf bigger.
     */
    outbuf[1] = value;
 
    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(fd, I2C_RDWR, &packets) < 0) {
        perror("Unable to send data");
        return -1;
    }
 
    return 0;
}

/*
 * 功能			: i2c连续写单个寄存器一系列值（8bits寄存器地址）
 */
int I2c_write_reg_datas(int fd, unsigned char addr, unsigned char reg, unsigned char *value, int num)
{
    unsigned char				outbuf[2];
    struct i2c_rdwr_ioctl_data	packets;
    struct i2c_msg				messages[1];
 
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;
 
    outbuf[0] = reg;
	packets.nmsgs = 1;
 
	for(int i=0; i<num; i++)
	{
		outbuf[1] = value[i];
		packets.msgs  = messages;

		if(ioctl(fd, I2C_RDWR, &packets) < 0) {
			perror("Unable to send data");
			return -1;
		}
	}
 
    return 0;
}

/*
 * 功能			: i2c读连续寄存器（8bits寄存器地址）
 */
int I2c_read_regs(int fd, unsigned char addr, unsigned char reg, unsigned char *val, int num)
{
    struct i2c_rdwr_ioctl_data	packets;
    struct i2c_msg				messages[2];
 
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = 1;
	messages[0].buf   = &reg;
 
    messages[1].addr  = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = num;
    messages[1].buf   = val;

	packets.nmsgs     = 2;
	packets.msgs      = messages;

	ioctl(fd, I2C_TIMEOUT, 2);
	ioctl(fd, I2C_RETRIES, 1);

	if(ioctl(fd, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return -1;
	}

	return 0;
}

/*
 * 功能			: i2c读多个连续寄存器（16bits寄存器地址）
 */
int I2c_read_regs_addr16(int fd, unsigned char addr, unsigned short reg, unsigned char *val, int num)
{
    struct i2c_rdwr_ioctl_data	packets;
    struct i2c_msg				messages[2];
	unsigned char				regAddr[2];

	regAddr[0] = (reg >> 8) & 0xFF;;
	regAddr[1] = reg & 0xFF;

    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = 2;
	messages[0].buf   = regAddr;
 
    messages[1].addr  = addr;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = num;
    messages[1].buf   = val;

	packets.nmsgs     = 2;
	packets.msgs      = messages;

	ioctl(fd, I2C_TIMEOUT, 2);
	ioctl(fd, I2C_RETRIES, 1);

	if(ioctl(fd, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return -1;
	}

	return 0;
}
