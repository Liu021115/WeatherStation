#include "stm32f10x.h"                  // Device header
#include "4G.h"
#include "Delay.h"
#include "string.h"
#include "usart.h"
#include "time.h"
#include <stdlib.h>

char *strx;

int  errcount = 0;	//失败次数 防止死循环
char ATSTR[BUFLEN];	//组建AT命令的函数

int len_buf212;   //缓冲数据长度

typedef enum {
	TCP_OK,      // TCP连接OK 
	TCP_ERROR		 // TCP连接错误
} TCPState;    // TCP连接状态

TCPState  TcpState = TCP_OK;

/*****************************************************
辅助函数：在指定长度内查找最后一个','
*****************************************************/
char* My_strrchr(const char *str, int c, size_t max_len) {
    const char *last = NULL;
    for (size_t i = 0; i < max_len && str[i] != '\0'; i++) {
        if (str[i] == c) {
            last = &str[i];
        }
    }
    return (char*)last;
}

/*****************************************************
清空模块反馈的信息
*****************************************************/
void Clear_Buffer(void)									
{
    //printf(buf_uart1.buf);							// 打印缓冲区
    Delay_ms(100);												
    buf_uart1.index = 0;								// 清空缓冲区索引
    memset(buf_uart1.buf, 0, BUFLEN);		// 清空缓冲区
}


/*****************************************************
初始化模块 和单片机连接，获取卡号和信号质量
*****************************************************/
void CSTX_4G_Init(void)
{
		IWDG_ReloadCounter();																							//喂狗
    Uart1_SendStr("AT+CPIN?\r\n");																		//查询SIM卡是否已准备就绪
    Delay_ms(1000);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CPIN: READY"); //准备就绪
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CPIN?\r\n");																//获取卡号，类似是否存在卡的意思，比较重要。
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CPIN: READY"); //返回OK,说明卡是存在的
    }
		printf("****SIM  READY****\r\n");																					//打印提示		SIM准备就绪
	  Clear_Buffer();																										//清空缓冲
		
    Uart1_SendStr("AT+CSQ\r\n");																			//查询信号强度
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CSQ:"); 	//返回信号强度
    if(strx)	
       printf("****CSQ:%s****\r\n", buf_uart1.buf + 14);         			//打印信号强度
    Clear_Buffer();

    Uart1_SendStr("AT+CGREG?\r\n");																		//查询网络是否附着上
    Delay_ms(2000);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,1"); //必须判断0,1或0,5才是正确的
		if(strx == NULL)
			strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,5"); //如果没有返回0,1则判断0,5
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGATT?\r\n");																//获取激活状态
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGATT: 1"); 
				if(strx == NULL)
					strstr((const char*)buf_uart1.buf, (const char*)"+CGREG: 0,5"); //如果没有返回0,1则判断0,5
    }
    Clear_Buffer();
}



/*****************************************************
关闭之前存在的和服务器的链接 
反馈失败  之前未建立socket连接
*****************************************************/
void CSTX_4G_ConTCP(void)
{
		Uart1_SendStr("AT+CIPCLOSE=0\r\n");//关闭socekt连接
		Delay_ms(100);
    Clear_Buffer();										 //清空缓冲区				
}


/*****************************************************
建立TCP链接
*****************************************************/
void CSTX_4G_CreateTCPSokcet(char *AdressIP,char *SERVERPORT )
{
    Uart1_SendStr("AT+NETOPEN\r\n");																				//启动TCP服务
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+NETOPEN: 0");	//等待返回
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+NETOPEN\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"Network is already opened"); //返回网络已经建立
    }	
    printf("****TCP  ENABLE****\r\n");																						  		//打印提示
    memset(ATSTR, 0, BUFLEN);																										//清空缓冲区
		sprintf(ATSTR, "AT+CIPOPEN=0,\"TCP\",\"%s\",%s\r\n", AdressIP, SERVERPORT);	//指令打印到缓冲区
    Uart1_SendStr(ATSTR);																												//创建连接TCP,输入IP以及服务器端口号码
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"++CIPOPEN: 0,0"); 	//检查是否登陆成功
    errcount = 0;																																//错误计数
    while(strx == NULL)
    {
        errcount++;
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CIPOPEN: 0,0"); //检查是否登陆成功
        Delay_ms(200);
        if(errcount > 100)   																										 //超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
            break;
        }
    }
    Clear_Buffer();																															//清空缓冲区
		IWDG_ReloadCounter();																												//喂狗
}

