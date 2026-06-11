#include <string.h>
#include "uart.h"

// 串口数据接收缓存
uint8_t Uart0_RecvBuffer[512];
uint16_t Uart0_RecvLenth = 0;
UartRecvState_t Uart0_RecvFlag;
uint16_t Uart0_RecvCnt = 0;

uint8_t Uart2_RecvBuffer[512];
uint16_t Uart2_RecvLenth = 0;
UartRecvState_t Uart2_RecvFlag;
uint16_t Uart2_RecvCnt = 0;

uint8_t Uart5_RecvBuffer[512];
uint16_t Uart5_RecvLenth = 0;
UartRecvState_t Uart5_RecvFlag;
uint16_t Uart5_RecvCnt = 0;

uint8_t delay_flag = 0;	  // 延时标志
uint16_t delay_count = 0; // 延时变量
/*********************************************************************************************
 *函数名:    uart_init
 *功能:      初始化相应串口
 *入口参数:  usart_periph   USART0/USART2
 *出口参数： 无
 *返回值：   无
 *作者：     ZZZ
 *日期:      2023/4/6
 **********************************************************************************************/
void uart_init(uint32_t usart_periph)
{
	if (usart_periph == USART0)
	{
		nvic_irq_enable(USART0_IRQn, 0, 0);
		rcu_periph_clock_enable(RCU_GPIOB);								  /* enable GPIO clock */
		rcu_periph_clock_enable(RCU_USART0);							  /* enable USART clock */
		gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_6);						  /* connect port to USARTx_Tx */
		gpio_af_set(GPIOB, GPIO_AF_7, GPIO_PIN_7);						  /* connect port to USARTx_Rx */
		gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6); /* configure USART Tx as alternate function push-pull */
		gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
		gpio_mode_set(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_7); /* configure USART Rx as alternate function push-pull */
		gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
		/* USART configure */
		usart_deinit(USART0);
		usart_baudrate_set(USART0, 115200U);
		usart_receive_config(USART0, USART_RECEIVE_ENABLE);
		usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
		usart_enable(USART0);
		usart_interrupt_enable(USART0, USART_INT_RBNE);
	}

	else if (usart_periph == USART2)
	{
		nvic_irq_enable(USART2_IRQn, 0, 0);
		rcu_periph_clock_enable(RCU_GPIOC);								   /* enable GPIO clock */
		rcu_periph_clock_enable(RCU_USART2);							   /* enable USART clock */
		gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_10);						   /* connect port to USARTx_Tx */
		gpio_af_set(GPIOC, GPIO_AF_7, GPIO_PIN_11);						   /* connect port to USARTx_Rx */
		gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10); /* configure USART Tx as alternate function push-pull */
		gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
		gpio_mode_set(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_11); /* configure USART Rx as alternate function push-pull */
		gpio_output_options_set(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);
		/* USART configure */
		usart_deinit(USART2);
		usart_baudrate_set(USART2, 115200U);
		usart_receive_config(USART2, USART_RECEIVE_ENABLE);
		usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
		usart_enable(USART2);
		usart_interrupt_enable(USART2, USART_INT_RBNE);
	}
}

/***********************************************************************************************************
//timer3 init 1ms定时
************************************************************************************************************/
void timer3_init(void)
{
	timer_parameter_struct timer_init_struct;

	rcu_periph_clock_enable(RCU_TIMER3);

	timer_deinit(TIMER3);
	timer_init_struct.prescaler = 4199;
	timer_init_struct.period = 20;
	timer_init_struct.alignedmode = TIMER_COUNTER_EDGE;
	timer_init_struct.counterdirection = TIMER_COUNTER_UP;
	timer_init_struct.clockdivision = TIMER_CKDIV_DIV1;
	timer_init_struct.repetitioncounter = 0;
	timer_init(TIMER3, &timer_init_struct);

	nvic_irq_enable(TIMER3_IRQn, 1, 1);
	timer_interrupt_enable(TIMER3, TIMER_INT_UP);
	timer_enable(TIMER3);
}

/********************************************************************************************************
//发送数据
*********************************************************************************************************/
uint16_t uart_send_bytes(uint32_t usart_periph, uint8_t *data, uint16_t len)
{
	uint8_t i;
	for (i = 0; i < len; i++)
	{
		while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET)
			;
		usart_data_transmit(usart_periph, data[i]);
	}
	while (usart_flag_get(usart_periph, USART_FLAG_TC) == RESET)
		;
	return len;
}

