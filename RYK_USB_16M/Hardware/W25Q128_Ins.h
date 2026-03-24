#ifndef __W25Q128_INS_H
#define __W25Q128_INS_H

#define W25Q128_WRITE_ENABLE												0x06
#define W25Q128_WRITE_DISABLE											0x04
#define W25Q128_READ_STATUS_REGISTER_1							0x05
#define W25Q128_READ_STATUS_REGISTER_2							0x35
#define W25Q128_WRITE_STATUS_REGISTER							0x01
#define W25Q128_PAGE_PROGRAM												0x02
#define W25Q128_QUAD_PAGE_PROGRAM									0x32//四路输入寻呼程序
#define W25Q128_BLOCK_ERASE_64KB										0xD8//块擦除 （64KB）
#define W25Q128_BLOCK_ERASE_32KB										0x52//块擦除 （32KB）
#define W25Q128_SECTOR_ERASE_4KB										0x20//扇区擦除
#define W25Q128_CHIP_ERASE													0xC7   //芯片擦除
#define W25Q128_ERASE_SUSPEND											0x75//擦除/程序暂停
#define W25Q128_ERASE_RESUME												0x7A//擦除/程序恢复
#define W25Q128_POWER_DOWN													0xB9//掉电
//#define W25Q128_HIGH_PERFORMANCE_MODE						0xA3
#define W25Q128_CONTINUOUS_READ_MODE_RESET					0xFF
#define W25Q128_RELEASE_POWER_DOWN_HPM_DEVICE_ID		0xAB//释放 Power-down / ID
#define W25Q128_MANUFACTURER_DEVICE_ID							0x90//制造商/设备 ID
#define W25Q128_READ_UNIQUE_ID											0x4B
#define W25Q128_JEDEC_ID														0x9F
#define W25Q128_READ_DATA													0x03
#define W25Q128_FAST_READ													0x0B
#define W25Q128_FAST_READ_DUAL_OUTPUT							0x3B//快速读取双输出
#define W25Q128_FAST_READ_DUAL_IO									0xBB//快速读取双 I/O
#define W25Q128_FAST_READ_QUAD_OUTPUT							0x6B//快速读取四路输出
#define W25Q128_FAST_READ_QUAD_IO									0xEB//快速读取四通道 I/O
#define W25Q128_OCTAL_WORD_READ_QUAD_IO						0xE3
		
#define W25Q128_DUMMY_BYTE													0xFF



#endif
