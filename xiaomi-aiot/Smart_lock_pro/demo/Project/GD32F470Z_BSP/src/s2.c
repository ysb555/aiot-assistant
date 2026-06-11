#include "i2c.h"
#include "s2.h"

/*********************************************************************************************
函数名:      delay_s2_ms
功能:        s2子板延时函数
入口参数:    count
出口参数:    无
返回值：     无
作者：       ZZZ
日期:        2023/4/1
**********************************************************************************************/
void delay_s2_ms(uint16_t count)
{
	uint16_t i;
	uint32_t decount;
	for(i=0;i<count;i++)
	{
		for(decount=0;decount<50000;decount++)
		{
		}	
	}	 
}

/*********************************************************************************************
函数名:      s2_sht3x_softreset
功能:        软件复位sht3x
入口参数:    i2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     无
作者：       ZZZ
日期:        2023/4/1
**********************************************************************************************/
void s2_sht3x_softreset(uint32_t i2c_periph,uint8_t i2c_addr)
{
     i2c_delay_byte_write(i2c_periph,i2c_addr,0x30,0xA2);
}



/*********************************************************************************************
函数名:      s2_init_bh1750
功能:        初始化bh1759
入口参数:    i2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     无
作者：       ZZZ
日期:        2023/4/1
**********************************************************************************************/
void s2_init_bh1750(uint32_t i2c_periph,uint8_t i2c_addr)
{
     i2c_cmd_write(i2c_periph,i2c_addr,0x01);
}


/*********************************************************************************************
函数名:      s2_icm20608_gyro_scaleget
功能:        获取陀螺仪的分辨率
入口参数:    i2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     获取到的分辨率
作者：       ZZZ
日期:        2023/4/3
**********************************************************************************************/
float s2_icm20608_gyro_scaleget(uint32_t i2c_periph,uint8_t i2c_addr)
{
	uint8_t data;
	float gyroscale;

	i2c_delay_read(i2c_periph,i2c_addr,ICM20_GYRO_CONFIG,&data,1);	

	data = (data >> 3) & 0X3;//读取量程寄存器值
	switch(data) 
	{
		case  0: 				//±250dps
			gyroscale = 131;	//分辨率gyroscale = ADC数据（65536）/量程（250*2）=131
			break;
		case 1:					//±500dps
			gyroscale = 65.5;	//分辨率gyroscale = ADC数据（65536）/量程（500*2）=65.5
			break;
		case 2:					//±1000dps
			gyroscale = 32.8;	//分辨率gyroscale = ADC数据（65536）/量程（1000*2）=32.8
			break;
		case 3:					//±2000dps
			gyroscale = 16.4;	//分辨率gyroscale = ADC数据（65536）/量程（2000*2）=16.4
			break;
	}
	return gyroscale;
}




/*********************************************************************************************
函数名:      s2_icm20608_accel_scaleget
功能:        获取加速度计的分辨率
入口参数:    i2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     获取到的分辨率
作者：       ZZZ
日期:        2023/4/3
**********************************************************************************************/
uint16_t s2_icm20608_accel_scaleget(uint32_t i2c_periph,uint8_t i2c_addr)
{
	uint8_t  data;
	uint16_t accelscale;
	
	i2c_delay_read(i2c_periph,i2c_addr,ICM20_ACCEL_CONFIG,&data,1);
	data = (data >> 3) & 0X3;
	switch(data) 
	{
			case 0: 
				accelscale = 16384;		//分辨率accelscale = ADC数据（65536）/量程（2*2）= 16384
				break;
			case 1:
				accelscale = 8192;		//分辨率accelscale = ADC数据（65536）/量程（4*2）= 8192
				break;
			case 2:
				accelscale = 4096;		//分辨率accelscale = ADC数据（65536）/量程（8*2）= 4096
				break;
			case 3:
				accelscale = 2048;		//分辨率accelscale = ADC数据（65536）/量程（16*2）= 2048
				break;
	}
	return accelscale;
}



/*********************************************************************************************
函数名:      s2_init_icm20608
功能:        icm20608
入口参数:    i2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     无
作者：       ZZZ
日期:        2023/4/3
**********************************************************************************************/
void s2_init_icm20608(uint32_t i2c_periph,uint8_t i2c_addr)
{

		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_PWR_MGMT_1, 0x80);		  //复位，复位后为0x40,睡眠模式 			
		 delay_s2_ms(100);
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_PWR_MGMT_1, 0x01);		  //关闭睡眠，自动选择时钟 					
		 delay_s2_ms(100);
		
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_SMPLRT_DIV, 0x00); 	    //输出速率是内部采样率					
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_GYRO_CONFIG, 0x18); 	  //陀螺仪±2000dps量程 				
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_ACCEL_CONFIG, 0x18); 	  //加速度计±16G量程 					
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_CONFIG, 0x04); 		      //陀螺仪低通滤波BW=20Hz 				
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_ACCEL_CONFIG2, 0x04); 	//加速度计低通滤波BW=21.2Hz 			
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_PWR_MGMT_2, 0x00); 	    //打开加速度计和陀螺仪所有轴 				
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_LP_MODE_CFG, 0x00); 	  //关闭低功耗 						
		 i2c_delay_byte_write(i2c_periph,i2c_addr,ICM20_FIFO_EN, 0x00);		      //关闭FIFO						

