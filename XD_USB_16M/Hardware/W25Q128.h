#ifndef __W25Q128_H
#define __W25Q128_H

#define W25Q128_PAGE_SIZE        										0x100
#define W25Q128_TOTAL_SIZE     											0x1000000

void W25Q128_Init(void);
uint16_t  W25Q128_ReadID(uint8_t *MID);
void W25Q128_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void W25Q128_SectorErase(uint32_t Address);
void W25Q128_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);
void W25Q128_ChipErase(void); 

void W25Q128_Write(uint8_t* pBuffer,uint32_t WriteAddr,uint16_t NumByteToWrite);   

#endif
