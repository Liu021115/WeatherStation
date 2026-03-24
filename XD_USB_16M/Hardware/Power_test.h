#ifndef __POWER_TEST_H
#define __POWER_TEST_H

typedef enum{
	ALL_OPEN,				 // 开启所有模块
	CLOSE_4G,				 // 关闭4G
	CLOSE_ALL				 // 关闭4G和RS485
} StateOfBattrey;

void AD_Init(void);
float PowerValue_GET(void);
void HardWareFlagChange(float Battery_V);

extern StateOfBattrey StateOfHarfWare;
#endif
