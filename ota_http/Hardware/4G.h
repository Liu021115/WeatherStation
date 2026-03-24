#ifndef __4G_H
#define __4G_H
#include "usart.h"
#include <stm32f10x.h>
#include "delay.h"
#include <stdbool.h>

void Clear_Buffer(void);//清空缓存
void CSTX_4G_Init(void);
void CSTX_HTTP_CLOSE(void);

void CSTX_4G_Senddata(uint8_t *len, const char *data); //发送字符串数据
void CSTX_4G_RECTCPData(void);
void GPRS_GPIO_Init(void);
void GPRS_OFF(void);
void GPRS_ON(void);

// 缓冲区大小定义
//#define BUFLEN2 2048
#define FLASH_PAGE_SIZE 2048  // STM32F105一页=2KB


extern char ATSTR[BUFLEN];
extern int errcount;


bool	CSTX_HTTP_GetFileList(void);
bool CSTX_HTTP_DownloadFile(const char* filename, uint32_t flash_start_addr);
bool DownloadAndUpdateFlash(void);


extern	uint8_t HttpVersion[32];                  // 设备版本  起始地址0x300   长度正常为6-10
extern	uint8_t AdreeMN[32];							//设备MN码，用于识别版本使用

#endif