/*********************************************************************************************************
//接收数据
**********************************************************************************************************/
uint16_t uart_rece_bytes(uint32_t usart_periph, out uint8_t *data, uint16_t len, uint16_t timeout)
{
	UartRecvState_t *recvflag;
	uint8_t *recvbuf;
	uint16_t *recvlen;
	uint16_t ret_len = 0;
	if (usart_periph == USART0)
	{
		recvflag = &Uart0_RecvFlag;
		recvbuf = Uart0_RecvBuffer;
		recvlen = &Uart0_RecvLenth;
	}
	else if (usart_periph == USART2)
	{
		recvflag = &Uart2_RecvFlag;
		recvbuf = Uart2_RecvBuffer;
		recvlen = &Uart2_RecvLenth;
	}
	while (timeout--)
	{
		if (*recvflag == STATE_RX_RECEIVED)
		{
			if (*recvlen > len)
			{
				ret_len = len;
			}
			else
			{
				ret_len = *recvlen;
			}
			memcpy(data, recvbuf, ret_len);

			memset(recvbuf, 0, *recvlen);
			*recvlen = 0;
			*recvflag = STATE_RX_IDLE;
		}
		delay_ms(1);
	}
	return ret_len;
}

/********************************************************************************************************
延时ms函数
*********************************************************************************************************/
void delay_ms(uint16_t mstime)
{
	delay_count = mstime;

	while (delay_count)
	{
	}
}

/********************************************************************************************************
1ms定时中断服务程序
*********************************************************************************************************/
void TIMER3_IRQHandler(void)
{
	if (timer_interrupt_flag_get(TIMER3, TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER3, TIMER_INT_FLAG_UP);

		if (delay_count > 0)
			delay_count--;
		if (Uart0_RecvFlag == STATE_RX_RECEIVING)
		{
			Uart0_RecvCnt++;
			if (Uart0_RecvCnt >= 20)
			{
				Uart0_RecvCnt = 0;
				Uart0_RecvFlag = STATE_RX_RECEIVED;
			}
		}

		if (Uart2_RecvFlag == STATE_RX_RECEIVING)
		{
			Uart2_RecvCnt++;
			if (Uart2_RecvCnt >= 20)
			{
				Uart2_RecvCnt = 0;
				Uart2_RecvFlag = STATE_RX_RECEIVED;
			}
		}
	}
}

/*********************************************************************************************
 *函数名:    USART0_IRQHandler
 *功能:      串口0中断函数,接收串口数据
 *入口参数:  中断相应数据
 *出口参数： Uart0_RecvBuffer存放接收数据,Uart0_RecvLenth接收数据长度
 *返回值：   无
 *作者：     ZZZ
 *日期:      2023/4/6
 **********************************************************************************************/
void USART0_IRQHandler(void)
{
	if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);

		Uart0_RecvBuffer[Uart0_RecvLenth] = usart_data_receive(USART0);
		Uart0_RecvLenth++;

		if (Uart0_RecvFlag != STATE_RX_RECEIVING)
		{
			Uart0_RecvFlag = STATE_RX_RECEIVING;
		}
		Uart0_RecvCnt = 0;
	}
}

void USART2_IRQHandler(void)
{
	if (usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_flag_clear(USART2, USART_INT_FLAG_RBNE);

		Uart2_RecvBuffer[Uart2_RecvLenth] = usart_data_receive(USART2);
		Uart2_RecvLenth++;

		if (Uart2_RecvFlag != STATE_RX_RECEIVING)
		{
			Uart2_RecvFlag = STATE_RX_RECEIVING;
		}
		Uart2_RecvCnt = 0;
	}
}

void USART5_IRQHandler(void)
{
	if (usart_interrupt_flag_get(USART5, USART_INT_FLAG_RBNE) != RESET)
	{
		usart_interrupt_flag_clear(USART5, USART_INT_FLAG_RBNE);

		Uart5_RecvBuffer[Uart5_RecvLenth] = usart_data_receive(USART5);
		Uart5_RecvLenth++;

		if (Uart5_RecvFlag != STATE_RX_RECEIVING)
		{
			Uart5_RecvFlag = STATE_RX_RECEIVING;
		}
		Uart5_RecvCnt = 0;
	}
}