/*****************************************************
发送数据函数
参数一: *len 数据长度
参数二: *data 字符串数据
*****************************************************/
void CSTX_4G_Senddata(uint8_t *len, const char *data) 
{
		
    memset(ATSTR, 0, BUFLEN);                                      //清空缓冲区
    sprintf(ATSTR, "AT+CIPSEND=0,%s\r\n", len);										 //数据加载到发送缓冲区 
    Uart1_SendStr(ATSTR);																					 //发送数据
    Delay_ms(500);
    //等待模块反馈 >
    strx = strstr((const char*)buf_uart1.buf, (const char*)">");   //模块反馈可以发送数据了
		errcount = 0;																									 //错误计数
    while(strx == NULL)
    {
        errcount++;
        strx = strstr((const char*)buf_uart1.buf, (const char*)">"); //模块反馈可以发送数据了
				Delay_ms(1000);
        if(errcount > 10)   																				 //防止死循环跳出
        {
            errcount = 0;
						TcpState = TCP_ERROR;																			//	TCP状态标志设为TCP错误
            break;
        }
    }

    Uart1_SendStr((char *)data);																			//	发送真正的数据
    Delay_ms(500);

    strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); 		//	检查是否发送成功
    errcount = 0;
    while(strx == NULL)
    {
        errcount++;
        strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); //	检查是否发送成功
        Delay_ms(1000);
        if(errcount > 10)   																					//	超时退出死循环 表示服务器连接失败
        {
            errcount = 0;
						TcpState = TCP_ERROR;																			//	TCP状态标志设为TCP错误
            break;
        }
    }
    Clear_Buffer();
	if(TcpState == TCP_ERROR){																					//	判断如果是TCP连接误断开无法排除原因,
		Delay_s(30);																											//	延时三十秒直接触发看门狗重启
	}
}

/*****************************************************
检测网络连接状态函数
*****************************************************/
void CSTX_check(void)
{
	  Uart1_SendStr("AT+CIPOPEN?\r\n");																				//启动TCP服务
    Delay_ms(500);
		printf("%s\r\n",buf_uart1.buf);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"+CIPOPEN:");	//等待返回
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+NETOPEN\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"Network is already opened"); //返回网络已经建立
    }	
	}

/*****************************************************
收到服务器下发的数据就直接打印
*****************************************************/
void CSTX_4G_RECTCPData(void)
{
    strx = strstr((const char*)buf_uart1.buf, (const char*)"RECV FROM:"); //返回+QIURC:，表明接收到TCP服务器发回的数据
    if(strx)
    {
			 printf("%s\r\n",buf_uart1.buf);        //打印数据
       Clear_Buffer();
    }
}


/*****************************************************
读取SIM卡的CID
*****************************************************/
void CSTX_ReadCID(char *CID)
{
		Uart1_SendStr("AT+CICCID\r\n");																		//从SIM获取ICCID
    Delay_ms(500);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"+ICCID:");//获取成功	
    while(strx == NULL)
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CICCID\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+ICCID:");
    }

		char *CID_index;																										//	ICCID数据索引
		if((CID_index = strstr(buf_uart1.buf,"CID:"))!= NULL)
			CID_index += 5;	
		memset(CID,0,21);
		char *passCID = CID;																			       	  // 复制设备号缓冲区地址用于参数传递
		uint8_t PassIdex = 0;
			while(PassIdex<20){																								// 传递设备号参数字符串
							*passCID++ = *CID_index++;    
							PassIdex++;
					}
		Clear_Buffer();
}

