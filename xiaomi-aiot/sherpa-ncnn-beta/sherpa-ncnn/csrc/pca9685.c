
#include "pca9685.h"


/*!
    \brief      pc9685 test program
    \param[in]  none
    \param[out] none
    \retval     none
*/
void pc9685_init(unsigned int i2c_periph,unsigned char i2c_addr)
{
		I2c_write_reg(i2c_periph,i2c_addr,PCA9685_MODE1,0x0);
		
		setPWM_off(i2c_periph,i2c_addr);    
}

/*!
    \brief      set pc9685 output pwm
		\param[in]  num: output ch
		\param[in]  on: LEDn ON cnt,12bits
		\param[in]  on: LEDn OFF cnt,12bits
    \param[out] none
    \retval     none
*/
void setPWM_off(unsigned int i2c_periph,unsigned char i2c_addr)
{
    I2c_write_reg(i2c_periph,i2c_addr,ALLLED_ON_L,0);
   I2c_write_reg(i2c_periph,i2c_addr,ALLLED_ON_H,0);
   I2c_write_reg(i2c_periph,i2c_addr,ALLLED_OFF_L,0);
 I2c_write_reg (i2c_periph,i2c_addr,ALLLED_OFF_H,0);
}

/*!
    \brief      set pc9685 output pwm
		\param[in]  num: output ch
		\param[in]  on: LEDn ON cnt,12bits
		\param[in]  on: LEDn OFF cnt,12bits
    \param[out] none
    \retval     none
*/
void setPWM(unsigned int i2c_periph,unsigned char i2c_addr,unsigned char num, unsigned short on, unsigned short off)
{
    I2c_write_reg(i2c_periph,i2c_addr,LED0_ON_L+4*num,on);
    I2c_write_reg(i2c_periph,i2c_addr,LED0_ON_H+4*num,on>>8);
    I2c_write_reg(i2c_periph,i2c_addr,LED0_OFF_L+4*num,off);
    I2c_write_reg(i2c_periph,i2c_addr,LED0_OFF_H+4*num,off>>8);
}