/*	   
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x6B,0x80);
		delay_ms(100);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x6B,0x00); 
	   delay_ms(100);
	
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x6A,0x01);  
	   delay_ms(100);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x6A,0x00);  
	   delay_ms(100);
	
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x1B,0x18);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x1C,0x18);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x19,0x00);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x1A,0x04);
	
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x38,0x00);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x23,0x00);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x37,0x80);
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x6b,0x01);
	
	   i2c_delay_byte_write(i2c_periph,i2c_addr,0x20,0x7F);
     i2c_delay_byte_write(i2c_periph,i2c_addr,0x21,0x7F);
     i2c_delay_byte_write(i2c_periph,i2c_addr,0x22,0x7F);	

     i2c_delay_byte_write(i2c_periph,i2c_addr,0x69,0x80);
     i2c_delay_byte_write(i2c_periph,i2c_addr,0x38,0xE0);
     i2c_delay_byte_write(i2c_periph,i2c_addr,0x6C,0x00);
*/	   
	   
}
/*********************************************************************************************
计算crc校验值
**********************************************************************************************/
uint8_t s2_sht3x_crc_cal(uint16_t DAT)
{
		uint8_t i,t,temp;
		uint8_t CRC_BYTE;

		CRC_BYTE = 0xFF;
		temp = (DAT>>8) & 0xFF;

		for(t = 0; t < 2; t++)
		{
				CRC_BYTE ^= temp;
				for(i = 0;i < 8;i ++)
				{
						if(CRC_BYTE & 0x80)
						{
							  CRC_BYTE <<= 1;
							  CRC_BYTE ^= 0x31;
						}
						else
						{
							  CRC_BYTE <<= 1;
						}
				}

				if(t == 0)
				{
					  temp = DAT & 0xFF;
				}
		}

	  return CRC_BYTE;
}




/*********************************************************************************************************
函数名:     s2_init
入口参数:   i2c初始地址 address 
出口参数:   无 
返回值:     i2c_addr_def定义结构体
作者:       zzz
日期:       2023/4/1
调用描述:   得到s2 i2c地址,若器件不存在则结构体flag值为0,若存在则初始化芯片置结构体flag值为1
**********************************************************************************************************/
i2c_addr_def s2_init(uint8_t address)
{
	i2c_addr_def e_addess;
	uint8_t i;     

	e_addess.flag = 0; 
	if( (address == TH_ADDRESS_S2) || (address == AX_ADDRESS_S2) )
	{
		for(i=0;i<2;i++)
		{
			if(i2c_addr_poll(I2C0,address+i*2))
			{
				e_addess.periph = I2C0;
				e_addess.addr = address+i*2;
				e_addess.flag = 1;			
				break;
			} 
		}
		if(e_addess.flag != 1)
		{			
			for(i=0;i<4;i++)
			{
				if(i2c_addr_poll(I2C1,address+i*2))
				{
					e_addess.periph = I2C1;
					e_addess.addr = address+i*2;
					e_addess.flag = 1;
					break;
				}	
			}
		}
		if(e_addess.flag)
		{
			if( (e_addess.addr == TH_ADDRESS_S2) || (e_addess.addr == (TH_ADDRESS_S2+2)) )
			{
				s2_sht3x_softreset(e_addess.periph,e_addess.addr);
			}		
			else if( (e_addess.addr == AX_ADDRESS_S2) || (e_addess.addr == (AX_ADDRESS_S2+2)) )   
			{
				s2_init_icm20608(e_addess.periph,e_addess.addr);
			}								
		}					 
	}
	else if(address == S1_ADDRESS_S2)
	{
		e_addess = get_board_address(S1_ADDRESS_S2);  
		if(e_addess.flag != 1)	
		{
			e_addess = get_board_address(S2_ADDRESS_S2);	
		}			
		if(e_addess.flag)	
		{
			s2_init_bh1750(e_addess.periph,e_addess.addr);
		}					 
	}

	return e_addess;		 
}




