/******************************************************************************
 * @file     MassStorage.c
 * @brief    USBD driver Sample file
 *
 * @note
 * Copyright (C) 2016 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

/*!<Includes */
#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "usbd_audio.h"
#include "audio_class.h"
#include "hid_trans.h"

#if 0
	#define DBG_PRINTF      printf
#else
	#define DBG_PRINTF(...)
#endif


/*--------------------------------------------------------------------------*/
// Extern Functions
void PDMA_I2S0_TX_Init(void);
void I2S_TX_Start(void);
void I2S_TX_Stop(void);
void PDMA_I2S0_RX_Init(void);
void I2S_RX_Start(void);
void I2S_RX_Stop(void);

//----------------------------------------------------------------------------
//  Global variables for Control Pipe
//---------------------------------------------------------------------------- 
volatile uint8_t g_u8AudioSpeakerState = 0;
volatile uint8_t g_u8AudioMicState = 0;

// USB flow control variables
uint32_t g_u32BulkBuf0, g_u32BulkBuf1;
static uint32_t g_u32SpeakerByteCount, g_u32SpeakerByteCountRec;
static uint8_t g_u8AudioSpeakerDebounce, g_u8AudioSpeakerPause;

// PlayBack Buffer Control Variable
volatile uint32_t g_au32USB_PlaybackBuff[MAX_USB_BUFFER_LEN];
volatile uint32_t g_au32I2S0TxBuff[2][I2S_BUFF_LEN];
volatile uint8_t g_u8I2S0TxBuffIndex = 0;
volatile uint16_t g_u16PlayBackReadPtr = 0;
volatile uint16_t g_u16PlayBackWritePtr = 0;
volatile uint16_t g_u16PlayBack_Ptrs_Distance = 0;
volatile uint16_t g_u16PayLoadLen;

// Record Buffer Control Variable
volatile uint32_t g_au32USB_RecordBuff[MAX_USB_BUFFER_LEN];
volatile uint32_t g_au32I2S0RxBuff[2][I2S_BUFF_LEN];
volatile uint8_t g_u8I2S0RxBuffIndex = 0;
volatile uint16_t g_u16RecordReadPtr = 0;
volatile uint16_t g_u16RecordWritePtr = 0;
volatile uint16_t g_u16Record_Ptrs_Distance = 0;

// Audio Parameter
volatile uint32_t g_usbd_PlaySampleRate = PLAY_RATE_48K;
volatile uint8_t g_usbd_PlayMute      = 0x00;     /* Play MUTE control. 0 = normal. 1 = MUTE */
volatile int16_t g_usbd_PlayVolumeL   = 0x0000; 
volatile int16_t g_usbd_PlayVolumeR   = 0x0000;
volatile int16_t g_usbd_CurPlayVolumeL   = 0x0000; 
volatile int16_t g_usbd_CurPlayVolumeR   = 0x0000;
volatile int16_t g_usbd_PlayMaxVolume = 0x0000;
volatile int16_t g_usbd_PlayMinVolume = 0xBA00;  // -70dB
volatile int16_t g_usbd_PlayResVolume = 0x400;

volatile uint8_t g_usbd_RecMute       = 0x00;     /* Record MUTE control. 0 = normal. 1 = MUTE */
volatile int16_t g_usbd_RecVolumeL    = 0x0000;
volatile int16_t g_usbd_RecVolumeR    = 0x0000;
volatile int16_t g_usbd_CurRecVolumeL    = 0x0000;
volatile int16_t g_usbd_CurRecVolumeR    = 0x0000;
volatile int16_t g_usbd_RecMaxVolume  = 0x0000;
volatile int16_t g_usbd_RecMinVolume  = 0xBA00;  // -70dB 
volatile int16_t g_usbd_RecResVolume  = 0x400;


