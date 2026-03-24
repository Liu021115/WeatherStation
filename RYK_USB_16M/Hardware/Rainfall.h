#ifndef __RAINFALL_H_
#define __RAINFALL_H_

#include "stm32f10x.h"
#include "RS485.h"
#include "string.h"
#include <stdio.h>
#include "protocol_def.h"

void Rainfall_Cmd(void);
void RainClear_Cmd(void);
void Rainfall_Printf(void);
void RainTimeUpdate_Cmd(DateTime *RTCTime);


#endif
