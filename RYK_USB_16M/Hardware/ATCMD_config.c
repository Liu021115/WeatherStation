#include "ATCMD_config.h" 
#include "usart.h"
#include "time.h"
#include "W25Q128.h"
#include "24LC64F.h"
#include <stdlib.h>
#include <string.h>
#include "Delay.h"
#include "rtc.h"
#include "Weather.h"
#include "WindDirection.h"
#include "WindSpeed.h"
#include "Rainfall.h"
#include "rtc.h"
#include "Power_test.h"

volatile   RecvState at_state = AT_IDLE;				// 接收状态标志位			
volatile   PackProcess pack_state = PACK_WAIT;	// AT数据包处理状态标志位
volatile   AT_message uart2recAt;								// AT消息接收结构体
ATCMD_FLAG AtCMD_Flag;                          // AT参数标志位

/*****************************************************
	AT指令缓冲区复位函数  
*****************************************************/
void reset_at_state(void)
{
    for (uint8_t i = 0; i < AT_CMD_LEN; i++) {
        uart2recAt.Atcmd[i] = 0;  							// 逐字节操作，清空缓冲区，兼容volatile
    }
	  for (uint8_t i = 0; i < AT_PARAM_LEN; i++) {
        uart2recAt.Atparam[i] = 0;
    }
		for (uint8_t i = 0; i < buf_uart2.index; i++) {
        buf_uart2.buf[i] = 0;										//重置UART2缓冲区
		}
    uart2recAt.AtcmdIndex = 0;									//重置命令符和参数索引
    uart2recAt.AtparamIndex = 0;	
		buf_uart2.index = 0;												//重置UART2缓冲区索引		
		at_state = AT_IDLE;													//重置AT命令接收状态为空闲
		pack_state = PACK_WAIT;											//重置AT包处理状态为等待
}


void AT_Pack_CmdProcess(const char* recvCmd,char* IP,char* PT,char* MN,char* PS,DateTime *RTCTime)
{
	if(pack_state == PACK_PREPARED)
		{
				AtCMD_Flag = ERROR_CMD ;
        // 遍历映射表查找匹配命令
        for (uint8_t i = 0; i < CMD_FLAG_MAP_LEN; i++) {
            if (strcmp(recvCmd, cmdFlagMap[i].cmd) == 0) {
                AtCMD_Flag = cmdFlagMap[i].flag;  // 匹配成功，设置对应标志
                break;  // 找到后退出循环，提高效率
            }
        }

				AT_Pack_ParamProcess(AtCMD_Flag,IP,PT,MN,PS,RTCTime);
		}
}

/*****************************************************
	校准命令   
	参数: *RTCTime 时间结构体指针
	没有写入存储芯片！！
*****************************************************/
void ParamTimeProcess(DateTime *RTCTime)
{
	IWDG_ReloadCounter();																						// 喂狗	
	const char* receive = (const char*)uart2recAt.Atparam;
	if(strcmp(receive,"")==0 ){
		printf("AT+TIME:%04d-%02d-%02d %02d:%02d:%02d\r\n",
		  RTCTime->year+2000,RTCTime->month,RTCTime->day,RTCTime->hour,RTCTime->min,RTCTime->sec);				//打印RTC内部时间
		}
	else{
		unsigned int time =atoi(receive);
		printf("time:%d\r\n",time);
		/* 声明时间结构体 */
		struct tm * timeinfo;
		/* 将时间戳转换为时间，赋值给时间结构体 */
		timeinfo = localtime(&time);

		RTCTime->year = timeinfo->tm_year -100;            // 将数据保存到RTC结构体
		RTCTime->month = timeinfo->tm_mon + 1;
		RTCTime->day = timeinfo->tm_mday;
		RTCTime->hour = timeinfo->tm_hour;
		RTCTime->min = timeinfo->tm_min;
		RTCTime->sec = timeinfo->tm_sec;
				RTC_WriteDate(RTCTime);
		printf("NEW TIME:%4d-%02d-%02d %02d:%02d:%02d\r\n",
		  RTCTime->year+2000,RTCTime->month,RTCTime->day,RTCTime->hour,RTCTime->min,RTCTime->sec);				//打印RTC内部时间	

		}		
	reset_at_state();																		 // 清空缓冲区
}

