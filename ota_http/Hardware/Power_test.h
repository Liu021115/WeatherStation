#ifndef __POWER_TEST_H
#define __POWER_TEST_H

typedef enum{
	OPEN_4G,				 // ¿ªÆô4G
	CLOSE_4G				 // ¹Ø±Õ4G
} StateOfBattrey;

void AD_Init(void);
float PowerValue_GET(void);
void HardWareFlagChange(float Battery_V);
uint8_t WebVoltageCheck(void);


#endif