//----------------------------------------------------------------------------
//  UART0 Initial function
//----------------------------------------------------------------------------
void USBD_INIT(void)
{
	// MFP select 
	SYS->GPB_MFPH &= ~( SYS_GPB_MFPH_PB13MFP_Msk | SYS_GPB_MFPH_PB14MFP_Msk | SYS_GPB_MFPH_PB15MFP_Msk );
	SYS->GPB_MFPH |= SYS_GPB_MFPH_PB13MFP_USBD_DN | SYS_GPB_MFPH_PB14MFP_USBD_DP | SYS_GPB_MFPH_PB15MFP_USBD_VBUS;
	
	// Select IP clock source
	CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL4_USBSEL_HIRC, CLK_CLKDIV0_USBD(1));
	CLK_EnableModuleClock(USBD_MODULE);
	
	// USBD initial.
	USBD_Open(&gsInfo, AUDIO_ClassRequest, (SET_INTERFACE_REQ)AUDIO_SetInterface);
	
	// Endpoint configuration.
	AUDIO_Init();
	NVIC_EnableIRQ(USBD_IRQn);
}
/*--------------------------------------------------------------------------*/
void USBD_IRQHandler(void)
{
	uint32_t u32IntSts = USBD_GET_INT_FLAG();
	uint32_t u32State = USBD_GET_BUS_STATE();

	if(u32IntSts & USBD_INTSTS_FLDET) 
	{
		// Floating detect
		USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

		if(USBD_IS_ATTACHED()) 
		{
			// Enable GPB15(VBUS) pull down state to solute suspend event issue.
			GPIO_EnablePullState(PB,BIT15,GPIO_PUSEL_PULL_DOWN); 
			/* USB Plug In */
			USBD_ENABLE_USB();
		} 
		else 
		{
			// Disable GPB15 pull down state.
			GPIO_DisablePullState(PB,BIT15); 
			/* USB Un-plug */
			USBD_DISABLE_USB();
		}
	}

		
	if ( u32IntSts & USBD_INTSTS_SOFIF_Msk )
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_SOFIF_Msk);
		if ( g_u32SpeakerByteCountRec != g_u32SpeakerByteCount )
		{
			g_u32SpeakerByteCountRec = g_u32SpeakerByteCount;
		}
		else
		{
			g_u8AudioSpeakerDebounce++;
			if (g_u8AudioSpeakerDebounce >= 5)
			{
				g_u8AudioSpeakerDebounce = 0;
				g_u8AudioSpeakerPause = 1;
				g_u32SpeakerByteCount = 0; 
				g_u32SpeakerByteCountRec = 0;
			}				
		}			
	}	
		
	if(u32IntSts & USBD_INTSTS_BUS) 
	{
		/* Clear event flag */
		USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

		if(u32State & USBD_STATE_USBRST) {
			/* Bus reset */
			USBD_ENABLE_USB();
			USBD_SwReset();
			DBG_PRINTF("Bus reset\n");
		}
		if(u32State & USBD_STATE_SUSPEND) {
			/* Enable USB but disable PHY */
			USBD_DISABLE_PHY();
			DBG_PRINTF("Suspend\n");
		}
		if(u32State & USBD_STATE_RESUME) {
			/* Enable USB and enable PHY */
			USBD_ENABLE_USB();
			DBG_PRINTF("Resume\n");
		}
	}


	if(u32IntSts & USBD_INTSTS_USB) {
		// EP events
		if(u32IntSts & USBD_INTSTS_EP0) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
			// control IN
			USBD_CtrlIn();
		}

		if(u32IntSts & USBD_INTSTS_EP1) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);
			// control OUT
			USBD_CtrlOut();
		}

		if(u32IntSts & USBD_INTSTS_EP2) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
			EP2_Handler();
		}

		if(u32IntSts & USBD_INTSTS_EP3) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
			EP3_Handler();
		}

		if(u32IntSts & USBD_INTSTS_EP4) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
			EP4_Handler();
		}

		if(u32IntSts & USBD_INTSTS_EP5) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
		}

		if(u32IntSts & USBD_INTSTS_EP6) {
			/* Clear event flag */
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
			// Interrupt IN
			EP6_Handler();
		}

		if(u32IntSts & USBD_INTSTS_EP7) {
				/* Clear event flag */
				USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
				// Interrupt OUT
				EP7_Handler();
		}
		
		// USB event
		if(u32IntSts & USBD_INTSTS_SETUP) {
				// Setup packet
				/* Clear event flag */
				USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

				/* Clear the data IN/OUT ready flag of control end-points */
				USBD_STOP_TRANSACTION(EP0);
				USBD_STOP_TRANSACTION(EP1);

				USBD_ProcessSetupPacket();
		}
	}
}

