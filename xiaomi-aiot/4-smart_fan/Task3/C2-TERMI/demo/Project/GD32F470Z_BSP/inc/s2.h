#ifndef S2_H
#define S2_H

#include "i2c.h"

#define  out
typedef struct
{
	 i2c_addr_def th_addr;
	 i2c_addr_def ss_addr;
	 i2c_addr_def ax_addr;
}s2_addr_def;

typedef struct
{
	  float temperature;
	  float humidity;
}s2_para;	


typedef struct 
{
		float gyro_x_act;		               //陀螺仪X轴实际值 			
		float gyro_y_act;		               //陀螺仪Y轴实际值 	
		float gyro_z_act;		               //陀螺仪Z轴实际值 			
		float accel_x_act;		               //加速度计X轴实际值 			
		float accel_y_act;		               //加速度计Y轴实际值 			
		float accel_z_act;		               //加速度计Z轴实际值 			
		float temp_act;		                   //温度实际值 				
}ax_para;





#define S1_ADDRESS_S2            	     0x46       //0x46 0xB8
#define S2_ADDRESS_S2            	     0xB8
#define	TH_ADDRESS_S2		       	     0x88       //0x88 0x8A
#define AX_ADDRESS_S2             		 0xD0       //0xD0 0xD2




//陀螺仪
#define	ICM20_SELF_TEST_X_GYRO			 0x00
#define	ICM20_SELF_TEST_Y_GYRO			 0x01
#define	ICM20_SELF_TEST_Z_GYRO			 0x02
//加速度	
#define	ICM20_SELF_TEST_X_ACCEL			 0x0D
#define	ICM20_SELF_TEST_Y_ACCEL			 0x0E
#define	ICM20_SELF_TEST_Z_ACCEL			 0x0F

//陀螺仪静态偏移 
#define	ICM20_XG_OFFS_USRH			     0x13
#define	ICM20_XG_OFFS_USRL			     0x14
#define	ICM20_YG_OFFS_USRH			     0x15
#define	ICM20_YG_OFFS_USRL			     0x16
#define	ICM20_ZG_OFFS_USRH			   	 0x17
#define	ICM20_ZG_OFFS_USRL			     0x18

#define	ICM20_SMPLRT_DIV			     0x19		//设置输出速率，输出速率计算公式如下:SAMPLE RATE=INTERNAL SAMPLE RATE(1 +SMPLRT DIV)
#define	ICM20_CONFIG				     0x1A		//设置陀螺仪低通滤波。可设置 0-7。
#define	ICM20_GYRO_CONFIG			     0x1B		//陀螺仪量程，0:±250dps; 1:±500dps; 2:±1000dps; 3:±2000dps
#define	ICM20_ACCEL_CONFIG			   	 0x1C		//加速度计量程，0:±2g; l:±4g; 2:±8g; 3:±16g
#define	ICM20_ACCEL_CONFIG2			     0x1D		//设置加速度计的低通滤波。可设置 0-7。
#define	ICM20_LP_MODE_CFG			     0x1E		//0:关闭陀螺仪的低功耗功能  l:使能陀螺仪的低功耗功能
#define	ICM20_ACCEL_WOM_THR			     0x1F		
#define	ICM20_FIFO_EN				     0x23		//FIFO 使能控制
#define	ICM20_FSYNC_INT				     0x36
#define	ICM20_INT_PIN_CFG			     0x37		//中断/旁路设置寄存器
#define	ICM20_INT_ENABLE			     0x38		//使能中断源
#define	ICM20_INT_STATUS			     0x3A

//加速度输出 
#define	ICM20_ACCEL_XOUT_H			  	 0x3B
#define	ICM20_ACCEL_XOUT_L				 0x3C
#define	ICM20_ACCEL_YOUT_H		     	 0x3D
#define	ICM20_ACCEL_YOUT_L			  	 0x3E
#define	ICM20_ACCEL_ZOUT_H			  	 0x3F
#define	ICM20_ACCEL_ZOUT_L			  	 0x40

//温度输出 
#define	ICM20_TEMP_OUT_H			     0x41
#define	ICM20_TEMP_OUT_L			     0x42

//陀螺仪输出 
#define	ICM20_GYRO_XOUT_H			     0x43
#define	ICM20_GYRO_XOUT_L			     0x44
#define	ICM20_GYRO_YOUT_H			     0x45
#define	ICM20_GYRO_YOUT_L			     0x46
#define	ICM20_GYRO_ZOUT_H			     0x47
#define	ICM20_GYRO_ZOUT_L			     0x48

#define	ICM20_SIGNAL_PATH_RESET		 	 0x68
#define	ICM20_ACCEL_INTEL_CTRL 		 	 0x69
#define	ICM20_USER_CTRL				     0x6A
#define	ICM20_PWR_MGMT_1			     0x6B
#define	ICM20_PWR_MGMT_2			     0x6C
#define	ICM20_FIFO_COUNTH			     0x72
#define	ICM20_FIFO_COUNTL			     0x73
#define	ICM20_FIFO_R_W				     0x74
#define	ICM20_WHO_AM_I 				     0x75

//加速度静态偏移 
#define	ICM20_XA_OFFSET_H			     0x77
#define	ICM20_XA_OFFSET_L			     0x78
#define	ICM20_YA_OFFSET_H			     0x7A
#define	ICM20_YA_OFFSET_L			     0x7B
#define	ICM20_ZA_OFFSET_H			     0x7D
#define	ICM20_ZA_OFFSET_L 			  	 0x7E




s2_para s2_read_sht3x(uint32_t i2c_periph,uint8_t i2c_addr);
i2c_addr_def s2_init(uint8_t address);
void s2_all_init(s2_addr_def *s2_address,uint8_t s2_th_addr,uint8_t s2_ss_addr,uint8_t s2_ax_addr);
float s2_read_bh1750_value(uint32_t i2c_periph,uint8_t i2c_addr);
ax_para s2_read_icm20608_value(uint32_t i2c_periph,uint8_t i2c_addr);




#endif
