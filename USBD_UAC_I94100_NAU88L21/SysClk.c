/**************************************************************************//**
 * @file     SysClk.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 18/04/02 01:10p $
 * @brief    System clock configuration file for framework.
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include "SysClk.h"
#include "NAU88L21_LIB.h"
// Initiate system clock depend on configuration in ConfigSysClk.h
UINT32 SysClk_Initiate(void)
{
	// Unlock protected registers
	uint8_t u8Lock = SYS_Unlock();
	
	// 1. Configuration HIRC ====================================================
#if (SYSCLK_HIRC_CLK==SYSCLK_HIRC_CLK_49152)
	// Select trim HIRC Frequency to 49.152MHz
	CLK_SELECT_TRIM_HIRC(CLK_CLKSEL0_HIRCFSEL_49M);
#else
	// Select trim HIRC Frequency to 48MHz
	CLK_SELECT_TRIM_HIRC(CLK_CLKSEL0_HIRCFSEL_48M);
#endif
	// Enable HIRC 
	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
	// Waiting for HIRC clock ready 
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

	// 2. Configuration HXT(if enable HXT) ======================================
#if ((SYSCLK_HCLK_CLK == SYSCLK_HCLK_CLK_PLL_HXT) || (SYSCLK_HCLK_CLK == SYSCLK_HCLK_CLK_HXT))
    // Enable HXT 
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
    // Waiting for HIRC clock ready 
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
#endif	

	// 3. Configuration HCLK(if source clock is PLL, enable PLL first) ==========
#if ((SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HIRC)||(SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HXT))
	// Enable PLL clock
	CLK_EnablePLL(((SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HIRC)?CLK_PLLCTL_PLLSRC_HIRC:CLK_PLLCTL_PLLSRC_HXT), (SYSCLK_PLL_CLK*1000));
	// Switch HCLK clock source to PLL
	CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(1));
	
	#if (SYSCLK_PLL_CLK > 90000UL)
	CLK_SetPCLKDivider(CLK_PCLKDIV_PCLK0DIV2);
	CLK_SetPCLKDivider(CLK_PCLKDIV_PCLK1DIV2);
	#endif
	
#else
	// Set HCLK clock source to HIRC(SYSCLK_HCLK_CLK_HIRC) or HXT(SYSCLK_HCLK_CLK_HXT)
	CLK_SetHCLK(((SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_HIRC)?CLK_CLKSEL0_HCLKSEL_HIRC:CLK_CLKSEL0_HCLKSEL_HXT), CLK_CLKDIV0_HCLK(1));
#endif

	// Update System Core Clock. 
	// User can use SystemCoreClockUpdate() to calculate PllClock, SystemCoreClock and CyclesPerUs automatically.
	SystemCoreClockUpdate();
	// Lock protected registers. 
	SYS_Lock(u8Lock);	
	HIRC_AutoTrim_Init();
  Systick_Init();
	return CLK_GetHCLKFreq();
}

//----------------------------------------------------------------------------
//  HIRC Trim function
//----------------------------------------------------------------------------
void IRC_IRQHandler(void)
{
	// Get Trim Failure Interrupt
	if(SYS_GET_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG))
	{ 
		// Clear Trim Failure Interrupt
		SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG);
	}
	
	// Get LXT Clock Error Interrupt
	if(SYS_GET_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_CLKERROR_INT_FLAG)) 
	{ 
		// Clear LXT Clock Error Interrupt 
		SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_CLKERROR_INT_FLAG);
	}
}

void TMR0_IRQHandler(void)
{
	static uint8_t u8Count = 0;
	PB2 = PB2^1;
    if(TIMER_GetIntFlag(TIMER0) == 1)
    {
			/* Clear Timer0 time-out interrupt flag */
			TIMER_ClearIntFlag(TIMER0);
		
			if(++u8Count >= 10)
			{
				SYS_EnableTrimHIRC(SYS_IRCTCTL_FREQSEL_48M);
				u8Count = 0;
			}
    }
}

void HIRC_AutoTrim_Init(void)
{
	SYS_SET_TRIMHIRC_LOOPSEL(SYS_IRCTCTL_LOOPSEL_4);
	SYS_SET_TRIMHIRC_RETRYCNT(SYS_IRCTCTL_RETRYCNT_64);
	SYS_ENABLE_TRIMHIRC_CLKERRSTOP();
	SYS_SET_TRIMHIRC_REFCLK(SYS_IRCTCTL_REFCLK_USBSOF);
	
	// Enable clock error / trim fail interrupt 		
	SYS_ENABLE_TRIMHIRC_INT(SYS_IRCTIEN_TRIMFAIL_INT_MASK|SYS_IRCTIEN_CLKERROR_INT_MASK);

	NVIC_EnableIRQ(IRC_IRQn);
	
	// Timer Initiate for periodic HIRC Auto Trim.
	/* Enable peripheral clock */
	CLK_EnableModuleClock(TMR0_MODULE);
	/* Peripheral clock source */
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_PCLK0, 0);
	
	/* Open Timer0 in periodic mode, enable interrupt and 1 interrupt tick per second */
	TIMER_Open(TIMER0, TIMER_PERIODIC_MODE, 1);
	TIMER_EnableInt(TIMER0);
	
	/* Enable Timer0 ~ Timer3 NVIC */
	NVIC_EnableIRQ(TMR0_IRQn);
	
	/* Start Timer0 ~ Timer3 counting */
	TIMER_Start(TIMER0);
}

void HIRC_AutoTrim_RefSof(void)
{																													
	// HIRC auto trim enable/disable 
	SYS_EnableTrimHIRC(SYS_IRCTCTL_FREQSEL_48M);

	while(!SYS_IS_TRIMHIRC_DONE())
	{
	}
	
	SYS_CLEAR_TRIMHIRC_INT_FLAG(SYS_IRCTISTS_TRIMFAIL_INT_FLAG|SYS_IRCTISTS_CLKERROR_INT_FLAG);
}

//----------------------------------------------------------------------
//  SYSTEM Initial function
//----------------------------------------------------------------------

void SysTick_Handler(void)
{
    if (g_u8CodecInit)
    {
        g_u16CodecTimeOutCnt++;
    }
}
void Systick_Init(void)
{
   SystemCoreClockUpdate();
   SysTick_Config(SystemCoreClock/1000);  //1ms
   NVIC_SetPriority(SysTick_IRQn, 1);
}
void DELAYMS(int MS)
{
	int t1,t2;
	for(t1=0;t1<MS;t1++)
	{
		for(t2=0;t2<16384;t2++){}
	}
}
/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
