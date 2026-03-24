#include "WindDirection.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define WindDirectionID 0x12
#define WindDirectionRead 0x03
#define WindDirCRC_H 0x86
#define WindDirCRC_L 0xA9

#define WEATHER_WAIT_MAX	300
#define WEATHER_WAIT_STEP	20 

uint8_t windDirection_receive[7];      //风向接收缓冲区
uint8_t windDirection_send[8]={WindDirectionID,WindDirectionRead,0x00,0x00,0x00,0x01,WindDirCRC_H,WindDirCRC_L};  //查询风向指令
uint16_t windDircrc;                  //风向CRC校验
char WindDirFlag;

/*****************************************************
打印数据  WindDirection：0xXX 0xXX形式
*****************************************************/
void WindDirection_Printf(void)
{
	printf("WindDirection:");
	for(uint8_t i=0;i<7;i++)
	printf("0x%02X ",windDirection_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取风向数据命令
*****************************************************/
void WindDirection_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为7位  
	rx_idex = 0;                   
	rx_max = 7;
	//发送查询指令	
	RS485_Send(windDirection_send,8);
		//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	windDircrc = ModbusCRC16(uart3_data,rx_max-2);

	//数据copy 
	if(uart3_data[0]==WindDirectionID &&  uart3_data[1] ==WindDirectionRead  && windDircrc == ((uart3_data[6] << 8) | uart3_data[5]))  //简单的校验
	{
		memset(windDirection_receive,0,rx_max);
		Delay_us(10);
		WindDirFlag = FLAG_NORMAL;
		memcpy(windDirection_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.winddirection.value = (float)((windDirection_receive[3]<<8)|windDirection_receive[4])/10;  //风向角度转换 			
		memset(uart3_data,0,rx_max+1);
	}
	else
		WindDirFlag = WEATHER_STOP;
	WeatherData.winddirection.flag = WindDirFlag;	

}


