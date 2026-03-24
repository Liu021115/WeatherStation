#include "Weather.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define WeatherID 0x14
#define RS485READ_FUN_CODE 0x04
#define RS485ReadAdd_H 	 0x00
#define RS485ReadAdd_L   0x00
#define WeatherReadLen_H	 0x00
#define WeatherReadLen_L	 0x20
#define WeatherCRC_H 0xF3
#define WeatherCRC_L 0x17

#define	RS485WRITE_FUN_CODE	0x06
#define RS485WriteAdd_H 	 0x20
#define RS485WriteAdd_L    0x00

#define RainClearCRC_H		0xD4
#define RainClearCRC_L		0x81

#define WEATHER_WAIT_MAX	500
#define WEATHER_WAIT_STEP	20 

uint8_t weather_receive[69];      //气象接收缓冲区
uint8_t weather_send[8]={WeatherID,RS485READ_FUN_CODE,RS485ReadAdd_H,RS485ReadAdd_L,WeatherReadLen_H,WeatherReadLen_L,WeatherCRC_H,WeatherCRC_L};  //查询气象指令
uint8_t Rain_clear[8]={WeatherID,RS485WRITE_FUN_CODE,RS485WriteAdd_H,RS485WriteAdd_L,0x01,RainClearCRC_H,RainClearCRC_L};
uint16_t weathercrc;                  //气象CRC校验
char WeatherFlag;

/*****************************************************
打印数据  Weather：0xXX 0xXX形式
*****************************************************/
void Weather_Printf(void)
{
	printf("Weather:");
	for(uint8_t i=0;i<69;i++)
	printf("0x%02X ",weather_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取气象数据命令
*****************************************************/
void Weather_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为69位  
	rx_idex = 0;                   
	rx_max = 69;
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
	if(uart3_data[0]==WeatherID &&  uart3_data[1] ==RS485READ_FUN_CODE  &&  weathercrc == ((uart3_data[68] << 8) | uart3_data[67]))
	{
		memset(weather_receive,0,rx_max);
		Delay_us(10);
		WeatherFlag = FLAG_NORMAL;
		memcpy(weather_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.tempe.value =(float)(int32_t)((weather_receive[3]<<24)|(weather_receive[4]<<16)|(weather_receive[5]<<8)|weather_receive[6])/1000;       //摄氏度
		WeatherData.humi.value = (float)((weather_receive[7]<<24)|(weather_receive[8]<<16)|(weather_receive[9]<<8)|weather_receive[10])/1000;			 //%
		WeatherData.pressure.value =(float)((weather_receive[11]<<24)|(weather_receive[12]<<16)|(weather_receive[13]<<8)|weather_receive[14])/1000/1000;;    //kPa
		WeatherData.Lux.value = (float)((weather_receive[15]<<24)|(weather_receive[16]<<16)|(weather_receive[17]<<8)|weather_receive[18])/1000;        //LUX
		WeatherData.winddirection.value =	(float)((weather_receive[27]<<24)|(weather_receive[28]<<16)|(weather_receive[29]<<8)|weather_receive[30])/1000; 
		WeatherData.windspeed.value =	(float)((weather_receive[39]<<24)|(weather_receive[40]<<16)|(weather_receive[41]<<8)|weather_receive[42])/1000; 
		WeatherData.Rainfall.value =	(float)((weather_receive[43]<<24)|(weather_receive[44]<<16)|(weather_receive[45]<<8)|weather_receive[46])/1000;  
		memset(uart3_data,0,rx_max+1);
	}
	else
		WeatherFlag = WEATHER_STOP;
	
	WeatherData.tempe.flag = WeatherFlag;
  WeatherData.humi.flag = WeatherFlag;									
  WeatherData.Lux.flag = WeatherFlag;
  WeatherData.pressure.flag = WeatherFlag;
  WeatherData.windspeed.flag = WeatherFlag;
	WeatherData.winddirection.flag = WeatherFlag;
	WeatherData.Rainfall.flag =WeatherFlag;

}

/*****************************************************
清零累计降雨量命令
*****************************************************/
uint8_t RainClear_Cmd(){
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为69位  
	rx_idex = 0;                   
	rx_max = 8;
	//发送查询指令	
	RS485_Send(Rain_clear,8);
	//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的命令是否正确写入
	if(strncmp((const char *)Rain_clear,(const char *)uart3_data,6 ) == 0)
		return 0;
	return 1;
}

