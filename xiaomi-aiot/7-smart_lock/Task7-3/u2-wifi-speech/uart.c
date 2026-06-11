/**@file  uart.c
* @brief    负责uart通信
* @details  主要包含打开并初始化串口、通过uart发送数据、通过uart接收数据
* @author      wangyuhang  any question please send mail to 1195341023@qq.com
* @date        2023-7-5
* @version     V1.0
**********************************************************************************
* @par 修改日志:
* <table>
* <tr><th>Date        <th>Version  <th>Author      <th>Description
* <tr><td>2023/07/05  <td>1.0      <td>wangyuhang  <td>创建初始版本
* </table>
*
**********************************************************************************
*/
# include "../inc/uart.h"

/*********************************************************************************************
函数名:    delay_ms
功能:      等待指定时间
入口参数:   mstime:等待时间，单位是毫秒
出口参数： 无
返回值：   无
作者：     wyh
日期:      2023/7/6
**********************************************************************************************/
void delay_ms(uint16_t mstime)
{
    usleep(mstime * 1000);
}

int serial_fd = 0; 
  
/*********************************************************************************************
函数名:    init_serial
功能:      打开串口并初始化设置
入口参数:   *device：希望打开的设备地址
出口参数： 无
返回值：   -1表示打开失败，0表示打开成功
作者：     wyh
日期:      2023/7/6
**********************************************************************************************/
int init_serial(const char* device) 
{ 
    serial_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); 
    if (serial_fd < 0) { 
        perror("open"); 
        return -1; 
    } 
      
    //串口主要设置结构体termios <termios.h> 
    struct termios options; 
      
    /**1. tcgetattr函数用于获取与终端相关的参数。 
    *参数fd为终端的文件描述符，返回的结果保存在termios结构体中 
    */ 
    tcgetattr(serial_fd, &options); 
    /**2. 修改所获得的参数*/ 
    options.c_cflag |= (CLOCAL | CREAD);//设置控制模式状态，本地连接，接收使能 
    options.c_cflag &= ~CSIZE;//字符长度，设置数据位之前一定要屏掉这个位 
    options.c_cflag &= ~020000000000;//无硬件流控 
    options.c_cflag |= CS8;//8位数据长度 
    options.c_cflag &= ~CSTOPB;//1位停止位 
    options.c_iflag |= IGNPAR;//无奇偶检验位 
    options.c_oflag = 0; //输出模式 
    options.c_lflag = 0; //不激活终端模式 
    cfsetospeed(&options, B115200);//设置波特率 
      
    /**3. 设置新属性，TCSANOW：所有改变立即生效*/ 
    tcflush(serial_fd, TCIFLUSH);//溢出数据可以接收，但不读 
    tcsetattr(serial_fd, TCSANOW, &options); 
      
    return 0; 
} 
  

/*********************************************************************************************
函数名:    uart_send
功能:      通过uart通信向指定文件发送指定消息
入口参数:   fd:设备文件标识符  *data:要发送的消息 datalen:发送消息的长度
出口参数： 无
返回值：   -1表示发送失败，否则返回发送数据的长度
作者：     wyh
日期:      2023/7/6
**********************************************************************************************/
int uart_send(int fd, uint8_t *data, int datalen) 
{ 
    int len = 0; 
    len = write(fd, data, datalen);//实际写入的长度 
    if(len == datalen) { 
        return len; 
    } else { 
        tcflush(fd, TCOFLUSH);//TCOFLUSH刷新写入的数据但不传送 
        return -1; 
    } 
    return 0; 
} 
  
/*********************************************************************************************
函数名:    uart_recv
功能:      通过uart通信从指定文件读取信息
入口参数:   fd:设备文件标识符  *data:用来接收信息的数组指针 datalen:接收信息的长度
出口参数： 无
返回值：   -1表示接收失败，否则返回接收到的信息长度
作者：     wyh
日期:      2023/7/6
**********************************************************************************************/
int uart_recv(int fd, uint8_t *data, int datalen) 
{ 
    int len=0; 
    fd_set fs_read; 
    struct timeval tv_timeout; 
      
    FD_ZERO(&fs_read); 
    FD_SET(fd, &fs_read); 

#ifdef S_TIMEOUT    
    tv_timeout.tv_sec = (10*20/115200+1); 
    tv_timeout.tv_usec = 0; 
    select(fd+1, &fs_read, NULL, NULL, &tv_timeout);
#elif
    select(fd+1, &fs_read, NULL, NULL, NULL);
#endif
     
    if (FD_ISSET(fd, &fs_read)) { 
        len = read(fd, data, datalen); 
        printf("read length =%d\n",len);
        return len; 
    } else { 
        perror("select"); 
        return -1; 
    } 
      
    return 0; 
} 










