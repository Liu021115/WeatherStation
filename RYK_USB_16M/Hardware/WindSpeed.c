#include "WindSpeed.h"
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
void WindSpeed_Printf(void)
{
	printf("WindSpeed:");
	for(uint8_t i=0;i<7;i++)
	printf("0x%02X ",windspeed_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取风速数据命令
*****************************************************/
void WindSpeed_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为7位  
	rx_idex = 0;                   
	rx_max = 7;
	//发送查询指令	
	RS485_Send(windspeed_send,8);
	memset(windspeed_receive,0,7);
	//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	windcrc = ModbusCRC16(uart3_data,rx_max-2);

	//数据copy
	if(uart3_data[0]==WindSpeedID &&  uart3_data[1] ==WindSpeedRead && windcrc == ((uart3_data[6] << 8) | uart3_data[5]))
	{
		memset(windspeed_receive,0,rx_max);
		Delay_us(10);
		WindSpeedFlag = FLAG_NORMAL;
		memcpy(windspeed_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.windspeed.value = (float)((windspeed_receive[3]<<8)|windspeed_receive[4])/10;    //风速转换 m/s 
		memset(uart3_data,0,rx_max+1);
	}
	else
				WindSpeedFlag = WEATHER_STOP;
	WeatherData.windspeed.flag = WindSpeedFlag;	

	/*
		windspeed_receive[7] 风速缓冲区数据格式
		0x02  	0x03 	  0x02  	0x00  0x0d  	0x3d  	0x81  
		地址  	功能码  字节数	风速					校验位
	*/
}


