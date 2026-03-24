#ifndef __WEATHER_H
#define __WEATHER_H

#include "stm32f10x.h"
#include "RS485.h"
#include "string.h"
#include <stdio.h>






void Weather_Printf(void);
void Weather_Cmd(void);
uint8_t RainClear_Cmd(void);

#endif
