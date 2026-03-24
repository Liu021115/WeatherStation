#ifndef __SD3078_H
#define __SD3078_H
#include "protocol_def.h"
void ModifyTime(uint8_t year,uint8_t mon,uint8_t day,uint8_t hou,uint8_t min,uint8_t sec);
void SD3078_Init(void);
int ReadTime_SD3078(	DateTime *dt);
#endif
