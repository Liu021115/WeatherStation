#include "stm32f10x.h"
#include <string.h>
#include "Flash.h"

#define FLASH_PAGE_SIZE      2048        // 根据芯片型号调整（F103C8T6是1K或2K）


__attribute__((aligned(4))) static uint8_t page_buffer[FLASH_PAGE_SIZE];

/*****************************************************
擦除Flash扇区（STM32F103示例）
*****************************************************/
void Flash_ErasePage(uint32_t address) {
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    
    // 计算页地址（对齐到页边界）
    address = address - (address % FLASH_PAGE_SIZE);
    FLASH_ErasePage(address);
    FLASH_Lock();
}

/*****************************************************
写入Flash（按字写入，必须解锁）
*****************************************************/
void Flash_WriteWord(uint32_t address, uint32_t data) {
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    FLASH_ProgramWord(address, data);
    FLASH_Lock();
}

/*****************************************************
读取Flash（用于校验）
*****************************************************/
uint32_t Flash_ReadWord(uint32_t address) {
    return *(uint32_t*)address;
}

/*****************************************************
写入缓冲区数据到Flash（支持任意长度）
*****************************************************/
void Flash_WriteBuffer(uint32_t address, uint8_t* data, uint32_t length) {
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);
    
    // 按字写入（4字节对齐）
    uint32_t i = 0;
    for(i = 0; i < (length & ~0x03); i += 4) {
        uint32_t word = *(uint32_t*)(data + i);
        FLASH_ProgramWord(address + i, word);
    }
    
    // 处理剩余字节（不足4字节）
    if(length & 0x03) {
        uint32_t last_word = 0xFFFFFFFF;
        memcpy(&last_word, data + i, length & 0x03);
        FLASH_ProgramWord(address + i, last_word);
    }
    
    FLASH_Lock();
}

/*****************************************************
校验Flash数据
*****************************************************/
bool Flash_Verify(uint32_t address, uint8_t* data, uint32_t length) {
    for(uint32_t i = 0; i < length; i++) {
        if(*(uint8_t*)(address + i) != data[i]) {
            return false;
        }
    }
    return true;
}

/*****************************************************
获取Flash页地址
*****************************************************/
uint32_t Flash_GetPageAddress(uint32_t addr) {
    return addr - (addr % FLASH_PAGE_SIZE);
}

