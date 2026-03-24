#include "stm32f10x.h"                  // Device header
#include "MySPI.h"
#include "W25Q128_Ins.h"
#include "W25Q128.h"

/*****************************************************
W25Q128   128Mbit==16MB
分为256个块 		 每个块64KB
每个块16个扇区  每个扇区4KB
*****************************************************/

/**
  * 函    数：W25Q128初始化
  * 参    数：无
  * 返 回 值：无
  */
void W25Q128_Init(void)
{
	MySPI_Init();//先初始化底层的SPI
}

/**
  * 函    数：W25Q128读取ID号
  * 参    数：MID 工厂ID，使用输出参数的形式返回
  * 参    数：DID 设备ID，使用输出参数的形式返回
  * 返 回 值：无
  */
uint16_t  W25Q128_ReadID(uint8_t *MID)
{
//	MySPI_Start();								//SPI起始
//	MySPI_SwapByte(W25Q128_JEDEC_ID);			//交换发送读取ID的指令
//	*MID = MySPI_SwapByte(W25Q128_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
//	*DID = MySPI_SwapByte(W25Q128_DUMMY_BYTE);	//交换接收DID高8位
//	*DID <<= 8;									//高8位移到高位
//	*DID |= MySPI_SwapByte(W25Q128_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
//	MySPI_Stop();								//SPI终止
	
    MySPI_Start();
    MySPI_SwapByte(W25Q128_JEDEC_ID);
    *MID = MySPI_SwapByte(W25Q128_DUMMY_BYTE);
    uint8_t hi = MySPI_SwapByte(W25Q128_DUMMY_BYTE);
    uint8_t lo = MySPI_SwapByte(W25Q128_DUMMY_BYTE);
    MySPI_Stop();
    return (uint16_t)hi << 8 | lo;
	
}

/**
  * 函    数：W25Q128写使能
  * 参    数：无
  * 返 回 值：无
  */
void W25Q128_WriteEnable(void)
{
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q128_WRITE_ENABLE);		//交换发送写使能的指令
	MySPI_Stop();									//SPI终止
}

/**
  * 函    数：W25Q128等待忙
  * 参    数：无
  * 返 回 值：无
  */
void W25Q128_WaitBusy(void)
{
	uint32_t Timeout;
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q128_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
	Timeout = 100000;							//给定超时计数时间
	while ((MySPI_SwapByte(W25Q128_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
	{
		Timeout --;								//等待时，计数值自减
		if (Timeout == 0)						//自减到0后，等待超时
		{
			/*超时的错误处理代码，可以添加到此处*/
			break;								//跳出等待，不等了
		}
	}
	MySPI_Stop();								//SPI终止
}

/**
  * 函    数：W25Q128页编程
  * 参    数：Address 页编程的起始地址，范围：0x000000~0xFFFFFF
  * 参    数：DataArray	用于写入数据的数组
  * 参    数：Count 要写入数据的数量，范围：0~256
  * 返 回 值：无
  * 注意事项：写入的地址范围不能跨页
  */
void W25Q128_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
	uint16_t i;
	
	W25Q128_WriteEnable();						//写使能
	
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q128_PAGE_PROGRAM);		//交换发送页编程的指令
	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	MySPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)				//循环Count次
	{
		MySPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
	}
	MySPI_Stop();								//SPI终止
	
	W25Q128_WaitBusy();							//等待忙
}

/**
  * 函    数：W25Q128扇区擦除（4KB）
  * 参    数：Address 指定扇区的地址，范围：0x000000~0xFFFFFF 
* 每个扇区地址是xxx000_xxxFFF
  * 返 回 值：无
  */
void W25Q128_SectorErase(uint32_t Address)
{
	W25Q128_WriteEnable();						//写使能
	
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q128_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	MySPI_SwapByte(Address);					//交换发送地址7~0位
	MySPI_Stop();								//SPI终止
	
	W25Q128_WaitBusy();							//等待忙
}

/**
  * 函    数：W25Q128芯片擦除
  * 参    数：擦除整个芯片
  * 返 回 值：无
  */
void W25Q128_ChipErase(void)
{
	W25Q128_WriteEnable();							//写使能	
	MySPI_Start();											//SPI起始
	MySPI_SwapByte(W25Q128_CHIP_ERASE);	//交换发送扇区擦除的指令
	MySPI_Stop();												//SPI终止
	W25Q128_WaitBusy();									//等待忙
}

