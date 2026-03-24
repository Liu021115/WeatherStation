#ifndef __USART_H
#define __USART_H
#include "stdio.h"	


#define BUFLEN 2000      //数组缓存大小
typedef struct _UART_BUF
{
    char buf [BUFLEN+1];               
    unsigned int index ;
}UART_BUF;	
	
	
void uart_init(uint32_t bound);
void uart2_init(uint32_t bound);
void UART1_send_byte(char data);
void UART2_send_byte(char data);;
void Uart1_SendStr(char*SendBuf);
void Uart2_SendStr(char*SendBuf);



extern UART_BUF buf_uart1;     //4G
extern UART_BUF buf_uart2;     //PC
extern UART_BUF buf_uart3;     //485
#endif