void EP2_Handler(void)
{	
	int i;
	uint16_t u16distance;
	uint8_t *pu8buf;
	uint8_t u8EP2_Byte_Count;
    
	if (g_u8AudioMicState == UAC_BUSY_AUDIO_RECORD)
	{
		// This section calculate the data number in UAC_RingBuffer.
		// And set the next transfer size depends on buffer threshold.
		if ( g_u16RecordWritePtr >= g_u16RecordReadPtr ) 
		{
			u16distance = g_u16RecordWritePtr - g_u16RecordReadPtr;	
		}
		else 
		{   
			u16distance = (MAX_USB_BUFFER_LEN - g_u16RecordReadPtr) + g_u16RecordWritePtr;	
		}
		
		if ( u16distance >= USB_BUFF_UPPER_THRE ) 
		{
			u8EP2_Byte_Count = USB_RECORD_BASE_BYTE + USB_RECORD_ADD;		
		}
		else if ( u16distance < USB_BUFF_LOWER_THRE ) 
		{
			u8EP2_Byte_Count = USB_RECORD_BASE_BYTE - USB_RECORD_SUB;		
		}
		else 
		{
			u8EP2_Byte_Count = USB_RECORD_BASE_BYTE;
		}
              
		pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2));

		for ( i= 0 ; i < (u8EP2_Byte_Count>>2) ; i++)
		{           
			pu8buf[i*4+0] = (g_au32USB_RecordBuff[g_u16RecordReadPtr] >>  0) & 0xff;
			pu8buf[i*4+1] = (g_au32USB_RecordBuff[g_u16RecordReadPtr] >>  8) & 0xff;
			pu8buf[i*4+2] = (g_au32USB_RecordBuff[g_u16RecordReadPtr] >> 16) & 0xff;
			pu8buf[i*4+3] = (g_au32USB_RecordBuff[g_u16RecordReadPtr] >> 24) & 0xff;
			g_u16RecordReadPtr++;
			
			if (g_u16RecordReadPtr >= MAX_USB_BUFFER_LEN)       
			{
				g_u16RecordReadPtr = 0;
			}
		}
		
		USBD_SET_PAYLOAD_LEN(EP2, u8EP2_Byte_Count);
	}
	else
	{	
		USBD_SET_PAYLOAD_LEN(EP2, 0);
	}
   
	return;
}

