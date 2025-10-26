/***********************************************************************
 * @
 * @ USBD_UAC_I94100_NAU88L21 Sample Code
 * @
 * @ Copyright (C) 2021 Nuvoton Technology Corporation America 
 * @ All rights reserved
 * @
 * @ Please contact email below for more technical support
 * @ ChipCorder@nuvoton.com
***********************************************************************/
#include <stdio.h>
#include <string.h> 
#include "Platform.h"
#include "usbd_audio.h"
#include "audio_class.h"
#include "SysClk.h"
#include "NAU88L21_LIB.h"
//----------------------------------------------------------------------------
//  MAIN
//----------------------------------------------------------------------------
int main(void)
{	
	// SYSTEM INIT
	SYSCLK_INIT();
  
	// NAU88L21 INIT
	I2C1_INIT(); 
	NAU88L21_INIT();
	I2S_INIT();
	
	// USBD INIT
	USBD_INIT();
	USBD_START();
	
	while(1);
}
/* @ Copyright (C) 2021 Nuvoton Technology Corporation America */
