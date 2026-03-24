#include "usbd_msc_mem.h"
#include "usb_conf.h"
#include "ff.h"
#include "diskio.h"
#include "W25Q128.h"



//最大支持的设备数,2个
#define STORAGE_LUN_NBR 	1

////////////////////////////自己定义的一个标记USB状态的寄存器///////////////////
//bit0:表示电脑正在向SD卡写入数据
//bit1:表示电脑正从SD卡读出数据
//bit2:SD卡写数据错误标志位
//bit3:SD卡读数据错误标志位
//bit4:1,表示电脑有轮询操作(表明连接还保持着)
vu8 USB_STATUS_REG=0;
////////////////////////////////////////////////////////////////////////////////


//USB Mass storage 标准查询数据(每个lun占36字节)
const int8_t  STORAGE_Inquirydata[] = { 	
	/* LUN 1 */
	0x00,
	0x80,		
	0x02,		
	0x02,
	(USBD_STD_INQUIRY_LENGTH - 4),
	0x00,
	0x00,	
	0x00,
	/* Vendor Identification */
  'P', 'R', 'E', 'C', 'H', 'I', 'N', ' ', ' ',//9字节
    /* Product Identification */
    'S', 'P', 'I', ' ', 'F', 'l', 'a', 's', 'h',//15字节
    ' ','D', 'i', 's', 'k', ' ',
    /* Product Revision Level */	
    '1', '.', '0', ' ',
}; 
int8_t STORAGE_Init (uint8_t lun);
int8_t STORAGE_GetCapacity (uint8_t lun,uint32_t *block_num,uint32_t *block_size);
int8_t  STORAGE_IsReady (uint8_t lun);
int8_t  STORAGE_IsWriteProtected (uint8_t lun);
int8_t STORAGE_Read (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len);
int8_t STORAGE_Write (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len);
int8_t STORAGE_GetMaxLun (void);

//USB Device 用户回调函数接口
USBD_STORAGE_cb_TypeDef USBD_MICRO_SDIO_fops =
{
	STORAGE_Init,
	STORAGE_GetCapacity,
	STORAGE_IsReady,
	STORAGE_IsWriteProtected,
	STORAGE_Read,
	STORAGE_Write,
	STORAGE_GetMaxLun,
	(int8_t *)STORAGE_Inquirydata,
};
USBD_STORAGE_cb_TypeDef  *USBD_STORAGE_fops=&USBD_MICRO_SDIO_fops;//指向USBD_MICRO_SDIO_fops结构体.

//初始化存储设备
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//返回值:0,成功;
//    其他,错误代码
int8_t STORAGE_Init (uint8_t lun)
{
	if (lun != 0) {
		printf("USB DEVICE INIT ERROR");
		return -1;
	}
		W25Q128_Init();
	return 0; 
} 

//获取存储设备的容量和块大小
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//block_num:块数量(扇区数)
//block_size:块大小(扇区大小)
//返回值:0,成功;
//    其他,错误代码
int8_t STORAGE_GetCapacity (uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{     
    if (lun != 0) return -1;
    DWORD sz = 0;
    if (disk_ioctl(0, GET_SECTOR_COUNT, &sz) != RES_OK) return -1;
    *block_size = 4096;    
    *block_num  = sz;
    return 0; 
} 

//查看存储设备是否就绪
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//返回值:0,就绪;
//    其他,未就绪
int8_t  STORAGE_IsReady (uint8_t lun)
{ 
	 return (lun == 0) ? 0 : -1;
} 

//查看存储设备是否写保护
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//返回值:0,没有写保护;
//    其他,写保护(只读)
int8_t  STORAGE_IsWriteProtected (uint8_t lun)
{
	return  0;
} 

//从存储设备读取数据
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//buf:数据存储区首地址指针
//blk_addr:要读取的地址(扇区地址)
//blk_len:要读取的块数(扇区数) 
//返回值:0,成功;
//    其他,错误代码 
int8_t STORAGE_Read (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len)
{
	 if (lun != 0) return -1;
    return (disk_read(0, buf, blk_addr, blk_len) == RES_OK) ? 0 : -1;
}
//向存储设备写数据
//lun:逻辑单元编号,1,SD卡;0,SPI FLASH
//buf:数据存储区首地址指针
//blk_addr:要写入的地址(扇区地址)
//blk_len:要写入的块数(扇区数) 
//返回值:0,成功;
//    其他,错误代码 
int8_t STORAGE_Write (uint8_t lun,uint8_t *buf,uint32_t blk_addr,uint16_t blk_len) 
{
if (lun != 0) return -1;
    return (disk_write(0, buf, blk_addr, blk_len) == RES_OK) ? 0 : -1;
}
//获取支持的最大逻辑单元个数
//返回值:支持的逻辑单元个数-1
int8_t STORAGE_GetMaxLun (void)
{
	return (STORAGE_LUN_NBR - 1);
}
