/*****************************************************
	设置网络参数命令
	参数: IP ip地址缓冲区   PT 端口号缓冲区
*****************************************************/
void ParamIpProcess(char* IP,char* PT)
{
		IWDG_ReloadCounter();																						// 喂狗	
		const char* receive = (const char*)uart2recAt.Atparam;    
		if(strcmp(receive,"")==0 ){																			// 如果参数为空  判断命令为获取
			printf("ip1=%s;port1=%s;\r\n",IP,PT);													// 打印当前IP地址和端口号
		}
		else{																														// 参数不为空
			printf("param:%s\r\n",receive);																// 打印接收数据	
			char *ip_start = NULL;																				// 创建临时指针提取接收参数中的数据				
			char *ip_end = NULL;
			char *port_start = NULL;
			char *port_end = NULL;
			if((ip_start = strstr(receive,"ip1="))!= NULL){								// 获取ip地址参数开始地址
					ip_start += 4;
					if((ip_end = strchr(ip_start,';')) != NULL){							// 获取ip地址参数结束地址
						memset(IP,0,32);																				// 清空ip地址缓冲区
						char *passIP = IP;																			// 复制IP缓冲区地址用于参数传递
						while(ip_start<ip_end)																	// 传递ip参数字符串
							*passIP++ = *ip_start++;
					}
					printf("IPconfig:%s\r\n",IP);															// 传递完成后打印配置好的IP地址	
			}		
			if(ip_end != NULL){																						// 获取端口号参数开始地址
					port_start = ip_end+7;
					if((port_end = strchr(port_start,';')) != NULL){					// 获取端口号参数结束地址
						memset(PT,0,32);																				// 清空端口号地址缓冲区
						char *passPT = PT;																			// 复制端口号缓冲区地址用于参数传递
						while(port_start<port_end)															// 传递端口号参数字符串
							*passPT++ = *port_start++;
					}
					printf("PORTconfig:%s\r\n",PT);														// 传递完成后打印配置好的端口号
			}		
		}
		M24LC64F_WriteReg(EEPROM_IP,(uint8_t *)IP, 32);                 // 将新的IP和端口号写入EEPROM
		Delay_ms(10);																										
		M24LC64F_WriteReg(EEPROM_PORT, (uint8_t *)PT, 32);
		Delay_ms(10);
		reset_at_state();																								// 复位缓冲区和索引	
}	

/*****************************************************
	设置上传参数命令
	参数: MN 设备号地址缓冲区(最高32位)   PS 密码缓冲区
	可增加参数：  ST码  上传间隔时间
*****************************************************/
void ParamIdProcess(char* MN,char* PS)
{
		const char* receive = (const char*)uart2recAt.Atparam;    
		if(strcmp(receive,"")==0 ){																							  // 如果参数为空  判断命令为获取
			printf("mn=%s;st:%d;pwd=%s;min=5;ten=0;hour=0;day=0;\r\n",MN,22,PS);		// 打印当前设备号
		}
		else{																														   	      // 参数不为空
			printf("param:%s\r\n",receive);																     	    // 打印接收数据	
			char *mn_start = NULL;																				          // 创建临时指针提取接收参数中的数据				
			char *mn_end = NULL;        
			char *password_start = NULL;        
			char *password_end = NULL;
			if((mn_start = strstr(receive,"mn="))!= NULL){								        	// 获取设备号参数开始地址
					mn_start += 3;
					if((mn_end = strchr(mn_start,';')) != NULL){								        // 获取设备号参数结束地址
						memset(MN,0,32);																				       	  // 清空设备号缓冲区
						char *passMN = MN;																			       	  // 复制设备号缓冲区地址用于参数传递
						while(mn_start<mn_end)																						// 传递设备号参数字符串
							*passMN++ = *mn_start++;        
					}
					printf("MNconfig:%s\r\n",MN);															          // 传递完成后打印配置好的MN地址	
			}		        
			if((password_start = strstr(mn_end,"pwd="))!= NULL){										// 获取密码参数开始地址
					password_start += 4;        
					if((password_end = strchr(password_start,';')) != NULL){						// 获取密码参数结束地址
						memset(PS,0,32);																				        	// 清空密码缓冲区
						char *passPS = PS;																								// 复制密码缓冲区地址用于参数传递
						while(password_start<password_end)															  // 传递密码参数字符串
							*passPS++ = *password_start++;
					}
					printf("PWDconfig:%s\r\n",PS);														          // 传递完成后打印配置好的密码
			}		
		printf("The upload time(5 min) and ST(22) have not been adjusted\r\n");	
		}
		M24LC64F_WriteReg(EEPROM_ID,(uint8_t *)MN, 32);                           // 将新的设备号和密码写入EEPROM
		Delay_ms(10);																										
		M24LC64F_WriteReg(EEPROM_PASSWORD, (uint8_t *)PS, 32);
		Delay_ms(10);		 
		reset_at_state();																											  	// 复位缓冲区和索引	
		IWDG_ReloadCounter();																											// 喂狗		
} 