void EP3_Handler(void)
{
	uint32_t i;
	uint16_t u16Outlen;
	uint32_t *pu32Buf;
	uint8_t *pu8buf;	

	// Get payload of EP3.
	g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3);
	g_u32SpeakerByteCount += g_u16PayLoadLen;
		
	if (g_u8AudioSpeakerState == UAC_BUSY_AUDIO_SPEAK)
	{
		// Set data pu32Tempbuf point to End-Point buffer address.
		pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
		u16Outlen = USBD_GET_PAYLOAD_LEN(EP3);
		u16Outlen >>= 2;
	 
		for ( i = 0 ; i < u16Outlen ; i++)
		{
			// Ring buffer write pointer turn around.
			if( ++g_u16PlayBackWritePtr == MAX_USB_BUFFER_LEN )
				g_u16PlayBackWritePtr = 0;
			
			// Get data from End-Point buffer.
			g_au32USB_PlaybackBuff[g_u16PlayBackWritePtr] = pu32Buf[i];
		}
		
		// Prepare for nex OUT packet.
		USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
		return;
	}
	// Audio start. To setup Buffer Control.
	else if (g_u8AudioSpeakerState == UAC_START_AUDIO_SPEAK)
	{
		// Set Payload to start EP4
		pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
		pu8buf[0] = 0x00;
		pu8buf[1] = 0x00;
		pu8buf[2] = 0x0C;
		USBD_SET_PAYLOAD_LEN(EP4, 3);
	
		g_u8AudioSpeakerState = UAC_PROCESS1_AUDIO_SPEAK;
		g_u16PlayBackWritePtr = 0;
		g_u16PlayBackReadPtr = 0;
	
		pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
		g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3); 	
		
		for ( i = 0 ; i < (g_u16PayLoadLen/4) ; i++)
		{	
			g_au32USB_PlaybackBuff[g_u16PlayBackWritePtr++] = pu32Buf[i];
		}
	}
	// First Audio transfer.
	else if (g_u8AudioSpeakerState == UAC_PROCESS1_AUDIO_SPEAK)
	{
		g_u8AudioSpeakerState = UAC_PROCESS2_AUDIO_SPEAK;
	
		pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
		g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3); 

		for ( i = 0 ; i < (g_u16PayLoadLen/4) ; i++)
		{		
				g_au32USB_PlaybackBuff[g_u16PlayBackWritePtr++] = pu32Buf[i];
		}
	}
	// Second Audio transfer.
	else if (g_u8AudioSpeakerState == UAC_PROCESS2_AUDIO_SPEAK)
	{
		g_u8AudioSpeakerState = UAC_BUSY_AUDIO_SPEAK;
	
		pu32Buf = (uint32_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));
		g_u16PayLoadLen = USBD_GET_PAYLOAD_LEN(EP3); 

		for ( i = 0 ; i < (g_u16PayLoadLen/4) ; i++)
		{		
			g_au32USB_PlaybackBuff[g_u16PlayBackWritePtr++] = pu32Buf[i];
		}		
		g_u8I2S0TxBuffIndex = 0;
		I2S_TX_Start();
	}
	
	/* Prepare for nex OUT packet */
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE );
}

void EP4_Handler(void)
{
	uint8_t *pu8buf;
	uint32_t u32Temp;
	
	u32Temp = g_u32SpeakerByteCount;
	g_u32SpeakerByteCount = 0;
	
	if (g_u8AudioSpeakerPause)
	{
		g_u8AudioSpeakerPause = 0;
		pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
		pu8buf[0] = 0x00; 
		pu8buf[1] = 0x00; 
		pu8buf[2] = 0x0C;
		USBD_SET_PAYLOAD_LEN(EP4, 3);		
		return;
	}
	// Calculate how many data in ring buffer.
	if ( g_u16PlayBackWritePtr >= g_u16PlayBackReadPtr )
	{
		g_u16PlayBack_Ptrs_Distance = g_u16PlayBackWritePtr - g_u16PlayBackReadPtr;
	}
	else
	{
		g_u16PlayBack_Ptrs_Distance = (MAX_USB_BUFFER_LEN - g_u16PlayBackReadPtr) + g_u16PlayBackWritePtr;	
	}
	// Adjust the amount of data that EP3 transfer from host.
	// Ring buffer is close to overflow.
	if ( g_u16PlayBack_Ptrs_Distance > USB_BUFF_UPPER_THRE ) 
	{
		u32Temp = USB_BUFFER_EP4_BASE - USB_BUFFER_EP4_SUB;		
	}
	// Ring buffer is close to underflow.
	else if ( g_u16PlayBack_Ptrs_Distance < USB_BUFF_LOWER_THRE ) 
	{
		u32Temp = USB_BUFFER_EP4_BASE + USB_BUFFER_EP4_ADD;
	}			
	else
	{
		u32Temp = USB_BUFFER_EP4_BASE;
	}
	
	pu8buf = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP4));
	pu8buf[0] = (u32Temp>> 0) & 0xff;
	pu8buf[1] = (u32Temp>> 8) & 0xff;
	pu8buf[2] = (u32Temp>>16) & 0xff;
	USBD_SET_PAYLOAD_LEN(EP4, 3);
}

void EP5_Handler(void)
{
	
}

