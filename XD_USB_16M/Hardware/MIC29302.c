#include "stm32f10x.h" 

/*****************************************************
	函数: 初始换4G电源控制引脚
	参数: 无
*****************************************************/
void Init_4GVBAT(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	
	GPIO_InitTypeDef GPIO_InitStructure;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);			
	GPIO_SetBits(GPIOC,GPIO_Pin_9);												// 默认PC9高电平，即不开启4G电源
}

/*****************************************************
	函数: 开启4G电源
	参数: 无
*****************************************************/
void ON_4GVBAT(void)
{
	GPIO_ResetBits(GPIOC,GPIO_Pin_9);	
}

/*****************************************************
	函数: 关闭4G电源
	参数: 无
*****************************************************/
void OFF_4GVBAT(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_9);	
}