/*****************************************************
	GPS参数设置命令
	气象站暂未启动定时上传定位功能
*****************************************************/
void ParamGPSprocess(void)  																									//GPS上传频率 参数未启用
{
		IWDG_ReloadCounter();																											//喂狗
		printf("The parameter(GPS) has not been utilized.\r\n");									//打印提示 参数未启用
		reset_at_state();
}

/*****************************************************
	采样间隔设置命令
	该参数可选用 目前由程序直接写死
*****************************************************/
void ParamSampleProcess()  																										//传感器获取频率 参数未启用
{
		IWDG_ReloadCounter();																											//喂狗
		printf("The parameter(SAMPLE) has not been utilized.\r\n");								//打印提示 参数未启用
		reset_at_state();
}

/*****************************************************
	初始化存储器命令   擦除FLASH
	参数: Page FLASH目前写入页地址
*****************************************************/
void ParamEraseProcess(void)
{
	IWDG_ReloadCounter();															 //喂狗
	printf("ERASE START     ");
	W25Q128_ChipErase();        											 //擦除EEPROM芯片	
	printf("ERASE FINISH\r\n");
	reset_at_state();                                  // 复位缓冲区和索引
}

/*****************************************************
	测试存储器命令   读写FLASH测试
	读取0X00起始的8个字节数据
	重新写入8个字节数据并重新读取
	擦除写入数据
*****************************************************/
void ParamTestProcess(void)                
{
IWDG_ReloadCounter();																// 喂狗
	uint8_t EEPROM_rec[8]={0};													// 临时缓冲区
	W25Q128_ReadData(0x0,EEPROM_rec,8);									// 读取0X00原始数据
	printf("Original Data:");														// 十六进制打印	
	for(int i =0;i<8;i++)
		printf("%02x ",EEPROM_rec[i]);
	printf("\r\n");
	W25Q128_SectorErase(0x0);														// 擦除扇区 
	uint8_t StringW[9] ="12345678";
	W25Q128_PageProgram(0x0,StringW,8);                 // 0x00写入数据
	memset(EEPROM_rec,0x0,8);														// 清空接收缓冲区	
	W25Q128_ReadData(0x0,EEPROM_rec,8);									// 读取写入0x00地址数据 
	printf("Read Written Data:");												// 十六进制打印
	for(int i =0;i<8;i++)
		printf("%02x ",EEPROM_rec[i]);
	printf("\r\n");
	if(EEPROM_rec[0] == '1'  &&  EEPROM_rec[7] == '8')	// 判断写入首位,末位是否一致
		printf("Test Ok\r\n");
	else
		printf("Test Error\r\n");
	W25Q128_SectorErase(0x0);														// 写入扇区进行擦除
	reset_at_state();                                   // 复位缓冲区和索引
	IWDG_ReloadCounter();																// 喂狗
}

/*****************************************************
	打印RS485传感器数据
*****************************************************/
void PritfRS485Process(void)                
{
	IWDG_ReloadCounter();																// 喂狗
	Weather_Printf();
	Delay_ms(100);
	WindSpeed_Printf();
	Delay_ms(100);
	WindDirection_Printf();
	Delay_ms(100);
	Rainfall_Printf();
	reset_at_state();                                   // 复位缓冲区和索引
	IWDG_ReloadCounter();																// 喂狗	
}

/*****************************************************
	打印RS485传感器数据
*****************************************************/
void BATTERY_CHECKProcess(void)                
{
	IWDG_ReloadCounter();																// 喂狗
	float battery_power = PowerValue_GET();							// 获取电压
	printf("Battery:%.6f\r\n",battery_power);						// 打印电压信息
	reset_at_state();                                   // 复位缓冲区和索引
	IWDG_ReloadCounter();																// 喂狗	
}

/*****************************************************
	初始化存储器命令   擦除FLASH
	参数: Page FLASH目前写入页地址
*****************************************************/
void AT_Pack_ParamProcess(ATCMD_FLAG AtCMD_Flag,char* IP,char* PT,char* MN,char* PS,DateTime *RTCTime)
{
	switch(AtCMD_Flag){
		case	TIME_CONFIG:ParamTimeProcess(RTCTime);break;
		case	IP_CONFIG:ParamIpProcess(IP,PT);break;
		case	ID_CONFIG:ParamIdProcess(MN,PS);break;
		case	GPS_CONFIG:ParamGPSprocess();break;
		case	SAMPLE_CONFIG:ParamSampleProcess();break;
		case	ERASE_EEPROM:ParamEraseProcess();break;
		case	TEST_EEPROM:ParamTestProcess();break;
		case	ERROR_CMD:reset_at_state();break;
		case  RS485_DATA:PritfRS485Process();break;
		case  BATTERY: BATTERY_CHECKProcess();break;
	}
}
