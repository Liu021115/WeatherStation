#ifndef __ATCMD_CONFIG_H
#define __ATCMD_CONFIG_H

#include "stm32f10x.h"   
#include "protocol_def.h"

//定义接收状态相关
#define AT_CMD_LEN 16            // 命令类型缓冲区
#define AT_PARAM_LEN 128				 //	命令参数缓冲区 


typedef enum {
    AT_IDLE,   				  // 空闲
    AT_A_RECEIVED,      // 收到'A'
    AT_AT_RECEIVED,			// 收到'AT'
		AT_CMD_RECEIVE,			// 接收命令符
		AT_PARAM_RECEIVE,		// 接收参数
		AT_COMPLETE					// 接收完成'\r\n'
} RecvState;

typedef enum {
	PACK_WAIT,            // 等待
	PACK_PREPARED					// 准备就绪
} PackProcess ;

typedef enum {
	ERROR_CMD,
	TIME_CONFIG,
	IP_CONFIG,
	ID_CONFIG,
	GPS_CONFIG,
	SAMPLE_CONFIG,
	ERASE_EEPROM,
	TEST_EEPROM,
	RS485_DATA,
	BATTERY
} ATCMD_FLAG;

typedef struct {
    const char* cmd;       // 命令字符串
    ATCMD_FLAG flag;          // 对应的标志值
} CmdFlagMap;

static const CmdFlagMap cmdFlagMap[] = {
    {"GPSCFG",   GPS_CONFIG},
    {"SAMPLE",   SAMPLE_CONFIG},
    {"TIME",     TIME_CONFIG},    // 假设时间配置命令是"TIME"
    {"QCFG",       IP_CONFIG},      // 假设IP配置命令是"IP"
    {"UPLOAD",       ID_CONFIG},    // 假设ID配置命令是"ID"
    {"erase",    ERASE_EEPROM},   // 假设擦除EEPROM命令是"ERASE"
    {"test",     TEST_EEPROM},     // 假设测试EEPROM命令是"TEST"
		{"RS485DATA", RS485_DATA},			
		{"BATTERY", 	BATTERY},				
		{"BATTERY\r\n", BATTERY},		
		{"RS485DATA\r\n", RS485_DATA},
    {"GPSCFG\r\n",   GPS_CONFIG},
    {"SAMPLE\r\n",   SAMPLE_CONFIG},
    {"TIME\r\n",     TIME_CONFIG},    // 假设时间配置命令是"TIME"
    {"QCFG\r\n",       IP_CONFIG},      // 假设IP配置命令是"IP"
    {"UPLOAD\r\n",       ID_CONFIG},    // 假设ID配置命令是"ID"
    {"erase\r\n",    ERASE_EEPROM},   // 假设擦除EEPROM命令是"ERASE"
    {"test\r\n",     TEST_EEPROM}    // 假设测试EEPROM命令是"TEST"		
};

#define CMD_FLAG_MAP_LEN (sizeof(cmdFlagMap) / sizeof(cmdFlagMap[0]))

typedef struct{
	char Atcmd[AT_CMD_LEN];
	char Atparam[AT_PARAM_LEN];
	uint8_t AtcmdIndex ;
	uint8_t AtparamIndex ;
} AT_message;

extern volatile   RecvState at_state;
extern volatile   PackProcess pack_state;
extern volatile   AT_message uart2recAt;
extern ATCMD_FLAG AtCMD_Flag;

void reset_at_state(void);
void AT_Pack_CmdProcess(const char* recvCmd,char* IP,char* PT,char* MN,char* PS,DateTime *RTCTime);
void AT_Pack_ParamProcess(ATCMD_FLAG AtCMD_Flag,char* IP,char* PT,char* MN,char* PS,DateTime *RTCTime);

#endif
