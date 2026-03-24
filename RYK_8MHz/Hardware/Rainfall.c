#include "Rainfall.h"

/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define RainfallID 0x56
#define RainfallRead 0x03
#define RainfallCRC_H 0xC8
#define RainfallCRC_L 0x2A
#define RainClearCRC_H 0x75
#define RainClearCRC_L 0xE2

#define WEATHER_WAIT_MAX	300
#define WEATHER_WAIT_STEP	20 


uint8_t Rainfall_receive[26];      		//降雨量接收缓冲区
uint8_t Rainfall_send[8]={RainfallID,RainfallRead,0x00,0x00,0x00,0x0A,RainfallCRC_H,RainfallCRC_L};  //查询降雨量指令
uint16_t Rainfallcrc;                 //降雨量CRC校验
char RainfallFlag;

// 十进制转BCD
static u8 Dec_To_BCD(u8 dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

/*****************************************************
打印数据  Rain：0xXX 0xXX形式
*****************************************************/
void Rainfall_Printf(void)
{
	printf("Rain:");
	for(uint8_t i=0;i<25;i++)
	printf("0x%02X ",Rainfall_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取降雨量数据命令
*****************************************************/
void Rainfall_Cmd(void)
{
	uint16_t wait_time = 0;
	//将RS485接收缓冲区设为25位  
	rx_idex = 0;                   
	rx_max = 25;
	//发送查询指令	
	RS485_Send(Rainfall_send,8);

	//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	Rainfallcrc = ModbusCRC16(uart3_data,rx_max-2);
	//数据copy
	if(uart3_data[0]==RainfallID &&  uart3_data[1] ==RainfallRead  &&  Rainfallcrc == ((uart3_data[24] << 8) | uart3_data[23]))
	{
		memset(Rainfall_receive,0,rx_max);
		Delay_us(10);
		RainfallFlag = FLAG_NORMAL;
		memcpy(Rainfall_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.Rainfall.value =(float)((Rainfall_receive[3]<<8)|Rainfall_receive[4])/10;       //当日总计降雨量 mm
		memset(uart3_data,0,rx_max+1);													//清空串口缓冲区
	}
	else
		RainfallFlag = WEATHER_STOP;
	
	WeatherData.Rainfall.flag = RainfallFlag;
}

/*****************************************************
雨量清零命令
*****************************************************/
void RainClear_Cmd(void)
{
	uint16_t wait_time = 0;
	//将RS485接收缓冲区设为8位  
	rx_idex = 0;                   
	rx_max = 8;
	//发送清零指令
	uint8_t RainClear_cmd[8]={RainfallID,0x06,0x00,0x37,0x00,0x03,RainClearCRC_H,RainClearCRC_L};  //雨量清零指令
	RS485_Send(RainClear_cmd,8);
	while(rx_idex < rx_max &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//比较返回命令
	if(memcmp((const char *)RainClear_cmd, (const char *)uart3_data, 8)!=0){
		Delay_ms(WEATHER_WAIT_MAX);
		RainClear_Cmd();
	}
}


/*****************************************************
修改系统时间命令
*****************************************************/
void RainTimeUpdate_Cmd(DateTime *RTCTime)
{
	uint16_t wait_time = 0;
	//将RS485接收缓冲区设为8位  
	rx_idex = 0;                   
	rx_max = 8;
	//发送清零指令
	uint8_t RainTimeUpdate[15]={RainfallID,0X10,0x00,0x34,0x00,0x03,0x06,
														 Dec_To_BCD(RTCTime->year),Dec_To_BCD(RTCTime->month),
														 Dec_To_BCD(RTCTime->day), Dec_To_BCD(RTCTime->hour),
														 Dec_To_BCD(RTCTime->min), Dec_To_BCD(RTCTime->sec),
														 };  			//修改雨量传感器时间指令
	Rainfallcrc = ModbusCRC16(RainTimeUpdate,13);
	RainTimeUpdate[13]=Rainfallcrc&0xFF;
	RainTimeUpdate[14]=Rainfallcrc>>8;											
														 
	RS485_Send(RainTimeUpdate,15);														 
	while(rx_idex < 8 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}

}

