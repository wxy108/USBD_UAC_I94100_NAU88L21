/***********************************************************************
 * @
 * @ NAU88L21.h
 * @
 * @ Copyright (C) 2021 Nuvoton Technology Corporation America 
 * @ All rights reserved
 * @
 * @ Please contact email below for more technical support
 * @ ChipCorder@nuvoton.com
***********************************************************************/
#ifndef __NAU88L21_library__
#define __NAU88L21_library__

extern volatile uint8_t g_u8CodecInit;
extern volatile uint16_t g_u16CodecTimeOutCnt;
void I2S_INIT(void);
void I2C1_INIT(void);
void NAU88L21_INIT(void);

#endif //__NAU88L21_library__
/* @ Copyright (C) 2021 Nuvoton Technology Corporation America */
