// protocol_def.h
#ifndef __PROTOCOL_DEF_H
#define __PROTOCOL_DEF_H

#include <stdint.h>
#include <stdbool.h>

// 系统编码定义
#define SYSTEM_CODE_AIR_QUALITY     22

// 命令编码定义
#define CMD_DATA_UPLOAD            2011


#define LIGHT_RADIATION


// 状态标志定义
#define FLAG_NORMAL                'N'
#define FLAG_ABNORMAL              'A'
#define WEATHER_STOP               'B'


#pragma pack(1)
typedef struct {
    uint8_t year;   // 年 (00-99)
    uint8_t month;  // 月 (1-12)
    uint8_t day;    // 日 (1-31)
		uint8_t	week;
    uint8_t hour;   // 时 (0-23)
    uint8_t min;    // 分 (0-59)
    uint8_t sec;    // 秒 (0-59)
} DateTime;

typedef struct {
    float value;    // 数值
    char flag;      // 标志(N正常, B异常)
} DataItem;


typedef struct {
    DateTime dataTime;      // 数据采集时间
    uint8_t realFlag;       // 实时标志
    DataItem tempe;   			// 温度(a01001)
    DataItem humi;      		// 湿度(a01002)
    DataItem windspeed;     // 风速(a01007)
    DataItem Lux;   				// 光照度(a86007)
    DataItem winddirection;	// 风向(a01008)
    DataItem pressure;  		// 大气压(a01006)
    DataItem CO2;    				// 二氧化碳(a05001)
		DataItem Rainfall;			// 降雨量(A86002)
		DataItem LightRadiation;  //   (A86006)
} WeatherStationData;      	//实时监测数据结构体

typedef struct {
    char Lat[16];			//纬度
    char Lon[16];			//经度
}positioning_;        //经纬度信息


#pragma pack()

extern  WeatherStationData WeatherData;
extern	positioning_ positioning;
extern  DateTime RTCTime;
// CRC16计算函数
uint16_t CalculateCRC16(const uint8_t *data, uint16_t length);

//  数据上传函数
void WeatherData2Web( char *deviceId,  char *password);

void Location2Web(DateTime *dt,char *deviceId, char *password);
void CID2Web(DateTime *dt,char *deviceId, char *password, char *CID);
#endif