// Interrupt IN handler 
void EP6_Handler(void)
{
    
}

// Interrupt OUT handler
void EP7_Handler(void)
{
    
}

////////////////////////////////////////////////////////////////////
void AUDIO_Init(void)
{
    int32_t i;
    uint8_t *pu8;
    uint8_t *pSerial = __TIME__;

    /* Init setup packet buffer */
    /* Buffer range for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

    /*****************************************************/
    /* EP2 ==> Iso IN endpoint, address 2 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | USBD_CFG_TYPE_ISO | EP2);
    /* Buffer range for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

    /* EP3 ==> Iso Out endpoint, address 3 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | USBD_CFG_TYPE_ISO | EP3);
    /* Buffer range for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
    /* trigger to receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
		
		
	/*****************************************************/
    /* EP4 ==> Interrupt IN endpoint number 4 */
//		USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_IN | USBD_CFG_TYPE_ISO | EP4);	
    /* Buffer range for EP4 */
//    USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);
		
//		/* EP5 ==> Interrupt IN endpoint number 5 */
//    USBD_CONFIG_EP(EP5, USBD_CFG_EPMODE_IN | EP5);
//    /* Buffer range for EP5 */
//    USBD_SET_EP_BUF_ADDR(EP5, EP5_BUF_BASE);

//    /*****************************************************/
//    /* EP6 ==> Interrupt IN endpoint, address 6 */
//    USBD_CONFIG_EP(EP6, USBD_CFG_EPMODE_IN | EP6);
//    /* Buffer range for EP6 */
//    USBD_SET_EP_BUF_ADDR(EP6, EP6_BUF_BASE);

//    /* EP7 ==> Interrupt Out endpoint, address 7 */
//    USBD_CONFIG_EP(EP7, USBD_CFG_EPMODE_OUT | EP7);
//    /* Buffer range for EP7 */
//    USBD_SET_EP_BUF_ADDR(EP7, EP7_BUF_BASE);
//    /* trigger to receive OUT data */
//    USBD_SET_PAYLOAD_LEN(EP7, EP7_MAX_PKT_SIZE);
    
    
    /*****************************************************/
//    g_u32BulkBuf0 = EP3_BUF_BASE;
//    g_u32BulkBuf1 = EP2_BUF_BASE;

//    g_sCSW.dCSWSignature = CSW_SIGNATURE;

    /*
       Generate Mass-Storage Device serial number
       To compliant USB-IF MSC test, we must enable serial string descriptor.
       However, window may fail to recognize the devices if PID/VID and serial number are all the same
       when plug them to Windows at the sample time.
       Therefore, we must generate different serial number for each device to avoid conflict
       when plug more then 2 MassStorage devices to Windows at the same time.

       NOTE: We use compiler predefine macro "__TIME__" to generate different number for serial
       at each build but each device here for a demo.
       User must change it to make sure all serial number is different between each device.
     */
    pu8 = (uint8_t *)gsInfo.gu8StringDesc[3];
		
    for(i = 0; i < 8; i++)
        pu8[pu8[0] - 16 + i * 2] = pSerial[i];

}

