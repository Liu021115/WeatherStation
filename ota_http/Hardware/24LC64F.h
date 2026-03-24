#ifndef __24LC64F_H
#define __24LC64F_H

//定义各部分在EEPROM的位置
#define EEPROM_ID 0X00             // 设备ID  mn码
#define EEPROM_PASSWORD 0X20			 //	设备密码 123456
#define EEPROM_IP 0X40						 // 服务器IP地址
#define EEPROM_PORT 0X60					 // 服务器端口号
#define EEPROM_ST  0x80						 // ST码 根据协议对应	
#define EEPROM_SENDWEB 0X85				 // 上报网络时间间隔	 常用5分钟
#define EEPROM_GETDATA 0X90        // 传感器采样时间间隔 默认不使用  
#define PAGEADDRESS  0X100 				 // EEPROM写地址	
#define EEPROM_LAT   0X200				 // 经度信息存储	
#define EEPROM_LON   0X220				 // 纬度信息存储	
#define EEPROM_VERSION_CURRENT  0x300		//版本信息存储

void M24LC64F_Init(void);
uint8_t M24LC64F_WriteReg(uint16_t RegAddress, uint8_t *Data,uint8_t SendLen);
uint8_t M24LC64F_ReadReg(uint16_t RegAddress,uint8_t *Data,uint8_t ReceiveLen);
uint8_t EEPROM_read(uint8_t *Version,uint8_t *MN);


#endif
