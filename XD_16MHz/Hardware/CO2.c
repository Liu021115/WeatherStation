#include "CO2.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define CO2ID 0x2D
#define	CO2Read_FUN_CODE 0x03
#define RS485ReadAdd_H 	 0x00
#define RS485ReadAdd_L   0x00
#define CO2ReadLen_H	 0x00
#define CO2ReadLen_L	 0x01
#define CO2CRC_H 0x83
#define CO2CRC_L 0xA6

#define WEATHER_WAIT_MAX	300
#define WEATHER_WAIT_STEP	20 

uint8_t CO2_receive[7];      //二氧化碳接收缓冲区
uint8_t CO2_send[8]={CO2ID,CO2Read_FUN_CODE,RS485ReadAdd_H,RS485ReadAdd_L,CO2ReadLen_H,CO2ReadLen_L,CO2CRC_H,CO2CRC_L};  //查询二氧化碳指令
uint16_t CO2crc;                  //风速CRC校验
char CO2SFlag;

/*****************************************************
打印数据  WindSpeed：0xXX 0xXX形式
*****************************************************/
void CO2_Printf(void)
{
	printf("CO2:");
	for(uint8_t i=0;i<7;i++)
	printf("0x%02X ",CO2_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取风速数据命令
*****************************************************/
void CO2_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为7位  
	rx_idex = 0;                   
	rx_max = 7;
	//发送查询指令	
	RS485_Send(CO2_send,8);
	//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	CO2crc = ModbusCRC16(uart3_data,rx_max-2);

	//数据copy
	if(uart3_data[0]==CO2ID &&  uart3_data[1] ==CO2Read_FUN_CODE  &&  CO2crc == ((uart3_data[6] << 8) | uart3_data[5]))
	{
		memset(CO2_receive,0,rx_max);
		Delay_us(10);
		CO2SFlag = FLAG_NORMAL;
		memcpy(CO2_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.CO2.value =(float)((CO2_receive[3]<<8)|CO2_receive[4]);       //二氧化碳
		memset(uart3_data,0,rx_max+1);
	}
	else
		CO2SFlag = WEATHER_STOP;
	
	WeatherData.CO2.flag =CO2SFlag;
 
}