void AUDIO_ClassRequest(void)
{
    uint8_t buf[8];
    uint32_t u32Temp;

    USBD_GetSetupPacket(buf);
    
    if((buf[0] & 0x7f) == 0x22) 	//bmRequestType= class, endpoint
		{ 
					if(buf[0] & 0x80)		// Device to host
					{
							switch(buf[1])
							{
                case UAC_GET_CUR:
                {
                    if(  buf[3]==SAMPLING_FREQ_CONTROL && (buf[4]==EP3))
										{
                      //CX  g_u8RecEn = 0;
                        u32Temp = g_usbd_PlaySampleRate;                                                          
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 0) = (uint8_t)u32Temp;
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = (uint8_t)(u32Temp >> 8);
                        M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 2) = (uint8_t)(u32Temp >> 16);
                        
                        /* Data stage */
                        USBD_SET_DATA1(EP0);
                        USBD_SET_PAYLOAD_LEN(EP0, 3);
                        printf("Get_CUR: %08X\n",g_usbd_PlaySampleRate); 



                    }
										else
										{
                        {
                            uint8_t i;
                            puts("error 1: ");
                            for(i=0;i<8;i++)
                                printf(" %2x", buf[i]);
                            
                            printf("\n");
                        }
                         /* error endpoint or un identified Control*/
                         /* Setup error, stall the device */
                         USBD_SetStall(0);
												 printf("Get_CUR USBD_SetStall(0): %08X\n",g_usbd_PlaySampleRate); 
                    }
                   
									// Trigger next Control Out DATA1 Transaction.
									/* Status stage */
									USBD_PrepareCtrlOut(0, 0);
								break;
                }
								 
                default:
                {
                    /* Setup error, stall the device */
                    /*unidentify CONTROL*/
                    USBD_SetStall(0);
                }
                               
            }
            
            
        }
				else		// Host to device
				{
            switch(buf[1])
						{
                case UAC_SET_CUR:
                {   

                    if(buf[3]==SAMPLING_FREQ_CONTROL && (buf[4]==EP3))
										{    
                        USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlaySampleRate, buf[6]);
												                            
                        /* Status stage */
                        USBD_SET_DATA1(EP0);
                        USBD_SET_PAYLOAD_LEN(EP0, 0);
										 
                           
                    }
										else
										{
                        {
                            uint8_t i;
                            puts("error 2: ");
                            for(i=0;i<8;i++)
                                printf(" %2x", buf[i]);
                            
                            printf("\n");
                        }
                        /* Setup error, stall the device */
                        /* Unidentify CONTROL*/
                        /* STALL control pipe */
                        USBD_SetStall(0);
                    }

                    break;
                }
                default:
                {
                    /*unimplement CONTROL or wrong endpoint number*/
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                }    
             }
        }
        
    }
		else		/*Feature unit control*/
		{ 
        if (buf[0] & 0x80)    // Device to host
        {
            
            switch(buf[1])
            {
                case UAC_GET_CUR:
                {
                    switch(buf[3])
                    {
                        case MUTE_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_RecMute;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_PlayMute;
                              M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                            }

                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 1);
                            break;
                        }
                        case VOLUME_CONTROL:
                        {
                                                
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                /* Left or right channel */
                                if(buf[2] == 1)
                                {
                                  u32Temp = g_usbd_RecVolumeL;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_RecVolumeL >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                                else
                                {
                                  u32Temp = g_usbd_RecVolumeR;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_RecVolumeR >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }

                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                /* Left or right channel */
                                if(buf[2] == 1)
                                {
                                  u32Temp = g_usbd_PlayVolumeL;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_PlayVolumeL >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                                else
                                {
                                  u32Temp = g_usbd_PlayVolumeR;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                    u32Temp = g_usbd_PlayVolumeR >> 8;
                                    M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                                }
                            }

                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                        {
                            /* Setup error, stall the device */
                            USBD_SetStall(0);
                        }
                    }

                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_MIN:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_RecMinVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecMinVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_PlayMinVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayMinVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }
                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_MAX:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_RecMaxVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecMaxVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_PlayMaxVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayMaxVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }
										
                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }

                case UAC_GET_RES:
                {
                    switch(buf[3])
                    {
                        case VOLUME_CONTROL:
                        {
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_RecResVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_RecResVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) = u32Temp;
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                              u32Temp = g_usbd_PlayResVolume;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)) = u32Temp;
                                u32Temp = g_usbd_PlayResVolume >> 8;
                                M8(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0) + 1) =u32Temp;
                            }
                            /* Data stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 2);
                            break;
                        }
                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                    }
                    // Trigger next Control Out DATA1 Transaction.
                    /* Status stage */
                    USBD_PrepareCtrlOut(0, 0);
                    break;
                }
								

                default:
                {
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                }
            }
        }
        else
        {
            // Host to device
            switch(buf[1])
            {
                case UAC_SET_CUR:
                {
                    switch(buf[3])
                    {
                        case MUTE_CONTROL:
                            if(REC_FEATURE_UNITID == buf[5])
                                USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecMute, buf[6]);
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {
                                USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayMute, buf[6]);
                            }
                            /* Status stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 0);
                            break;

                        case VOLUME_CONTROL:
                            if(REC_FEATURE_UNITID == buf[5])
                            {
                                if(buf[2] == 1)
                                {
                                    /* Prepare the buffer for new record volume of left channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecVolumeL, buf[6]);
                                }
                                else
                                {
                                    /* Prepare the buffer for new record volume of right channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_RecVolumeR, buf[6]);
                                }
                            }
                            else if(PLAY_FEATURE_UNITID == buf[5])
                            {

                                if(buf[2] == 1)
                                {
                                    /* Prepare the buffer for new play volume of left channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayVolumeL, buf[6]);
                                }
                                else
                                {
                                    /* Prepare the buffer for new play volume of right channel */
                                    USBD_PrepareCtrlOut((uint8_t *)&g_usbd_PlayVolumeR, buf[6]);
                                }
                            }

                            /* Status stage */
                            USBD_SET_DATA1(EP0);
                            USBD_SET_PAYLOAD_LEN(EP0, 0);
                            break;

                        default:
                            /* STALL control pipe */
                            USBD_SetStall(0);
                            break;
                    }
                    break;
                }
								// HID 
                case HID_SET_REPORT:
                {
                    if(buf[3] == 2)
                    {
                        /* Request Type = Output */
                        USBD_SET_DATA1(EP1);
                        USBD_SET_PAYLOAD_LEN(EP1, buf[6]);

                        /* Status stage */
                        USBD_PrepareCtrlIn(0, 0);
                    }
                    break;
                }
                case HID_SET_IDLE:
                {
                    /* Status stage */
                    USBD_SET_DATA1(EP0);
                    USBD_SET_PAYLOAD_LEN(EP0, 0);
                    break;
                }
                case HID_SET_PROTOCOL:

                default:
                {
                    /* Setup error, stall the device */
                    USBD_SetStall(0);
                    break;
                }
            }
        }
    }
}

