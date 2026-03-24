#include "stm32f10x.h"                  // Device header
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "Delay.h"
#include "Timer.h"
#include "LED.h"
#include "24LC64F.h"
#include "rtc.h"
#include "4G.h"
#include "usart.h"
#include "protocol_def.h"
#include "RS485.h"
#include "WindSpeed.h"
#include "Weather.h"
#include "WindDirection.h"
#include "Rainfall.h"
#include <stdarg.h>
#include "MySPI.h"
#include "W25Q128.h"
#include "ATCMD_config.h"
#include "MIC29302.h"
#include "Power_test.h"
#include "LightRadiation.h"


/*****************************************************
		数据定义
*****************************************************/
// 状态定义
typedef enum {
	DATA_READY,      // 准备完成 
	DATA_PREPARE		 // 准备中
} Send4GState;     // 4G数据包上传状态

typedef enum{
	FLASH_READY,		 // 准备完成
	FLASH_PREPARE    // 准备中
} Save2Flash;			 // 数据保存到Flash状态


// 全局变量
volatile Send4GState WeatherDataState = DATA_PREPARE;						// 数据包上传标志位
volatile Save2Flash  FlashSaveState   = FLASH_PREPARE;					// Flash保存标志位		

static uint8_t ChargeCmd = 0x81;        // 充电指令 0x81充电电流小于0x82
char AdressID[32]={0};                  // 设备MN  起始地址0x00   长度不能超过0X1F
char PassWord[32]={0};									// 密码    起始地址0x20	  长度不能超过0X1F
char AdressIP[32]={0};									// IP地址  起始地址0x40  长度不能超过0X1F
char PortIP[32]={0};										// 端口号  起始地址0x60  长度不能超过0X1F

char CID[21]={0};												// 设备CID 	
float Battery_V = 0; 										// 电池电压检测
static u8 Timecount = 0;       					// 时间计数标志

DateTime RTCTime={0,0,0,0,0,0,0};  			// 存放时钟数据
Time_Def AlarmTime={0x30,0x01,0x00,0x0,0,0,0};       //设置重启时间   BCD码形式  每天0"1"30重启
positioning_ positioning; 							// 定位信息
StateOfBattrey BeforeState;
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
	CSTX_CNSSUpdate(&RTCTime);  //获取经纬度信息
	if(RTCTime.year >= 25)			//简易判断 防止校时失败刷新成0
		RTC_WriteDate(&RTCTime);  //注意函数里给秒补偿，因为校时到给到RTC芯片有延时
	CSTX_ReadCID(CID);	
	CSTX_4G_ConTCP();																						//关闭上一次连接
	CSTX_4G_Init();																							//对4G网络连接初始化
	CSTX_4G_CreateTCPSokcet(AdressIP,PortIP);										//创建一个SOCKET连接	
	IWDG_ReloadCounter();						//重装计数器，喂狗
	
	if(strlen(CID) == 20)																				//判断CID读取长度正常
		CID2Web(&RTCTime,AdressID,PassWord,CID);									//发送CID到平台
	Location2Web(&RTCTime,AdressID,PassWord);										//发送经纬度信息到平台
	M24LC64F_WriteReg(0X200,(uint8_t *)positioning.Lat,16);			//将纬度写入EEPROM
	M24LC64F_WriteReg(0X220,(uint8_t *)positioning.Lon,16);			//将经度写入EEPROM
}

/*****************************************************
A7680C关机函数
*****************************************************/
void A7680C_CLOSE(void)
{
	GPRS_OFF();
	OFF_4GVBAT();
	Delay_ms(100);
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
	LED_Init();          																				//电源指示灯初始化
	IWDG_Init();																								//硬件看门狗初始化
	IIC_Init();         						  													//SD3078初始化，RTC芯片
	M24LC64F_Init();																						//24LC64F初始化，EEPROM
	W25Q128_Init();																							//W25Q128初始化，FLASH
	uart2_init(115200);																					//调试或Lora串口初始化
  uart_init(115200);                													//4G串口初始化
	RS485_Init(9600);																						//RS485初始化
	

	AD_Init();																									//电压检测引脚初始化
	I2CWriteSerial(RTC_Address,0x18,1,&ChargeCmd);							//开启SD3078充电模式
	ReadInfor24LC64F((uint8_t*)AdressID,(uint8_t*)PassWord,
									 (uint8_t*)AdressIP,(uint8_t*)PortIP);  		//读取EEPROM存放设备信息
	Battery_V=PowerValue_GET();																	//获取电池电压值
	HardWareFlagChange(Battery_V);															//设置外设状态标志		
		
	if( StateOfHarfWare == ALL_OPEN)
		A7680C_Init();
	printf("Power:%f\r\n",Battery_V);
					
	Set_Alarm(0X07,&AlarmTime);  																//开启时、分、秒报警  用于整点重启

	RTC_ReadDate(&RTCTime);																		  //获取时间
	RainTimeUpdate_Cmd(&RTCTime);																//更新降雨量传感器时间
	Timer2_Init();        																			//定时器2初始化，改变状态标志位
	
	while (1)
	{

			if(WeatherDataState == DATA_READY) {   									//数据包上传标志判断
				WeatherDataState = DATA_PREPARE;											//清空标志位
				if(StateOfHarfWare == ALL_OPEN){
				IWDG_ReloadCounter();																	//喂狗，给4G充足的发送时间防止信号不好重启
				WeatherData2Web(AdressID,PassWord);										//发送数据到平台	
				}
				IWDG_ReloadCounter();																	//喂狗
				if(FlashSaveState == FLASH_READY){										//Flash保存数据标志判断
					FlashSaveState = FLASH_PREPARE;											//清空标志位
					BeforeState = StateOfHarfWare;
					Battery_V=PowerValue_GET();													//获取电池电压值
					HardWareFlagChange(Battery_V);											//设置外设状态标志
					if(BeforeState == ALL_OPEN && StateOfHarfWare != ALL_OPEN)
						A7680C_CLOSE();																		//由高电压到低电压，关闭4G模块
					else if(BeforeState != ALL_OPEN && StateOfHarfWare == ALL_OPEN)
						A7680C_Init();																		//由低电压到高电压，打开4G模块
				}
			}
			
		if(StateOfHarfWare != CLOSE_ALL ){																//判断状态RS485未关闭
			switch(Timecount)  																							//功能周期执行  进行定期喂狗
			{
				case 0:  Weather_Cmd(); IWDG_ReloadCounter();break;						//获取百叶箱传感器信息	    温度、湿度、气压、光照、二氧化碳
													
				case 20: WindSpeed_Cmd(); break;															//获取风速传感器信息		  风速

				case 40: WindDirection_Cmd(); IWDG_ReloadCounter(); break;		//获取风向传感器信息			风向角度	
	
				case 60: Rainfall_Cmd();break;																//获取降雨量传感器信息

				case 80: 
	#ifdef LIGHT_RADIATION
				Light_radiation_Cmd();
	#endif			
				IWDG_ReloadCounter();break;				
	
			}
		}
		else{																				//判断状态RS485关闭
			if(Timecount %40 == 0){										
				IWDG_ReloadCounter();							//喂狗		
			}
		}		
		AT_Pack_CmdProcess((const char*)uart2recAt.Atcmd,AdressIP,PortIP,
												AdressID,PassWord,&RTCTime);					//设备初始化命令处理
		Timecount = ( Timecount + 1 ) % 120;														//Timecount计数周期循环
		Delay_ms(499);
	}
}

