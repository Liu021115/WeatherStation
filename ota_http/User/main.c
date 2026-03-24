#include "stm32f10x.h"                  // Device header
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "Delay.h"
#include "24LC64F.h"
#include "4G.h"
#include "usart.h"
#include <stdarg.h>
#include "MIC29302.h"
#include "Power_test.h"
#include "rtc.h"




/*****************************************************
		数据定义
*****************************************************/

uint8_t runflag = 0;									//启动函数运行标志
Time_Def AlarmTime={0x30,0x01,0x00,0x0,0,0,0};       //设置重启时间   BCD码形式  每天0"1"30重启
typedef  void (*pFunction)(void);		//定义一个无参无返回类型的函数指针类型
pFunction jump_to_application;			//声明一个函数指针变量
#define FLASH_JUMP_ADDR (0x08007000)	//定义要跳转的FLASH地址
/*****************************************************
硬件看门狗初始化函数
配置超时时间 20s
*****************************************************/
void IWDG_Init(void)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);								//独立看门狗写使能
	IWDG_SetPrescaler(IWDG_Prescaler_256);											//设置预分频为256
	IWDG_SetReload(4095);																				//设置重装值为4095，独立看门狗的超时时间为26214ms(26s)
	IWDG_ReloadCounter();																				//重装计数器，喂狗
	IWDG_Enable();								   														//独立看门狗使能	
}

/*****************************************************  
A7680C开机函数
*****************************************************/
void A7680C_Init(void)
{
	ON_4GVBAT();								//开启4G模块电源转换
	Delay_ms(50);								//延时使其稳定
	GPRS_ON();									//开启4G模块
	Delay_ms(50);								//延时使其稳定
	CSTX_4G_Init();			
	DownloadAndUpdateFlash();
	IWDG_ReloadCounter();						//重装计数器，喂狗
}

/*****************************************************  
函数名：跳转到应用程序
参  数：app_addr：用户代码起始地址
*****************************************************/
void JumpToApplication(uint32_t app_addr)
{
    pFunction jump_to_app;
		uint32_t stack_ptr = *((uint32_t *)app_addr);
		// 检查栈顶地址是否合法  栈顶（向量表第一个字节）指向RAM地址，STM32F105的RAM为0x20000000 ~ 0x2001FFFF
    if ((stack_ptr >= 0x20000000) && (stack_ptr < 0x20020000))
    {
        // 关闭全局中断  到APP程序重新开启
        __set_PRIMASK(1);

        // 关闭 SysTick
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL  = 0;

				SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk; //清除挂起
			
        // 关闭 NVIC 所有中断
        for (int i = 0; i < 8; i++)
        {
            NVIC->ICER[i] = 0xFFFFFFFF;
            NVIC->ICPR[i] = 0xFFFFFFFF;
        }

				
        // 复位 RCC（可选，让 APP 从默认时钟启动）
        RCC_DeInit();

				// 使能全局中断，可以在APP程序里使能会更加安全
				//__set_PRIMASK(0);
				
				__DSB();
        __ISB();
				
        // 设置新的 MSP 栈顶
        __set_MSP(*((uint32_t *)app_addr));
				// 0 强制使用MSP，如果是RTOS系统使用的是 1 PSP，这样更安全
				__set_CONTROL(0); 
        // 获取 Reset_Handler 地址
        jump_to_app = (pFunction)(*((uint32_t *)(app_addr + 4)));

        // 跳转到 APP
        jump_to_app();
    }

    // 如果跳转失败，软件复位
    NVIC_SystemReset();
}

/*****************************************************
主函数
*****************************************************/
int main(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);					//使能AFIO时钟，即端口复用时钟
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);			//失能JTAG	
	GPRS_GPIO_Init();																						//初始化GPRS控制引脚
	Init_4GVBAT();																							//初始化4G模块电源控制引脚
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);							//配置NVIC为分组2 抢占优先级范围：0~3，响应优先级范围：0~3
	IIC_Init();         						  													//SD3078初始化，RTC芯片
	uart2_init(115200);																					//调试或Lora串口初始化
  uart_init(115200);                													//4G串口初始化
	runflag =	EEPROM_read(HttpVersion,AdreeMN);									//读取EEPROM存放的ID，读取长度32位
	Set_Alarm(0X07,&AlarmTime);  																//开启时、分、秒报警  用于整点重启
	if(!runflag)
			runflag	= WebVoltageCheck();														//检测电压											
	else
			printf("***** READ  EEPROM  FAULT *****\r\n");							//
	if(runflag)	{
			printf("***** LOW 	VOLTAGE	***** \r\n");
			printf("***** JUMP 	TO  APP	***** \r\n");
			JumpToApplication(FLASH_JUMP_ADDR);    									//跳转到APP
		}		
	A7680C_Init();																							//开启4G准备下载
	printf("***** JUMP 	TO  APP	***** \r\n");	
	JumpToApplication(FLASH_JUMP_ADDR);     //跳转到APP
	while (1)
	{
		printf("***** JUMP  ERROR *****\r\n");	
		NVIC_SystemReset();										//跳转失败，重启设备			
	}
}

	
/*****************************************************
串口1相关函数
*****************************************************/
void ec200x_receive_process_event(unsigned char ch )     //串口1给4g用
{
    if(buf_uart1.index >= BUFLEN)
    {
        buf_uart1.index = 0 ;
    }
    else
    {
        buf_uart1.buf[buf_uart1.index++] = ch;
    }
}

/*****************************************************
串口1中断服务程序
*****************************************************/
void USART1_IRQHandler(void)                            //串口1接收函数
{

    if(USART_GetITStatus(USART1, USART_IT_RXNE)==SET)
    {
				char ch = USART_ReceiveData(USART1);
				ec200x_receive_process_event(USART_ReceiveData(USART1));
				USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
        
    if(USART_GetFlagStatus(USART1,USART_FLAG_ORE)==SET)
    {
        USART_ClearFlag(USART1,USART_FLAG_ORE);
    }
}

/*****************************************************
串口2中断服务程序
*****************************************************/
void USART2_IRQHandler(void)                               			 //串口2中断服务程序
{
		if (USART_GetFlagStatus(USART2, USART_FLAG_ORE) != RESET)    //溢出错误检查   数据未被读取新数据被接收  高速数据接收可不
    {
       USART_ReceiveData(USART2);
       USART_ClearFlag(USART2, USART_FLAG_ORE);
    }
        
    if (USART_GetFlagStatus(USART2, USART_FLAG_FE) != RESET)     //帧错误  停止位错误置1
    {
       USART_ReceiveData(USART2);
       USART_ClearFlag(USART2, USART_FLAG_FE);
    }
	  if (USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)       //空闲中断标志位检查
    {
       USART_ReceiveData(USART2);  															 // 必须读取 DR 才能清除 IDLE 标志
       USART_ClearITPendingBit(USART2, USART_IT_IDLE);
    }
	
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  
    {
				USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        buf_uart2.buf[buf_uart2.index++] = USART_ReceiveData(USART2);			 //接收模块的数据
    }	  	
} 

/*****************************************************
串口重定向  printf通过UART2打印
*****************************************************/
int fputc(int ch, FILE *stream)                     			//串口重定向 USART2
{
	USART_SendData(USART2,(uint16_t)ch);
	while(USART_GetFlagStatus(USART2,USART_FLAG_TXE)==RESET);
	
	return ch;
}