/**
 * @brief       Set Interface standard request
 *
 * @param[in]   u32AltInterface Interface
 *
 * @return      None
 *
 * @details     This function is used to set UAC Class relative setting
 */
void AUDIO_SetInterface(void)			
{
	uint8_t buf[8];
	uint8_t u32AltInterface;
	uint8_t g_usbd_UsbInterface;
	

	USBD_GetSetupPacket(buf);

	u32AltInterface = buf[2];
	g_usbd_UsbInterface = buf[4];
	
    if ( g_usbd_UsbInterface == 1 )		
	{
		/* Audio Iso IN interface */	
		if (u32AltInterface == 1)
		{
			g_u8AudioMicState = UAC_START_AUDIO_RECORD;
				
			USBD_SET_DATA1(EP2);
			USBD_SET_PAYLOAD_LEN(EP2, 0);
			PDMA_I2S0_RX_Init();
      I2S_RX_Start();
		}
		else
		{
			g_u8AudioMicState = UAC_STOP_AUDIO_RECORD;

			USBD_SET_DATA1(EP2);
			USBD_SET_PAYLOAD_LEN(EP2, 0);
      I2S_RX_Stop();
		}
	}
    
    
    if ( g_usbd_UsbInterface == 2 )		
	{
		/* Audio Iso OUT interface */	
		if (u32AltInterface == 1)
		{
			g_u8AudioSpeakerState = UAC_START_AUDIO_SPEAK;
			g_u32SpeakerByteCount = 0;	
			g_u32SpeakerByteCountRec = 0;
			PDMA_I2S0_TX_Init();
			//I2S_TX_Start();
		}
		else
		{
			g_u8AudioSpeakerState = UAC_STOP_AUDIO_SPEAK;
			g_u32SpeakerByteCount = 0;	
			g_u32SpeakerByteCountRec = 0;
			I2S_TX_Stop();
		}
	}
}
