#include "stm32f10x.h"                  // Device header
#include "usart.h"	  
#include "Delay.h"
#include "stdio.h"
#include "string.h"
//////////////////////////////////////////////////////////////////
       
// 数据定义
UART_BUF buf_uart1;     //4G模块 A7680C
UART_BUF buf_uart2;     //PC调试串口活LORA模块

/*****************************************************
	串口一发送字符函数
	参数： data  待发送的字节
*****************************************************/
void UART1_send_byte(char data)
{
	while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	USART_SendData(USART1, data);
}

/*****************************************************
	串口二发送字符函数
	参数： data  待发送的字节
*****************************************************/
void UART2_send_byte(char data)
{
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
	USART_SendData(USART2, data);
}


/*****************************************************
	串口一初始化函数
	参数: bound 波特率
*****************************************************/
void uart_init(uint32_t bound)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
		USART_DeInit(USART1);  //复位串口1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级0
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;			//子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);													//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置
		USART_InitStructure.USART_BaudRate = bound;									//波特率可修改
		USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;			//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;					//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(USART1, &USART_InitStructure); //初始化串口
		
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启空闲中断
    USART_Cmd(USART1, ENABLE);                    //使能串口 

}

/*****************************************************
	串口二初始化函数
	参数: bound 波特率
*****************************************************/
void uart2_init(uint32_t bound){
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能，GPIOA时钟
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);	//USART2
		USART_DeInit(USART2);  //复位串口2
	 //USART2_TX   PA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
   
    //USART2_RX	  PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3

   //Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//抢占优先级2
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;			//子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);													//根据指定的参数初始化VIC寄存器
		
		 //USART 初始化设置
		USART_InitStructure.USART_BaudRate = bound;						  		//一般设置为9600;可修改
		USART_InitStructure.USART_WordLength = USART_WordLength_8b; //字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;			//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;					//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART2, &USART_InitStructure); //初始化串口
    USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);//开启空闲中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启接收中断
    USART_Cmd(USART2, ENABLE);                    //使能串口 
}

/*****************************************************
	串口一打印数据函数
*****************************************************/
void Uart1_SendStr(char*SendBuf)//串口1打印数据
{
	while(*SendBuf)
	{
        while((USART1->SR&0X40)==0);//等待发送完成 
        USART1->DR = (u8) *SendBuf; 
        SendBuf++;
	}
}

/*****************************************************
	串口二打印数据函数
*****************************************************/
void Uart2_SendStr(char*SendBuf)//串口2打印数据
{
	while(*SendBuf)
	{
        while((USART2->SR&0X40)==0);//等待发送完成 
        USART2->DR = (u8) *SendBuf; 
        SendBuf++;
	}

}





