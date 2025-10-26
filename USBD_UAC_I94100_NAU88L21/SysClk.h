#ifndef __CONFIGSYSCLK_H__
#define __CONFIGSYSCLK_H__

#include "Platform.h"

#ifdef __cplusplus
extern "C"
{
#endif
	
/**
  * @brief   Provide user selections of HIRC's frequency.
  * @details 
  * SYSCLK_HIRC_CLK_49152 : HIRC's frequency is 49.152 MHz.
  * SYSCLK_HIRC_CLK_48000 : HIRC's frequency is 48 MHz.
  */
#define SYSCLK_HIRC_CLK_49152    (0)   
#define SYSCLK_HIRC_CLK_48000    (1)   

/**
  * @brief   Provide user selections of HCLK's clock source.
  * @details 
  * SYSCLK_HCLK_CLK_HIRC     : HCLK's clock source is from HIRC.
  * SYSCLK_HCLK_CLK_HXT      : HCLK's clock source is from HXT.
  * SYSCLK_HCLK_CLK_PLL_HIRC : HCLK's clock source is from PLL, and PLL's clock source is from HIRC.
  * SYSCLK_HCLK_CLK_PLL_HXT  : HCLK's clock source is from PLL, and PLL's clock source is from HXT.
  */
#define SYSCLK_HCLK_CLK_HIRC     (0)    
#define SYSCLK_HCLK_CLK_HXT      (1)    
#define SYSCLK_HCLK_CLK_PLL_HIRC (2)     
#define SYSCLK_HCLK_CLK_PLL_HXT  (3)    
	
/**
  * @brief  This function generates desired system clock via the configuration defined in ConfigSysClk.h.
  * @return The actually system clock
  */
UINT32 SysClk_Initiate(void);
	
/**
  * @brief   Provide user selections of HIRC's frequency.
  * @details 
  * SYSCLK_HIRC_CLK_49152 : HIRC's frequency is 49.152 MHz.
  * SYSCLK_HIRC_CLK_48000 : HIRC's frequency is 48 MHz.
  */
#define SYSCLK_HIRC_CLK   (SYSCLK_HIRC_CLK_48000) 

/**
  * @brief   Provide user selections of HCLK's clock source.
  * @details 
  * SYSCLK_HCLK_CLK_HIRC     : HCLK's clock source is from HIRC.
  * SYSCLK_HCLK_CLK_HXT      : HCLK's clock source is from HXT.
  * SYSCLK_HCLK_CLK_PLL_HIRC : HCLK's clock source is from PLL, and PLL's clock source is from HIRC.
  * SYSCLK_HCLK_CLK_PLL_HXT  : HCLK's clock source is from PLL, and PLL's clock source is from HXT.
  */	
#define SYSCLK_HCLK_CLK   (SYSCLK_HCLK_CLK_PLL_HXT)

/**
  * @brief   Provide user config the PLL frequency if HCLK's clock source is from PLL.
  * @details 
  * (1) SYSCLK_PLL_CLK must be in the range of 50000~200000 (KHz).
  * (2) SYSCLK_PLL_CLK's unit is KHz.
  * (3) When the PLL's source clock is from 12.288MHz(HXT) or 49.152MHz(HIRC), 
  *     SYSCLK_PLL_CLK must be multiple of '1024'.
  * (4) When the PLL's source clock is from 12MHz(HXT) or 48MHz(HIRC), 
  *     SYSCLK_PLL_CLK must be multiple of '1000'.
  */	
#define SYSCLK_PLL_CLK    (98304UL)	

/**
  * @brief   Provide user call function and this function's clock configuration depending on upper definition.
  */	
#define SYSCLK_INIT()  SysClk_Initiate()


// Compile check.
#if ((SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_HXT)||(SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HXT))
	#if (SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HXT)
		#if (((SYSCLK_PLL_CLK%1000)!=0)&&(((__HXT/1000)%1024)!=0))
			#error "Please check 'SYSCLK_PLL_CLK' must be multiple of '1000' depending on current HXT frequency."
		#elif (((SYSCLK_PLL_CLK%1024)!=0)&&(((__HXT/1000)%1024)==0))
			#error "Please check 'SYSCLK_PLL_CLK' must be multiple of '1024' depending on current HXT frequency."
		#endif
	#endif
#elif (SYSCLK_HCLK_CLK==SYSCLK_HCLK_CLK_PLL_HIRC)
	#if ((SYSCLK_HIRC_CLK==SYSCLK_HIRC_CLK_49152)&&((SYSCLK_PLL_CLK%1024)!=0))
		#error "Please check 'SYSCLK_PLL_CLK' must be multiple of '1024' because current HIRC clock is 49.152MHz."
	#elif ((SYSCLK_HIRC_CLK==SYSCLK_HIRC_CLK_48000)&&((SYSCLK_PLL_CLK%1000)!=0))
		#error "Please check 'SYSCLK_PLL_CLK' must be multiple of '1000' because current HIRC clock is 48MHz."
	#endif
#endif

void Systick_Init(void);
void HIRC_AutoTrim_Init(void);
void DELAYMS(int MS);

#ifdef __cplusplus
}
#endif

#endif
