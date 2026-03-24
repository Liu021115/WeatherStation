#include "LightRadiation.h"
#include "protocol_def.h"
/*****************************************************
下面就是需要修改的地方，修改设备地址、功能码、CRC  
*****************************************************/
#define LightRadiationID 0x51
#define LightRadiationRead 0x03
#define LightRadiationCRC_H 0x88
#define LightRadiationCRC_L 0x5A

#define WEATHER_WAIT_MAX	300
#define WEATHER_WAIT_STEP	20 

uint8_t lightradiation_receive[7];      //光总辐射接收缓冲区
uint8_t lightradiation_send[8]={LightRadiationID,LightRadiationRead,0x00,0x00,0x00,0x01,LightRadiationCRC_H,LightRadiationCRC_L};  //查询风向指令
uint16_t LightRadiacrc;                  //光总辐射CRC校验
char LightRadiaFlag;

/*****************************************************
打印数据  WindDirection：0xXX 0xXX形式
*****************************************************/
void Light_radiation_Printf(void)
{
	printf("LightRadiation:");
	for(uint8_t i=0;i<7;i++)
	printf("0x%02X ",lightradiation_receive[i]);
	printf("\r\n");
}

/*****************************************************
获取风向数据命令
*****************************************************/
void Light_radiation_Cmd(void)
{
	uint16_t wait_time = 0;   //延时等待时间
	//将RS485接收缓冲区设为7位  
	rx_idex = 0;                   
	rx_max = 7;
	//发送查询指令	
	RS485_Send(lightradiation_send,8);
		//循环等待接收完成
	while(rx_idex < rx_max-1 &&	wait_time < WEATHER_WAIT_MAX){
		Delay_ms(WEATHER_WAIT_STEP);
		wait_time += WEATHER_WAIT_STEP;
	}
	//判断接收的CRC校验位	
	LightRadiacrc = ModbusCRC16(uart3_data,rx_max-2);

	//数据copy 
	if(uart3_data[0]==LightRadiationID &&  uart3_data[1] ==LightRadiationRead  && LightRadiacrc == ((uart3_data[6] << 8) | uart3_data[5]))  //简单的校验
	{
		memset(lightradiation_receive,0,rx_max);
		Delay_us(10);
		LightRadiaFlag = FLAG_NORMAL;
		memcpy(lightradiation_receive,uart3_data,rx_max);
		Delay_us(10);
		WeatherData.LightRadiation.value = (float)((lightradiation_receive[3]<<8)|lightradiation_receive[4]);  //风向角度转换 			
		memset(uart3_data,0,rx_max+1);
	}
	else
		LightRadiaFlag = WEATHER_STOP;
	WeatherData.LightRadiation.flag = LightRadiaFlag;	

}



