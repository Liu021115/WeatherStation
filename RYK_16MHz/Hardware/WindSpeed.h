#ifndef __WINDSPEED_H_
#define __WINDSPEED_H_

#include "stm32f10x.h"
#include "RS485.h"
#include "string.h"
#include <stdio.h>

void WindSpeed_Cmd(void);
void WindSpeed_Printf(void);
void WindSpeed_Printf0x(void);
#endif
