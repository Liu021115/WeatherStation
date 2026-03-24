#include "CO2.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define WindSpeedID 0x13
#define WindSpeedRead 0x03
#define WindSpeedCRC_H 0x87
#define WindSpeedCRC_L 0x78

#define WEATHER_WAIT_MAX	300
#define WEATHER_WAIT_STEP	20 

uint8_t windspeed_receive[7];      //风速接收缓冲区
uint8_t windspeed_send[8]={WindSpeedID,0x03,0x00,0x00,0x00,0x01,WindSpeedCRC_H,WindSpeedCRC_L};  //查询风速指令
uint16_t windcrc;                  //风速CRC校验
char WindSpeedFlag;

/*****************************************************
打印数据  WindSpeed：0xXX 0xXX形式
*****************************************************/
void CO2_Printf(void)
{
	printf("CO2:");
	for(uint8_t i=0;i<7;i++)
	printf("0x%02X ",windspeed_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取风速数据命令
*****************************************************/
void CO2_Cmd(void)
{
			WeatherData.CO2.flag =FLAG_NORMAL;

	/*
		windspeed_receive[7] 风速缓冲区数据格式
		0x02  	0x03 	  0x02  	0x00  0x0d  	0x3d  	0x81  
		地址  	功能码  字节数	风速					校验位
	*/
}


