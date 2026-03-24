#ifndef __RS485_H_
#define __RS485_H_

#include "stm32f10x.h"  

#include "Delay.h"

extern volatile uint8_t rx_idex;            //RS485接收缓冲区索引
extern volatile uint8_t rx_max;						 //RS485接收缓冲区索引最大值
extern uint8_t uart3_data[30];         //RS485接收缓冲区


void RS485_Init(uint32_t bound);
void RS485_Send(uint8_t *data,uint8_t len);
uint16_t ModbusCRC16(uint8_t *data, uint16_t len);
void USART3_IRQHandler(void);
#endif