/**
  * 函    数：W25Q128读取数据
  * 参    数：Address 读取数据的起始地址，范围：0x000000~0xFFFFFF
  * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
  * 参    数：Count 要读取数据的数量，范围：0~0x1000000
  * 返 回 值：无
  */
void W25Q128_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
	uint32_t i;
	MySPI_Start();													//SPI起始
	MySPI_SwapByte(W25Q128_READ_DATA);			//交换发送读取数据的指令
	MySPI_SwapByte(Address >> 16);					//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);						//交换发送地址15~8位
	MySPI_SwapByte(Address);								//交换发送地址7~0位
	for (i = 0; i < Count; i ++)						//循环Count次
		DataArray[i] = MySPI_SwapByte(W25Q128_DUMMY_BYTE);	//依次在起始地址后读取数据
	MySPI_Stop();														//SPI终止
}

/**
  * 函    数：W25Q128写数据（无校验）
  * 参    数：WriteAddr:开始写入的地址(24bit)
  * 参    数：pBuffer:数据存储区
  * 参    数：NumByteToWrite:要写入的字节数(最大65535)
  * 返 回 值：无
	*	必须确保所写的地址范围内的数据全部为0XFF,否则在非0XFF处写入的数据将失败!
  */
void W25Q128_Write_NoCheck(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)   
{ 			 		 
	uint16_t pageremain = 256-WriteAddr%256; //单页剩余的字节数	;	  	    
	if(NumByteToWrite<=pageremain)
		pageremain=NumByteToWrite;//不大于256个字节
	while(1)
	{	   
		W25Q128_PageProgram(WriteAddr,pBuffer,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			  //减去已经写入了的字节数
			if(NumByteToWrite>256)
				pageremain=256; //一次可以写入256个字节
			else 
				pageremain=NumByteToWrite; 	  //不够256个字节了
		}
	}	    
} 


/**
  * 函    数：W25Q128写数据(带擦除)
  * 参    数：WriteAddr 读取数据的起始地址，范围：0x000000~0xFFFFFF,24bit
  * 参    数：pBuffer 用于接收读取数据的数组，通过输出参数返回
  * 参    数：NumByteToWrite 要写入的字节数(最大65535) 
  * 返 回 值：无
  */
u8 W25Q128_BUFFER[4096];		 
void W25Q128_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite)   
{ 
	uint32_t secpos;
	uint16_t secoff;
	uint16_t secremain;	   
 	uint16_t i;    
	uint8_t * W25Q128_BUF;	  
   	W25Q128_BUF=W25Q128_BUFFER;	     
 	secpos=WriteAddr/4096;//扇区地址  
	secoff=WriteAddr%4096;//在扇区内的偏移
	secremain=4096-secoff;//扇区剩余空间大小   
 	//printf("ad:%X,nb:%X\r\n",WriteAddr,NumByteToWrite);//测试用
 	if(NumByteToWrite<=secremain)secremain=NumByteToWrite;//不大于4096个字节
	while(1) 
	{	
		W25Q128_ReadData(secpos*4096,W25Q128_BUF,4096);//读出整个扇区的内容
		for(i=0;i<secremain;i++)//校验数据
		{
			if(W25Q128_BUF[secoff+i]!=0XFF)break;//需要擦除  	  
		}
		if(i<secremain)//需要擦除
		{
			W25Q128_SectorErase(secpos *4096);//擦除这个扇区
			for(i=0;i<secremain;i++)	   //复制
			{
				W25Q128_BUF[i+secoff]=pBuffer[i];	  
			}
			W25Q128_Write_NoCheck(W25Q128_BUF,secpos*4096,4096);//写入整个扇区  
		}
		else 
			W25Q128_Write_NoCheck(pBuffer,WriteAddr,secremain);//写已经擦除了的,直接写入扇区剩余区间. 				   
		if(NumByteToWrite==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0 	 

		   	pBuffer+=secremain;  //指针偏移
				WriteAddr+=secremain;//写地址偏移	   
		   	NumByteToWrite-=secremain;				//字节数递减
			if(NumByteToWrite>4096)
				secremain=4096;	//下一个扇区还是写不完
			else 
				secremain=NumByteToWrite;			//下一个扇区可以写完了
		}	 
	}	 
}
