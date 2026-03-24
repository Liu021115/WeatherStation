/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "W25Q128.h"
#include "rtc.h"
#include "protocol_def.h"

/* Example: Mapping of physical drive number for each drive */
#define SPI_FLASH	0	/* Map FTL to physical drive 0 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
static	uint8_t MID = 0;  
static	uint16_t DID = 0;

static uint16_t call_cnt = 0;
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
    uint8_t  *pmid = &MID;
    uint16_t *pdid = &DID;
    if (!pmid || !pdid) {
        printf("ID ptr null %p %p\n", pmid, pdid);
        while(1);
    }

	switch (pdrv) {
	case SPI_FLASH :
	 __disable_irq();	
		DID = W25Q128_ReadID(&MID);		//读取flashID
	 __enable_irq();
	

		if(MID == 0xEF && DID == 0x4018)							  //读取成功状态正常	
			return 0;
		else													//读取失败状态不正常
			return STA_NODISK;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {
	case SPI_FLASH :
	__disable_irq();	
		W25Q128_Init();	//flash初始化
__enable_irq();

		return	disk_status(SPI_FLASH);		//调用函数检查状态
	
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;

	switch (pdrv) {
	case SPI_FLASH :
		// translate the arguments here
__disable_irq();
		W25Q128_ReadData(sector *4096, buff, count *4096); 
__enable_irq();
		res = RES_OK;

		return res;
	}

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;

	switch (pdrv) {
	case SPI_FLASH :
__disable_irq();		
		//W25Q128_SectorErase(sector*4096);
		W25Q128_Write((uint8_t *)buff, sector*4096,count*4096);
__enable_irq();

		res = RES_OK;
		return res;

	}

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	switch (pdrv) {
	case SPI_FLASH :

		// Process of the command for the RAM drive
	switch (cmd)
	{
		case	GET_SECTOR_COUNT:		
			*(DWORD	*)buff = 4096;
			break;
		case GET_BLOCK_SIZE:			//返回擦除扇区的最小个数(单位为扇区数)
			*(DWORD	*)buff = 1;
			break;
		case GET_SECTOR_SIZE:
			*(DWORD	*)buff = 4096;
			break;
		case CTRL_SYNC:
			res = RES_OK;
			break;
	}
		res	=	RES_OK;
		return res;

	}

	return RES_PARERR;
}


DWORD get_fattime (void){
	DateTime FStime;
	RTC_ReadDate(&FStime);				//获取当前时间	
	DWORD fattime =	(((FStime.year + 20) << 25)	//将时间按系统要求存放到DWORD变量类型
									|(FStime.month << 21)
									|(FStime.day   << 16)
									|(FStime.hour  << 11)
									|(FStime.min   << 5)
									|(FStime.sec   >> 1));
	return fattime; 
}

