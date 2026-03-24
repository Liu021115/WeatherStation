/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
//void HardFault_Handler(void)
//{
//	__disable_irq();
//    uint32_t *sp = (uint32_t *)__get_MSP();   // ???
//    /* ?? 8 ???,???? PC ? */
//    for(int i=0;i<8;i++) printf("SP[%d]=%08X\r\n", i, sp[i]);
//  /* Go to infinite loop when Hard Fault exception occurs */
//  while (1)
//  {
//  }
//}

///**
//  * @brief  This function handles Memory Manage exception.
//  * @param  None
//  * @retval None
//  */
//void MemManage_Handler(void)
//{
//	    uint32_t *sp = (uint32_t *)__get_MSP();
//    printf("\nMemManage @ PC=%08X\n", sp[6]); 
//  /* Go to infinite loop when Memory Manage exception occurs */
//  while (1)
//  {
//  }
//}

///**
//  * @brief  This function handles Bus Fault exception.
//  * @param  None
//  * @retval None
//  */
//void BusFault_Handler(void)
//{
//  /* Go to infinite loop when Bus Fault exception occurs */
//  while (1)
//  {
//  }
//}
static void fault_dump(const char *name)
{
   uint32_t *sp = (uint32_t *)__get_MSP();
    uint32_t pc = sp[6];
    uint32_t lr = sp[5];
    uint32_t r4_pop = sp[4];   /* POP ?? r4 ? */
    printf("\n%sFault @ PC=%08X LR=%08X  r4_pop=%08X\n", name, pc, lr, r4_pop);
//	 printf("MSP dump:\n");
//    for (int i = 0; i < 16; i += 4) {
//        printf("%08lX: %08lX %08lX %08lX %08lX\n",
//               (uint32_t)(sp + i), sp[i], sp[i + 1], sp[i + 2], sp[i + 3]);
//    }
    while(1);
}
void HardFault_Handler(void)   { fault_dump("Hard"); }
void MemManage_Handler(void)   { fault_dump("Mem");  }
void BusFault_Handler(void)    { fault_dump("Bus");  }



/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