/*****************************************************
定时器中断函数
电源指示灯与标志位控制
*****************************************************/
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		RTC_ReadDate(&RTCTime);									 //获取时间
		if(RTCTime.min%5==0 && RTCTime.sec==0){  //如果满足整五分钟，置数据上传标志位
			WeatherDataState=DATA_READY;
			if(RTCTime.min==0){  									 //判断如果整小时 上传数据到FLASH
				FlashSaveState = FLASH_READY;
				if(RTCTime.month == 1 && RTCTime.day == 1 &&	RTCTime.hour == 0 )
					RainClear_Cmd();									 //判断每年进行一次降雨量寄存器数据删除，防止溢出
			}
		}
		if(RTCTime.sec%2==0)										 //电源指示灯2s周期闪烁
			GPIO_SetBits(GPIOB, GPIO_Pin_12);
		else
			GPIO_ResetBits(GPIOB ,GPIO_Pin_12);
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
	

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
       pack_state = PACK_PREPARED;  														 // 设置数据包准备状态
    }
	
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  
    {
				USART_ClearITPendingBit(USART2, USART_IT_RXNE);
				char ch=USART_ReceiveData(USART2);											 //接收模块的数据
        buf_uart2.buf[buf_uart2.index++] = ch;									 //接收模块的数据
				
				switch(at_state){
					case AT_IDLE:																					 //空闲状态接收'A'
						if(ch == 'A'){
							at_state = AT_A_RECEIVED;
						}
						break;
					case AT_A_RECEIVED:
						if(ch == 'T'){																			 //接收"AT"
							at_state = AT_AT_RECEIVED;
						}
						else
							reset_at_state();
						break;
					case AT_AT_RECEIVED:
						if(ch == '+'){																			 //接收"AT+"
							at_state = AT_CMD_RECEIVE;
							uart2recAt.AtcmdIndex = 0;												 //重置CMD索引
						}
						else
							reset_at_state();
						break;
					case AT_CMD_RECEIVE:
						if(ch == ':'){																	  	 //接收CMD,收到':'停止
							at_state = AT_PARAM_RECEIVE;						
							uart2recAt.AtparamIndex = 0;										   //重置参数接收索引
							uart2recAt.Atcmd[uart2recAt.AtcmdIndex] = '\0'; 	 //CMD缓冲区结尾补充'\0'结束符 应对缓冲区刚好被填满的情况
						}
						else if(uart2recAt.AtcmdIndex < AT_CMD_LEN-1)     	 //未溢出时正常接收CMD字符
						{
							uart2recAt.Atcmd[uart2recAt.AtcmdIndex++] = ch;						
						}
						else
							reset_at_state();
						break;
					case AT_PARAM_RECEIVE:
						if(ch == '\r'){																						//接收参数,收到'\r'停止
							at_state = AT_COMPLETE;
							uart2recAt.Atparam[uart2recAt.AtparamIndex++] = '\r';	
						}
						else if(uart2recAt.AtparamIndex <AT_PARAM_LEN-2){					//未溢出正常接收参数
							uart2recAt.Atparam[uart2recAt.AtparamIndex++] = ch;
						}
						else
							reset_at_state();
						break;
					case AT_COMPLETE:
						if(ch == '\n'){																						//接收'\n'，收到\r\n以后置AT包处理标志位
							uart2recAt.Atparam[uart2recAt.AtparamIndex] = '\n';
							pack_state = PACK_PREPARED;
						}
						else
							reset_at_state();
						break;
					default:
						reset_at_state();
						break;
				}				
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

