#include "Weather.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define WeatherID 0x15
#define WeatherRead 0x03
#define WeatherCRC_H 0x86
#define WeatherCRC_L 0xDD

#define WEATHER_WAIT_MAX	500
#define WEATHER_WAIT_STEP	20 

uint8_t weather_receive[15];      //气象接收缓冲区
uint8_t weather_send[8]={WeatherID,WeatherRead,0x00,0x00,0x00,0x05,WeatherCRC_H,WeatherCRC_L};  //查询气象指令
uint16_t weathercrc;                  //气象CRC校验
char WeatherFlag;

/*****************************************************
打印数据  Weather：0xXX 0xXX形式
*****************************************************/
void Weather_Printf(void)
{
	printf("Weather:");
	for(uint8_t i=0;i<15;i++)
	printf("0x%02X ",weather_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取气象数据命令
*****************************************************/
void Weather_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为7位  
	rx_idex = 0;                   
	rx_max = 15;
	//发送查询指令	
	RS485_Send(weather_send,8);
	//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	weathercrc = ModbusCRC16(uart3_data,rx_max-2);

	//数据copy
	if(uart3_data[0]==WeatherID &&  uart3_data[1] ==WeatherRead  &&  weathercrc == ((uart3_data[14] << 8) | uart3_data[13]))
	{
		memset(weather_receive,0,rx_max);
		Delay_us(10);
		WeatherFlag = FLAG_NORMAL;
		memcpy(weather_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.tempe.value =(float)(int16_t)((weather_receive[3]<<8)|weather_receive[4])/10;       //摄氏度
		WeatherData.humi.value = (float)((weather_receive[5]<<8)|weather_receive[6])/10;			 //%
		WeatherData.Lux.value = (float)((weather_receive[9]<<8)|weather_receive[10])*10;        //LUX
		WeatherData.pressure.value =(float)((weather_receive[7]<<8)|weather_receive[8])/100;    //kPa
		WeatherData.CO2.value = (float)((weather_receive[11]<<8)|weather_receive[12]);           //ppm
		memset(uart3_data,0,rx_max+1);
	}
	else
		WeatherFlag = WEATHER_STOP;
	
	WeatherData.tempe.flag = WeatherFlag;
  WeatherData.humi.flag = WeatherFlag;									
  WeatherData.Lux.flag = WeatherFlag;
  WeatherData.pressure.flag = WeatherFlag;
  WeatherData.CO2.flag = WeatherFlag;

}



