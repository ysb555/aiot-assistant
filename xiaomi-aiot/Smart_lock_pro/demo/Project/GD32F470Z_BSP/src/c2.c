#include "string.h"
#include "stdio.h"
#include "uart.h"
#include "c2.h"


/*********************************************************************************************
º¯ÊýÃû:    c2_init
¹¦ÄÜ:      ³õÊ¼»¯zigbeeÄ£¿é
Èë¿Ú²ÎÊý:  zigbeemode:zigbee¹¤×÷Ä£Ê½
³ö¿Ú²ÎÊý£º ÎÞ
·µ»ØÖµ£º   ³É¹¦·µ»Ø1,Ê§°Ü·µ»Ø0
×÷Õß£º     ZZZ
ÈÕÆÚ:      2023/4/8
µ÷ÓÃÃèÊö:  ³õÊ¼»¯zigbeeÄ£¿é¹¤×÷Ä£Ê½,Ð­µ÷Æ÷»òÕßÖÕ¶Ë
**********************************************************************************************/
uint8_t c2_init(uint8_t zigbee_mode)
{
	uart_init(USART2);
	volatile uint8_t result = 0;
	uint8_t recvdata[50];
	uint8_t i;

	uint8_t read_device_data[4] = {0xFE,0x01,0xFE,0xFF};	      //¶ÁÈ¡Éè±¸ËùÓÐÊý¾Ý
	uint8_t terminal[5] = {0xFD,0x02,0x01,0x02,0xFF};	          //ÅäÖÃÉè±¸ÀàÐÍ£¬ÅäÖÃÎª 02 ÖÕ¶Ë
	uint8_t coordinator[5] = {0xFD,0x02,0x01,0x00,0xFF};          //ÅäÖÃÎªÐ­µ÷Æ÷ 
	uint8_t pan_id[6] = {0xFD,0x03,0x03,0x3F,0x00,0xFF};          //ÅäÖÃPAN_ID 0x3f2c
	uint8_t group_id[5] = {0xFD,0x02,0x09,0x01,0xFF};             //ÅäÖÃÍøÂç×é
	uint8_t key[20] = {
		0xFD, 0x11, 0x04, 0x12, 0x13, 
    0x15, 0x17, 0x19, 0x1B, 0x1D, 
    0x1F, 0x10, 0x12, 0x14, 0x16, 
    0x18, 0x1A, 0x1C, 0x1D, 0xFF};
	for(i=0;i<3;i++)
	{
		memset(recvdata,0,50);
		uart_send_bytes(USART2,read_device_data,4);	
		uart_rece_bytes(USART2, recvdata, 10, 1000);	
		if(recvdata[0] == 0xfb)
		{ 
			result = 1;
			break;	
		}	
	}

	if(result)
	{
		result = 0;

		for(i=0;i<3;i++)
		{
			memset(recvdata,0,50);
			if(zigbee_mode == COORDINATOR)
				uart_send_bytes(USART2,coordinator,5);	
			else 
				uart_send_bytes(USART2,terminal,5);	

			uart_rece_bytes(USART2, recvdata, 10, 1000);
			
			if( (recvdata[0] == 0xFA) && (recvdata[1] == 0x01) )
			{
				result = 1;
				break;
			}  
		}
	}	


	if(result)
	{
		result = 0;
		for(i=0;i<3;i++)
		{
			memset(recvdata,0,50);
			uart_send_bytes(USART2,pan_id,6);
			uart_rece_bytes(USART2, recvdata, 10, 1000);
			if( (recvdata[0] == 0xFA) && (recvdata[1] == 0x03) )
			{
				result = 1;
				break;															
			}	
		}
	}

	if(result)
	{
		result = 0;
		for(i=0;i<3;i++)
		{
			memset(recvdata,0,50);
			uart_send_bytes(USART2,group_id,5);
			uart_rece_bytes(USART2, recvdata, 10, 1000);
			if( (recvdata[0] == 0xFA) && (recvdata[1] == 0x09) )
			{
				result = 1;
				break;															
			}	
		}
	}
	if(result)
	{
		result = 0;
		for(i=0;i<3;i++)
		{
			memset(recvdata,0,50);
			uart_send_bytes(USART2,key,20);
			uart_rece_bytes(USART2, recvdata, 10, 1000);
			if( (recvdata[0] == 0xFA) && (recvdata[1] == 0x04) )
			{
				result = 1;
				break;															
			}	
		}
	}

	return result;
}


/*********************************************************************************************
º¯ÊýÃû:    c2_broadcast_data
¹¦ÄÜ:      c2·¢ËÍ¹ã²¥Êý¾Ý
Èë¿Ú²ÎÊý:  *send_data:´ý·¢ËÍÊý¾ÝÖ¸Õë mode:¹ã²¥Ä£Ê½
           01£º¹ã²¥Ä£Ê½ 1 ¡ª¡ª¸ÃÏûÏ¢¹ã²¥µ½È«ÍøÂçÖÐËùÓÐÉè±¸
           02£º¹ã²¥Ä£Ê½ 2 ¡ª¡ª¸ÃÏûÏ¢¹ã²¥µ½Ö»¶Ô´ò¿ªÁË½ÓÊÕ£¨³ýÐÝÃßÄ£Ê½£©µÄÉè±¸
           03£º¹ã²¥Ä£Ê½ 3 ¡ª¡ª¸ÃÏûÏ¢¹ã²¥µ½ËùÓÐÈ«¹¦ÄÜÉè±¸£¨Â·ÓÉÆ÷ºÍÐ­µ÷Æ÷£©
³ö¿Ú²ÎÊý£º ÎÞ
·µ»ØÖµ£º   ÎÞ
×÷Õß£º     ZZZ
ÈÕÆÚ:      2023/4/8
µ÷ÓÃÃèÊö:  ·¢ËÍ¹ã²¥Êý¾Ý
**********************************************************************************************/
void c2_broadcast_data(char *send_data, uint8_t mode)
{
	char sendbuf[100]; 
	uint8_t len;

	sendbuf[0] = 0xFC;
	len = strlen(send_data);
	sendbuf[1] = len + 2;
	sendbuf[2] = 0x01; 
	sendbuf[3] = mode; 
	strncpy(&sendbuf[4],(const char*)send_data,len);
	uart_send_bytes(USART2,(uint8_t *)sendbuf,len+4);
}



/*********************************************************************************************
º¯ÊýÃû:    c2_rec_data
¹¦ÄÜ:      ½ÓÊÕzigbee·¢ËÍÊý¾Ý
Èë¿Ú²ÎÊý:  *rec_data:½ÓÊÕÊý¾ÝÖ¸Õë  len:½ÓÊÕÊý¾Ý³¤¶È  timeout:µÈ´ýÊ±¼ä
³ö¿Ú²ÎÊý£º *rec_data
·µ»ØÖµ£º   Êµ¼Ê½ÓÊÕ³¤¶È
×÷Õß£º     ZZZ
ÈÕÆÚ:      2023/4/8
µ÷ÓÃÃèÊö:  ½ÓÊÕzigbee·¢ËÍÊý¾Ý
**********************************************************************************************/
uint8_t c2_rec_data(out uint8_t *rec_data, uint16_t len, uint16_t timeout)
{
	uint8_t recvlen;

	recvlen = uart_rece_bytes(USART2, rec_data, len, timeout);	  

	return recvlen;	
}