/*********************************************************************************************************
函数名:     s2_all_init
入口参数:   s2_address结构体指针  s2_th_addr:sht35温湿度传感器地址  s2_ss_add:bh1750传感器地址 s2_ax_addr:6轴传感器地址
出口参数:   s2_address结构体指针 
返回值:     无
作者:       zzz
日期:       2023/4/1
调用描述:   查找并初始化所有S2子板,并得到相应的地址放在结构体指针s2_address
**********************************************************************************************************/
void s2_all_init(s2_addr_def *s2_address,uint8_t s2_th_addr,uint8_t s2_ss_addr,uint8_t s2_ax_addr)
{
	   s2_address->th_addr = s2_init(s2_th_addr);
	   s2_address->ss_addr = s2_init(s2_ss_addr);
	   s2_address->ax_addr = s2_init(s2_ax_addr);
}	



/*********************************************************************************************
函数名:      s2_read_sht3x
功能:        读取温度和湿度  
入口参数:    无
出口参数:    无
返回值：     s2_para结构体
作者：       ZZZ
日期:        2023/4/1
调用描述:    读取sht35的温度湿度值
**********************************************************************************************/
s2_para s2_read_sht3x(uint32_t i2c_periph,uint8_t i2c_addr)
{
	uint8_t  th_value[6];
	s2_para  sht_para;
	uint16_t tmp;

	i2c_delay_byte_write(i2c_periph,i2c_addr,0x2C,0x0D);
	delay_s2_ms(100);
	i2c_direct_read(i2c_periph,i2c_addr,th_value,6);

	tmp = (th_value[0]<<8) + th_value[1];
	if(s2_sht3x_crc_cal(tmp) == th_value[2])
	{
		sht_para.temperature = (float)tmp*175/(65536-1)-45;
	}

	tmp = (th_value[3]<<8) + th_value[4];
	if(s2_sht3x_crc_cal(tmp) == th_value[5])
	{
		sht_para.humidity = (float)tmp*100/(65536-1);
	}

	return sht_para;
}



/*********************************************************************************************
函数名:      s2_read_bh1750_value
功能:        读取光照度  
入口参数:    2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     float ss单精度值
作者：       ZZZ
日期:        2023/4/1
调用描述:    读取bh1750的光照强度值
**********************************************************************************************/
float s2_read_bh1750_value(uint32_t i2c_periph,uint8_t i2c_addr)
{	
	uint16_t tmp;
	uint8_t ss_value[2];
	float ss;

	i2c_cmd_write(i2c_periph,i2c_addr,0x01);
	i2c_cmd_write(i2c_periph,i2c_addr,0x10);
	delay_s2_ms(200);
	i2c_direct_read(i2c_periph,i2c_addr,ss_value,2);
	tmp = (ss_value[0]<<8) + ss_value[1];
	ss = (float)tmp*10/12;

	return ss;
}



/*********************************************************************************************
函数名:      s2_read_icm20608_value
功能:        读取6轴数据  
入口参数:    2c_periph:i2c口   i2c_addr:i2c地址
出口参数:    无
返回值：     icm_value:ax_para结构体数据
作者：       ZZZ
日期:        2023/4/3
调用描述:    读取icm20608的x,y,z轴的角速度和加速度值并同时读取温度值
**********************************************************************************************/
ax_para s2_read_icm20608_value(uint32_t i2c_periph,uint8_t i2c_addr)
{
	ax_para icm_value;
	float gyroscale;
	uint16_t accescale;
	uint8_t data[14];

	int16_t gyro_x_adc;		
	int16_t gyro_y_adc;		
	int16_t gyro_z_adc;		
	int16_t accel_x_adc;		
	int16_t accel_y_adc;		
	int16_t accel_z_adc;		
	int16_t temp_adc;		

	gyroscale = s2_icm20608_gyro_scaleget(i2c_periph,i2c_addr);
	accescale = s2_icm20608_accel_scaleget(i2c_periph,i2c_addr);

	i2c_delay_read(i2c_periph,i2c_addr,ICM20_ACCEL_XOUT_H,data,14);

	accel_x_adc = (data[0] << 8) | data[1]; 
	accel_y_adc = (data[2] << 8) | data[3]; 
	accel_z_adc = (data[4] << 8) | data[5]; 
	temp_adc    = (data[6] << 8) | data[7]; 
	gyro_x_adc  = (data[8] << 8) | data[9]; 
	gyro_y_adc  = (data[10] << 8) | data[11];
	gyro_z_adc  = (data[12] << 8) | data[13];

	icm_value.gyro_x_act = (float)(gyro_x_adc) / gyroscale;
	icm_value.gyro_y_act = (float)(gyro_y_adc) / gyroscale;
	icm_value.gyro_z_act = (float)(gyro_z_adc) / gyroscale;

	icm_value.accel_x_act = (float)(accel_x_adc) / accescale;
	icm_value.accel_y_act = (float)(accel_y_adc) / accescale;
	icm_value.accel_z_act = (float)(accel_z_adc) / accescale;

	icm_value.temp_act = (float)((temp_adc - 25 )*10 / 3268) + 25;
	return icm_value;
}



