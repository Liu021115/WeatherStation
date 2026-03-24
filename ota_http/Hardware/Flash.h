#ifndef __FLASH_H
#define __FLASH_H

#include "stm32f10x.h"
#include <stdbool.h>


#define FLASH_BASE_ADDR      0x08007000  // 写入起始地址（避开0x08000000区域）

void Flash_ErasePage(uint32_t address);
void Flash_WriteWord(uint32_t address, uint32_t data);
uint32_t Flash_ReadWord(uint32_t address);
void Flash_WriteBuffer(uint32_t address, uint8_t* data, uint32_t length);
bool Flash_Verify(uint32_t address, uint8_t* data, uint32_t length);
uint32_t Flash_GetPageAddress(uint32_t addr);

#endif