/*****************************************************
通过GNSS获取定位
*****************************************************/
void CSTX_CNSSUpdate(DateTime	*RTCTime)
{
		IWDG_ReloadCounter();																							//喂狗
    Uart1_SendStr("AT\r\n");																					//发送测试指令
    Delay_ms(500);
    strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); 		//返回OK
    while(strx == NULL)																								//循环发送等待返回成功
    {
        Clear_Buffer();
        Uart1_SendStr("AT\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"OK"); 
    }
    printf("****AT  TEST  OK****\r\n");																//打印提示 AT指令测试成功
	  Clear_Buffer();																										//清空缓冲
		
		Uart1_SendStr("AT+CGNSSPWR?\r\n");																			//GNSS电源判断
    Delay_ms(500);																											
		strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGNSSPWR:");		//判断合理返回值
    while(strx == NULL)																											//等待合理的返回值
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSPWR?\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGNSSPWR:");
    }
		strx = strstr((const char*)buf_uart1.buf, (const char*)"+CGNSSPWR: 1");	//判断是否GNSS电源已开启
		if(strx == NULL){																												//如果已开启则跳过
    Clear_Buffer();																													//清空缓冲
		Uart1_SendStr("AT+CGNSSPWR=1\r\n");																			//打开电源
    Delay_ms(3000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"OK");						//开启成功
    while(strx == NULL)																											//循环发送等待开启电源成功
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSPWR=1\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"READY!");
    }
    Clear_Buffer();																													 //清空缓冲				
		}

		IWDG_ReloadCounter();																										 //喂狗
		Uart1_SendStr("AT+CAGPS\r\n");																					 //获取AGPS数据辅助定位
    Delay_ms(3000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"success");			 //等待返回成功
		errcount = 0;																														 //错误计数置零
    while(strx == NULL)																											 //循环发送等待返回成功		
    {
				errcount ++;																												 //错误计数+1
        Clear_Buffer();
        Uart1_SendStr("AT+CAGPS\r\n");
        Delay_ms(3000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"success");
				if( errcount == 7 )
					IWDG_ReloadCounter();																								//定时喂狗
				else if( errcount == 14 )
					break;																															//一直获取不到则退出循环
    }
    Clear_Buffer();																														//清空缓冲			
		IWDG_ReloadCounter();																											//喂狗

		Uart1_SendStr("AT+CGPSHOT\r\n");																					//开启GNSS热启动
    Delay_ms(2000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"OK");							//等待返回成功
    while(strx == NULL)																												//循环发送等待返回成功
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGPSHOT\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"OK");
    }	
		Clear_Buffer();	
			
		Uart1_SendStr("AT+CGNSSINFO=0\r\n");																			//设置返回单次定位信息
    Delay_ms(500);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"OK");							//等待返回成功
    while(strx == NULL)																												//循环发送等待返回成功	
    {
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSINFO=0\r\n");
        Delay_ms(500);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"OK");
    }
	  Clear_Buffer();																														//清空缓冲	
		
		Uart1_SendStr("AT+CGNSSINFO\r\n");																				//获取定位信息
    Delay_ms(3000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");							//等待返回定位信息，借助'N'北半球判断	
		errcount = 0;																															//错误计数清零	
    while(strx == NULL)																												//循环发送等待返回成功
    {
				errcount ++;																													//错误计数+1
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSINFO\r\n");																		
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");
				if( errcount == 10  || errcount == 30  || errcount == 50  ||  errcount == 70 ||  errcount == 90  ||  errcount == 110  ||  errcount == 130)
					IWDG_ReloadCounter();																								//定期喂狗
				else if(errcount == 150){
					IWDG_ReloadCounter();
					return;																															//一分半以上没有获取到定位退出函数
				}
    }
		IWDG_ReloadCounter();																											//喂狗
		float Lat[3]={0};																													//设置flaot数组存放三组获取数据
		float Lon[3]={0};
		char *lastIndex = NULL;																										//设置指针用于提取经纬度信息
		char * LonIndex = NULL;																										
		lastIndex = My_strrchr(buf_uart1.buf, ',', strx - buf_uart1.buf-2); 			//定位数据信息，纬度之前的,
		if (lastIndex != NULL) {																									//指针非空说明信息正确
					lastIndex++;  																											//跳过','，指向纬度第一个字符
					Lat[0] = atof(lastIndex);  																					//atof会自动在非数字处（这里是',')停止
			}
		LonIndex = strx +	2;																											//获取经度信息定位
		Lon[0] = atof(LonIndex);																									//将字符串转换成浮点数保存到缓冲区
		lastIndex = NULL;																													//清空指针防止崩溃
		LonIndex = NULL;																													//清空指针防止崩溃
		Clear_Buffer();																														//清空缓冲区

					
		Uart1_SendStr("AT+CGNSSINFO\r\n");																				//获取定位信息
    Delay_ms(1000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");							//等待返回定位信息，借助'N'北半球判断
		errcount = 0;																															//清空错误计数		
    while(strx == NULL)																												//循环发送等待获取定位信息成功	
    {
				errcount ++;																													//错误计数+1
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSINFO\r\n");
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");				
				if( errcount == 10){																									//获取定位失败10次
					return;																															//退出函数
				}
    }				
		lastIndex = My_strrchr(buf_uart1.buf, ',', strx - buf_uart1.buf-2);				
		if (lastIndex != NULL) {
            lastIndex++;  																										// 跳过','，指向纬度第一个字符
            Lat[1] = atof(lastIndex);  																				// atof会自动在非数字处（这里是',')停止
        }
	
		LonIndex = strx +	2;		
		Lon[1] = atof(LonIndex);
		lastIndex = NULL;
		LonIndex = NULL;				
	  Clear_Buffer();
					
		Uart1_SendStr("AT+CGNSSINFO\r\n");																				//获取定位信息
    Delay_ms(1000);
		strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");							//等待返回定位信息，借助'N'北半球判断
		errcount = 0;																															//清空错误计数		
    while(strx == NULL)																												//循环发送等待获取定位信息成功	
    {
				errcount ++;																													//错误计数+1
        Clear_Buffer();
        Uart1_SendStr("AT+CGNSSINFO\r\n");
        Delay_ms(1000);
        strx = strstr((const char*)buf_uart1.buf, (const char*)"N,");				
				if( errcount == 10){																									//获取定位失败10次
					return;																															//退出函数
				}
    }					
		lastIndex = My_strrchr(buf_uart1.buf, ',', strx - buf_uart1.buf-2);
		if (lastIndex != NULL) {
            lastIndex++; 																											// 跳过','，指向纬度第一个字符
            Lat[2] = atof(lastIndex);  																				// atof会自动在非数字处（这里是',')停止
        }	
		LonIndex = strx +	2;		
		Lon[2] = atof(LonIndex);

		char *TimeString = strstr((const char*)LonIndex, (const char*)"E") + 2;		//获取时间数据指针
		if(TimeString != NULL){
			int ddmmyy = atoi(TimeString);																					//将字符串转换成int类型数据，格式ddmmyy
			int hhmmss = atoi(TimeString + 7);																			//将字符串转换成int类型数据，格式hhmmss																								
			struct tm tm_time={0};
			tm_time.tm_year = ddmmyy % 100 + 100; 		//取余获取年份     1990开始
			tm_time.tm_mon = ddmmyy / 100 % 100 - 1;  //获取月份   月份从0开始（0=1月，11=12月）
			tm_time.tm_mday = ddmmyy / 10000;					//获取日期
			tm_time.tm_hour = hhmmss / 10000 + 8;			//获取小时 东八区+8
			tm_time.tm_min = hhmmss / 100 % 100;			//获取分
			tm_time.tm_sec = hhmmss % 100;						//取余获取秒
			time_t timeUTC = mktime(&tm_time);				//校准并转化为时间戳
			struct tm* local_tm = localtime(&timeUTC);
			if (local_tm == NULL) {
				printf("****RTC UPDATE ERROR****\r\n");
        return ; // 转换失败
			}
			// 提取并修正时间字段（struct tm的特殊定义）
			RTCTime->year = local_tm->tm_year - 100;   // tm_year是自1900年的偏移量
			RTCTime->month = local_tm->tm_mon + 1;     // tm_mon是0-11（对应1-12月）
			RTCTime->day = local_tm->tm_mday;          // 日期（1-31）
			RTCTime->hour = local_tm->tm_hour;         // 小时（0-23）
			RTCTime->min = local_tm->tm_min;        	 // 分钟（0-59）
			RTCTime->sec = local_tm->tm_sec;       	 	 // 秒（0-59）
		}
		

		u8 RightPosition = 0;																											//判断三次获取数据正常次数
		
		for(int i = 0;i<3;i++){
			if(Lat[i] != 0 && Lon[i] != 0)
					RightPosition ++;
				}
		if(RightPosition == 3){																										//三次数据获取正常，则取平均更新定位信息	
			sprintf(positioning.Lat,"%.6f",(Lat[0]+Lat[1]+Lat[2])/3);
			sprintf(positioning.Lon,"%.6f",(Lon[0]+Lon[1]+Lon[2])/3);
			printf("****GNSS GET****\r\n");
		}
		lastIndex = NULL;																													//释放指针
		LonIndex = NULL;																													//释放指针
	  Clear_Buffer();																														//清空缓冲		
		IWDG_ReloadCounter();																											//喂狗
}


/*****************************************************
	函数: 4G模块PWRKEY与RESET引脚初始化
	参数: 无
	说明: 需紧跟JTAC失能防止定电平脉冲开机
*****************************************************/
void GPRS_GPIO_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	//使能GPIOC
	GPIO_InitTypeDef GPIO_InitStructure;		
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	GPIO_ResetBits(GPIOA,GPIO_Pin_15);										//拉高RESET

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);			
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);										//拉高PWRKEY
	
}

/*****************************************************
	函数: 4G关机
	参数: 无
	说明: 4G模块关机时序，PWRKEY>2.5s的低电平脉冲
*****************************************************/
void GPRS_OFF(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_10);				// 拉低PWRKEY
	Delay_ms(3000);  												// 延时3s确保稳定		
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);			// 拉高PWRKEY
}

/*****************************************************
	函数: 4G开机
	参数: 无
	说明: 4G模块开机时序，PWRKEY>50ms的低电平脉冲
*****************************************************/
void GPRS_ON(void)
{
	GPIO_SetBits(GPIOC,GPIO_Pin_10);			// 拉低PWRKEY
	Delay_ms(100);  											// 延时100ms确保稳定		
	GPIO_ResetBits(GPIOC,GPIO_Pin_10);		// 拉高PWRKEY
}
