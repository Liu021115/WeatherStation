#include "RS485.H"

uint8_t uart3_data[70];		// 串口三接收缓冲区
volatile uint8_t rx_idex =0;
volatile uint8_t rx_max =69;

/*****************************************************
下面就是需要修改的地方，修改端口和引脚
*****************************************************/
#define DE_PORT  GPIOD
#define DE_PIN   GPIO_Pin_2
#define DE_RCC	 RCC_APB2Periph_GPIOD


void RS485_Init(uint32_t bound)
{
//开启时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	RCC_APB2PeriphClockCmd(DE_RCC,ENABLE);	
	
//串口初始化
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART3,&USART_InitStructure);
	
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
	
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);
	
	USART_Cmd(USART3,ENABLE);
	//DE初始化

	// 配置PD2为推挽输出（DE控制引脚）
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = DE_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	// 初始化时默认设置为接收模式（DE=0）
	GPIO_ResetBits(DE_PORT, DE_PIN);
	
}

//接收模式
void RS485_RxMode(void)
{
	GPIO_ResetBits(DE_PORT, DE_PIN);
}

//发送模式
void RS485_TxMode(void)
{
	GPIO_SetBits(DE_PORT, DE_PIN);
}

//发送数据
void RS485_Send(uint8_t *data,uint8_t len)   //接收参数 发送数组、发送长度
{
	RS485_TxMode();					//切换发送模式
	Delay_us(500);
	for(uint8_t i =0;i<len;i++)
	{
		while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);	//等待上一次数据发送完成
		USART_SendData(USART3,data[i]);
	}		
	while(USART_GetFlagStatus(USART3,USART_FLAG_TXE)==RESET);//等待最后一组数据发送完成
	Delay_ms(2);        //接收数据格式不对时改变延时，不同设备的信息之间间隔不同
	RS485_RxMode();  				//切换接收模式
}	

//数据处理
void RS485_Receive()
{
	
	
}

//CRC校验
uint16_t ModbusCRC16(uint8_t *data, uint16_t len)
{
	uint16_t crc = 0xFFFF;
    for(uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(uint8_t j = 0; j < 8; j++) {
            if(crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

//中断，用于接收数据
void USART3_IRQHandler(void)
{
    uint32_t temp;  // 用于清除溢出错误 
    // 处理接收溢出错误（ORE）
    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) == SET)
    {
        temp = USART3->SR;  // 先读SR寄存器
        temp = USART3->DR;  // 再读DR寄存器，清除ORE标志
    }
		
	if (USART_GetFlagStatus(USART3,USART_FLAG_RXNE) == SET)
	{
		uint8_t data = USART_ReceiveData(USART3);

			uart3_data[rx_idex] = data;
			rx_idex++;
		USART_ClearFlag(USART3,USART_FLAG_RXNE);
		
	}

}
