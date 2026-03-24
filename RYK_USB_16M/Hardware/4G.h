#ifndef __BC26_H
#define __BC26_H
#include "usart.h"
#include <stm32f10x.h>
#include "delay.h"
#include "protocol_def.h"
void Clear_Buffer(void);//Çå¿Õ»º´æ
void CSTX_4G_Init(void);
void CSTX_4G_PDPACT(void);
void CSTX_4G_ConTCP(void);
void CSTX_4G_CreateTCPSokcet(char *AdressIP,char *SERVERPORT);
void CSTX_4G_Senddata(uint8_t *len, const char *data); //·¢ËÍ×Ö·û´®Êý¾Ý
void CSTX_4G_CreateSokcet(void);
void Clear_Buffer(void);
void CSTX_4G_ChecekConStatus(void);
void CSTX_4G_RECTCPData(void);
void CSTX_ClkUpdate(DateTime	*RTCTime);
void parse_lat_lon(char *input, char *latitude, char *longitude);
void Get_locate_information(void);

void GNSS_lat_lon(char *input, char *latitude, char *longitude);
void CSTX_ReadCID(char *CID);
void CSTX_CNSSUpdate(DateTime	*RTCTime);

void CSTX_check(void);

void GPRS_GPIO_Init(void);
void GPRS_OFF(void);
void GPRS_ON(void);





#endif







