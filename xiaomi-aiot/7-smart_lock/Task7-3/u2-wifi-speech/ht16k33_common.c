#include "ht16k33_common.h" 
 
/******************************************************************************************************************
 * 4¦Λ?????
 *****************************************************************************************************************/
//?????????/????
#define	HT16K33_STANDBY_MODE_ENABLE		0x20
#define	HT16K33_STANDBY_MODE_DISABLE	0x21
//??????/????
#define	HT16K33_DISPLAY_DISABLE			0x80
#define	HT16K33_DISPLAY_ENABLE			0x81

typedef struct ht16k33_display_info
{
	char			content;	//???????
	unsigned char	reg1_val;	//?????????????
	unsigned char	reg2_val;	//?????????????
}HT16K33_DISPLAY_INFO;

//?????????????
const HT16K33_DISPLAY_INFO ht16k33_dis[] = {{'0',0xF8,0x01},
											{'1',0x30,0x00},
											{'2',0xD8,0x02},
											{'3',0x78,0x02},
											{'4',0x30,0x03},
											{'5',0x68,0x03},
											{'6',0xE8,0x03},
											{'7',0x38,0x00},
											{'8',0xF8,0x03},
											{'9',0x78,0x03},
											{'A',0xB8,0x03},
											{'B',0xE0,0x03},
											{'C',0xC8,0x01},
											{'D',0xF0,0x02},
											{'E',0xC8,0x03},
											{'F',0x88,0x03},
											{'-',0x00,0x02}};

//?????????
#define	HT16K33_KEY_KS0					0x40
#define	HT16K33_KEY_REG_NUM				6

/******************************************************************************************************************
 * ????			: ????????????????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 *****************************************************************************************************************/
void Ht16k33_clear(int fd, unsigned char addr)
{
	I2c_write_cmd(fd, addr, HT16K33_STANDBY_MODE_DISABLE);
	I2c_write_reg(fd, addr, 0x02, 0x00);
	I2c_write_reg(fd, addr, 0x03, 0x00);
	I2c_write_reg(fd, addr, 0x04, 0x00);
	I2c_write_reg(fd, addr, 0x05, 0x00);
	I2c_write_reg(fd, addr, 0x06, 0x00);
	I2c_write_reg(fd, addr, 0x07, 0x00);
	I2c_write_reg(fd, addr, 0x08, 0x00);
	I2c_write_reg(fd, addr, 0x09, 0x00);
	I2c_write_cmd(fd, addr, HT16K33_DISPLAY_ENABLE);
}

/******************************************************************************************************************
 * ????			: ???????????????????????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 *				: int index				: ?????????????????????????0??1??2??3??
 *				: char content			: ????????????
 *****************************************************************************************************************/
int Ht16k33_led_display_bit_left(int fd, unsigned char addr, int index, char content)
{
	if(index < 0 || index > 3)
		return -1;

	unsigned char	reg_base = 0x02 + index * 2;
	unsigned char	value1 = 0x00;
	unsigned char	value2 = 0x00;
	int				len = sizeof(ht16k33_dis)/sizeof(HT16K33_DISPLAY_INFO);
	for(int i=0; i<len; i++)
	{
		if(ht16k33_dis[i].content == content)
		{
			value1 = ht16k33_dis[i].reg1_val;
			value2 = ht16k33_dis[i].reg2_val;
			break;
		}
	}
	I2c_write_reg(fd, addr, reg_base, value1);
	I2c_write_reg(fd, addr, reg_base+1, value2);
	return 0;
}
/******************************************************************************************************************
 * ????			: ??????????4????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 *				: int startidx			: ????????????????????????????0??1??2??3??
 *				: char *content			: ??????????????
 *****************************************************************************************************************/
int Ht16k33_led_display_bits(int fd, unsigned char addr, int startidx, char *content)
{
	int	len = strlen(content);
	int	endidx = startidx + len - 1;

	if(endidx < 0 || endidx > 3)
		return -1;

	I2c_write_cmd(fd, addr, HT16K33_STANDBY_MODE_DISABLE);
	for(int i = startidx; i <= endidx; i++)
	{
		Ht16k33_led_display_bit_left(fd, addr, i, content[i-startidx]);
	}
	I2c_write_cmd(fd, addr, HT16K33_DISPLAY_ENABLE);

	return 0;
}

/******************************************************************************************************************
 * ????			: ????????????????????????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 *				: int index				: ?????????????????????????0??1??2??3??
 *				: char content			: ????????????
 *****************************************************************************************************************/
int Ht16k33_led_display_bit_right(int fd, unsigned char addr, int index, char content)
{
	if(index < 0 || index > 3)
		return -1;

	unsigned char	reg_base = 0x08 - index * 2;
	unsigned char	value1 = 0x00;
	unsigned char	value2 = 0x00;
	int				len = sizeof(ht16k33_dis)/sizeof(HT16K33_DISPLAY_INFO);
	for(int i=0; i<len; i++)
	{
		if(ht16k33_dis[i].content == content)
		{
			value1 = ht16k33_dis[i].reg1_val;
			value2 = ht16k33_dis[i].reg2_val;
			break;
		}
	}
	I2c_write_reg(fd, addr, reg_base, value1);
	I2c_write_reg(fd, addr, reg_base+1, value2);
	return 0;
}

/******************************************************************************************************************
 * ????			: ??????????4????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 *				: char *content			: ??????????????
 *****************************************************************************************************************/
int Ht16k33_led_display(int fd, unsigned char addr, char *content)
{
	int	len = strlen(content);

	if(len < 0 || len > 4)
		return -1;

	//???????????????
	Ht16k33_clear(fd, addr);

	I2c_write_cmd(fd, addr, HT16K33_STANDBY_MODE_DISABLE);
	for(int i = 0; i < len; i++)
	{
		Ht16k33_led_display_bit_right(fd, addr, i, content[len-1-i]);
	}
	I2c_write_cmd(fd, addr, HT16K33_DISPLAY_ENABLE);

	return 0;
}

/******************************************************************************************************************
 * ????			: ??????????
 * ???????		: int fd				: i2c?υτ?????????
 *				: unsigned char addr	: i2c???
 * ???????		: unsigned char *key	: ???????1~12??
 *****************************************************************************************************************/
int Ht16k33_key_value_get(int fd, unsigned char addr, unsigned char *key)
{
	int				ret;
	unsigned char	value[HT16K33_KEY_REG_NUM];

	ret = I2c_read_regs(fd, addr, HT16K33_KEY_KS0, value, HT16K33_KEY_REG_NUM);
	if(!ret)
	{
		if(value[0] & 0x01)
			*key = 1;
		else if(value[2] & 0x01)
			*key = 2;
		else if(value[4] & 0x01)
			*key = 3;
		else if(value[0] & 0x02)
			*key = 4;
		else if(value[2] & 0x02)
			*key = 5;
		else if(value[4] & 0x02)
			*key = 6;
		else if(value[0] & 0x04)
			*key = 7;
		else if(value[2] & 0x04)
			*key = 8;
		else if(value[4] & 0x04)
			*key = 9;
		else if(value[0] & 0x08)
			*key = 10;
		else if(value[2] & 0x08)
			*key = 11;
		else if(value[4] & 0x08)
			*key = 12;
		else
			ret = -1;
	}
	return ret;
}
