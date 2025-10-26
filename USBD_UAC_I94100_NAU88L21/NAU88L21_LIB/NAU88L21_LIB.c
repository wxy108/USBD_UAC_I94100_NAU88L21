/***********************************************************************
 * @
 * @ NAU88L21_LIB.c
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
//----------------------------------------------------------------------
// ISD94124	-	NAU88L21 Hardware Connection
//----------------------------------------------------------------------

// NL-ISD94124A			-		NL-NAU88L21

// PD2 MCLK	PIN38		-		MCLK
// PD6 BCLK PIN42		-		BCLK
// PD5 DO		PIN41		-		DACIN
// PD4 DI	  PIN40		-		ADCOUT
// PD3 LRCK	PIN39		-		FS

// PD14	SCL	PIN47		-		SCL
// PD15	SDA	PIN48		-		SDA

// VCC 3.3V					-		VDDIO
// VCCtoLDO 1.8V		-		VDD18
// VCC 3.3V					-		VDD33
// GND							-		GND

//----------------------------------------------------------------------
// NAU88L21 - INITIAL REGISTER SEQUENCE
//----------------------------------------------------------------------
typedef struct {
	uint8_t  u8DeviceAddr;
	uint16_t u16Counter;
	uint16_t u16MaxCount;
	uint8_t* pau8Cmd;
} S_I2CCTRL;
typedef struct {
	uint8_t  u8Reg[2];
	uint8_t  u8Value[2];
} S_I2CCMD;
volatile uint8_t g_u8CodecInit = 0;
volatile uint16_t g_u16CodecTimeOutCnt = 0;
volatile S_I2CCTRL s_I2CCtrl;

S_I2CCMD NAU88L21_MIC_I2S_SPK[] = {
{	0x00	,	0x00	,	0x00	,	0x00	}	,
{	0x00	,	0x01	,	0x0F	,	0xFF	}	,
{	0x00	,	0x03	,	0x00	,	0x50	}	,
{	0x00	,	0x04	,	0x00	,	0x00	}	,
{	0x00	,	0x05	,	0x00	,	0xBC	}	,
{	0x00	,	0x06	,	0x00	,	0x08	}	,
{	0x00	,	0x07	,	0x00	,	0x10	}	,
{	0x00	,	0x08	,	0x40	,	0x00	}	,
{	0x00	,	0x09	,	0x69	,	0x00	}	,
{	0x00	,	0x0A	,	0x00	,	0x31	}	,
{	0x00	,	0x0B	,	0x26	,	0xE9	}	,
{	0x00	,	0x0D	,	0xC0	,	0x00	}	,
{	0x00	,	0x12	,	0xFF	,	0xFF	}	,
{	0x00	,	0x1C	,	0x00	,	0x0E	}	,
{	0x00	,	0x1D	,	0x00	,	0x00	}	,
{	0x00	,	0x2B	,	0x40	,	0x02	}	,
{	0x00	,	0x2C	,	0x00	,	0x82	}	,
{	0x00	,	0x30	,	0x00	,	0x40	}	,
{	0x00	,	0x34	,	0xCF	,	0xCF	}	,
{	0x00	,	0x35	,	0xCF	,	0xCF	}	,
{	0x00	,	0x4B	,	0x20	,	0x07	}	,
{	0x00	,	0x66	,	0x00	,	0x60	}	,
{	0x00	,	0x6A	,	0x10	,	0x03	}	,
{	0x00	,	0x80	,	0x00	,	0x20	}	,
{	0x00	,	0x72	,	0x04	,	0x70	}	,
{	0x00	,	0x73	,	0x33	,	0x08	}	,
{	0x00	,	0x74	,	0x15	,	0x06	}	,
{	0x00	,	0x76	,	0x31	,	0x40	}	,
{	0x00	,	0x77	,	0x00	,	0x00	}	,
{	0x00	,	0x7E	,	0x1F	,	0x1F	}	,
{	0x00	,	0x7F	,	0xC0	,	0x3C	}	,
{	0x00	,	0x7F	,	0xC0	,	0x3F	}	,
{	0x00	,	0x80	,	0x07	,	0x20	}	,
};
void NAU88L21_INIT(void)
{
    uint16_t u16i;
    s_I2CCtrl.u8DeviceAddr = 0x1B;
    
    g_u8CodecInit = 1;
    g_u16CodecTimeOutCnt = 0;    

    for(u16i=0;u16i<sizeof(NAU88L21_MIC_I2S_SPK)/sizeof(S_I2CCMD);u16i++) 
    {
        s_I2CCtrl.pau8Cmd = (uint8_t*)&NAU88L21_MIC_I2S_SPK[u16i];
        s_I2CCtrl.u16Counter = 0;
        s_I2CCtrl.u16MaxCount = sizeof(S_I2CCMD);
        I2C_START(I2C1);
        // Wait for I2C transmit completed
        while((s_I2CCtrl.u16MaxCount>0) && (g_u16CodecTimeOutCnt < 100)); // 100ms timeout
        if (g_u16CodecTimeOutCnt >= 100)
        {
            //I2C_STOP(I2C1);
            I2C1->CTL |= (I2C_CTL_SI_Msk | I2C_CTL_STO_Msk);
            g_u16CodecTimeOutCnt = 0;
            while(g_u16CodecTimeOutCnt < 3);  // after stop, wait 3ms to ensure I2C has stopped
            break;
        }            
    }
    g_u8CodecInit = 0;
    g_u16CodecTimeOutCnt = 0;
}
//----------------------------------------------------------------------
// NAU88L21 - I2C1
//----------------------------------------------------------------------
void I2C1_INIT(void)
{
    // Reset module
    SYS_ResetModule(I2C1_RST);
    // Enable I2C0 module clock 
    CLK_EnableModuleClock(I2C1_MODULE);
    // Open I2C module and set bus clock
    I2C_Open(I2C1, 100000);
    // GPIO multi-function(GPD14:I2C1_SCL,GPD15:I2C1_SDA)
    SYS->GPD_MFPH  = (SYS->GPD_MFPH & (~SYS_GPD_MFPH_PD14MFP_Msk) ) | SYS_GPD_MFPH_PD14MFP_I2C1_SCL;	
    SYS->GPD_MFPH  = (SYS->GPD_MFPH & (~SYS_GPD_MFPH_PD15MFP_Msk) ) | SYS_GPD_MFPH_PD15MFP_I2C1_SDA;	
		// GPIO Pull-up (GPD14:I2C1_SCL,GPD15:I2C1_SDA)
		GPIO_EnablePullState(PD,BIT14,GPIO_PUSEL_PULL_UP);
		GPIO_EnablePullState(PD,BIT15,GPIO_PUSEL_PULL_UP);
    // Enable I2C interrupt 
    I2C_EnableInt(I2C1);
    NVIC_ClearPendingIRQ(I2C1_IRQn);
    NVIC_EnableIRQ(I2C1_IRQn);	
}
void I2C1_IRQHandler() 
{
    if(I2C_GET_TIMEOUT_FLAG(I2C1)) 
        I2C_ClearTimeoutFlag(I2C1); 
	else 
	{
        uint8_t u8Temp;
          
        switch(I2C_GET_STATUS(I2C1)) {
            /* START has been transmitted and Write SLA+W to Register I2CDAT. */
			case 0x08:
				I2C_SET_DATA(I2C1, s_I2CCtrl.u8DeviceAddr << 1);    
				I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);			
				break;
			/* SLA+W has been transmitted and ACK has been received. */
			case 0x18:
                u8Temp = s_I2CCtrl.u16Counter++;
                u8Temp = s_I2CCtrl.pau8Cmd[u8Temp];
				I2C_SET_DATA(I2C1, u8Temp);
				I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);		
				break;
			/* SLA+W has been transmitted and NACK has been received. */
			case 0x20:
				I2C_STOP(I2C1);
				I2C_START(I2C1);	
				s_I2CCtrl.u16MaxCount = 0;
				break;
			/* DATA has been transmitted and ACK has been received. */
			case 0x28:
                u8Temp = s_I2CCtrl.u16MaxCount;
				if(s_I2CCtrl.u16Counter < u8Temp) {
                u8Temp = s_I2CCtrl.u16Counter++;
					I2C_SET_DATA(I2C1, s_I2CCtrl.pau8Cmd[u8Temp]);
					I2C_SET_CONTROL_REG(I2C1, I2C_CTL_SI);
				} else {
					I2C_SET_CONTROL_REG(I2C1, I2C_CTL_STO_SI);
					s_I2CCtrl.u16MaxCount = 0;
				}
        break;
		}
	}
}
//----------------------------------------------------------------------
// NAU88L21 - I2S0
//----------------------------------------------------------------------
#define I2S_PIN_MASK     (SYS_GPD_MFPL_PD2MFP_Msk|SYS_GPD_MFPL_PD3MFP_Msk|SYS_GPD_MFPL_PD4MFP_Msk|SYS_GPD_MFPL_PD5MFP_Msk|SYS_GPD_MFPL_PD6MFP_Msk)
#define I2S_PIN          (SYS_GPD_MFPL_PD2MFP_I2S0_MCLK|SYS_GPD_MFPL_PD3MFP_I2S0_LRCK|SYS_GPD_MFPL_PD4MFP_I2S0_DI|SYS_GPD_MFPL_PD5MFP_I2S0_DO|SYS_GPD_MFPL_PD6MFP_I2S0_BCLK)
#define I2S_SAMPLE_RATE  (PLAY_RATE)
void I2S_INIT(void)
{
	// Set I2S MFP
	SYS->GPD_MFPL = (SYS->GPD_MFPL&~I2S_PIN_MASK)|I2S_PIN;
	// Enable I2S0 clock.
	CLK_EnableModuleClock(I2S0_MODULE);
	// Select I2S0 clock.
	CLK_SetModuleClock(I2S0_MODULE, CLK_CLKSEL3_I2S0SEL_PLL, NULL);
	// I2S IPReset.
	SYS_ResetModule(I2S0_RST);
	
	// Open I2S0 hardware IP
	I2S_Open(I2S0, I2S_MASTER, 48000, I2S_DATABIT_16, I2S_TDMCHNUM_2CH, I2S_STEREO, I2S_FORMAT_I2S);
	// I2S0 Configuration
	I2S_SET_PCMSYNC(I2S0, I2S_PCMSYNC_BCLK);
	I2S_SET_MONO_RX_CHANNEL(I2S0, I2S_MONO_RX_RIGHT);
	I2S_SET_STEREOORDER(I2S0, I2S_ORDER_EVENLOW);
  I2S_EnableMCLK(I2S0, (48000 * 256));
	// Set channel width.
	I2S_SET_CHWIDTH(I2S0, I2S_CHWIDTH_16);
	// Enable I2S
	I2S_ENABLE(I2S0);
}
void I2S_TX_Start(void)
{
	I2S_CLR_TX_FIFO(I2S0);
  I2S_ENABLE_TXDMA(I2S0);
	I2S_ENABLE_TX(I2S0);
}
void I2S_TX_Stop(void)
{
    g_u16PlayBackReadPtr = 0;
    g_u16PlayBackWritePtr = 0;
    I2S_DISABLE_TXDMA(I2S0);
		I2S_DISABLE_TX(I2S0);
}
void I2S_RX_Start(void)
{
	I2S_CLR_RX_FIFO(I2S0);
  I2S_ENABLE_RXDMA(I2S0);
	I2S_ENABLE_RX(I2S0);
}
void I2S_RX_Stop(void)
{
    g_u16RecordReadPtr = 0;
    g_u16RecordWritePtr = 0;
    g_u8I2S0TxBuffIndex = 0;
		I2S_DISABLE_RXDMA(I2S0);
    I2S_DISABLE_RX(I2S0);
}
//----------------------------------------------------------------------
//  NAU88L21 - PDMA I2S0
//----------------------------------------------------------------------
#define I2S0_RX_PDMA_CH 0
#define I2S0_TX_PDMA_CH 1
typedef struct dma_desc_t 
{
    uint32_t ctl;
    uint32_t endsrc;
    uint32_t enddest;
    uint32_t offset;
} DMA_DESC_T;
DMA_DESC_T DMA_TXDESC[2], DMA_RXDESC[2];

void PDMA_I2S0_TX_Init(void)
{
	// Enable PDMA engine clock.
	CLK_EnableModuleClock(PDMA_MODULE);
	// Reset PDMA module.
	SYS_ResetModule(PDMA_RST);
	
	PDMA_Open((1<<I2S0_TX_PDMA_CH));
	
	// USB_EP3 to PDMA to I2S.
	// Tx description.
	DMA_TXDESC[0].ctl = (((I2S_BUFF_LEN)-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	    |       // transfer byte count   
												PDMA_WIDTH_32               |		
												PDMA_SAR_INC                |       // Source Address Increment
												PDMA_DAR_FIX                |       // Destination Address fixed
												PDMA_REQ_SINGLE	            |       // Transfer Type = single
												PDMA_OP_SCATTER;                    // Basic mode
	DMA_TXDESC[0].endsrc = (uint32_t)&g_au32I2S0TxBuff[0][0];    
	DMA_TXDESC[0].enddest = (uint32_t)&I2S0->TXFIFO;
	DMA_TXDESC[0].offset = ((uint32_t)&DMA_TXDESC[1] - (PDMA->SCATBA));

	DMA_TXDESC[1].ctl = (((I2S_BUFF_LEN)-1)<<PDMA_DSCT_CTL_TXCNT_Pos)       |       // transfer byte count   
												PDMA_WIDTH_32               |		
												PDMA_SAR_INC                |       // Source Address Increment
												PDMA_DAR_FIX                |       // Destination Address fixed
												PDMA_REQ_SINGLE             |       // Transfer Type = single
												PDMA_OP_SCATTER;                    // Basic mode											
	DMA_TXDESC[1].endsrc = (uint32_t)&g_au32I2S0TxBuff[1][0];    
	DMA_TXDESC[1].enddest = (uint32_t)&I2S0->TXFIFO;
	DMA_TXDESC[1].offset = ((uint32_t)&DMA_TXDESC[0] - (PDMA->SCATBA));

	// Request source is memory to memory
	PDMA_SetTransferMode(I2S0_TX_PDMA_CH, PDMA_I2S0_TX, TRUE, DMA_TXDESC[1].offset);
	
	// Enable PDMA channel I2S0_TX_PDMA_CH interrupt
	PDMA_EnableInt(I2S0_TX_PDMA_CH, PDMA_INT_TRANS_DONE);

	NVIC_EnableIRQ(PDMA_IRQn);
}


void PDMA_I2S0_RX_Init(void)
{
	// Enable PDMA engine clock.
	CLK_EnableModuleClock(PDMA_MODULE);
	// Reset PDMA module.
	SYS_ResetModule(PDMA_RST);
	
	PDMA_Open((1<<I2S0_RX_PDMA_CH));
	
	// USB_EP2 to PDMA to I2S.
	// Rx description.
	DMA_RXDESC[0].ctl = (((I2S_BUFF_LEN)-1)<<PDMA_DSCT_CTL_TXCNT_Pos)	    |       // transfer byte count   
												PDMA_WIDTH_32               |		
												PDMA_SAR_FIX                |       // Source Address fixed
												PDMA_DAR_INC                |       // Destination Address increasing
												PDMA_REQ_SINGLE	            |       // Transfer Type = single
												PDMA_OP_SCATTER;                    // Basic mode
	DMA_RXDESC[0].endsrc = (uint32_t)&I2S0->RXFIFO;    
	DMA_RXDESC[0].enddest = (uint32_t)&g_au32I2S0RxBuff[0][0];
	DMA_RXDESC[0].offset = ((uint32_t)&DMA_RXDESC[1] - (PDMA->SCATBA));

	DMA_RXDESC[1].ctl = (((I2S_BUFF_LEN)-1)<<PDMA_DSCT_CTL_TXCNT_Pos)       |       // transfer byte count   
												PDMA_WIDTH_32               |		
												PDMA_SAR_FIX                |       // Source Address fixed
												PDMA_DAR_INC                |       // Destination Address increasing
												PDMA_REQ_SINGLE             |       // Transfer Type = single
												PDMA_OP_SCATTER;                    // Basic mode											
	DMA_RXDESC[1].endsrc = (uint32_t)&I2S0->RXFIFO;   
	DMA_RXDESC[1].enddest = (uint32_t)&g_au32I2S0RxBuff[1][0];
	DMA_RXDESC[1].offset = ((uint32_t)&DMA_RXDESC[0] - (PDMA->SCATBA));

	// Request source is memory to memory
	PDMA_SetTransferMode(I2S0_RX_PDMA_CH, PDMA_I2S0_RX, TRUE, DMA_RXDESC[1].offset);
	
	// Enable PDMA channel I2S0_TX_PDMA_CH interrupt
	PDMA_EnableInt(I2S0_RX_PDMA_CH, PDMA_INT_TRANS_DONE);

	NVIC_EnableIRQ(PDMA_IRQn);
}
void PDMA_IRQHandler(void)
{
	int i;
	uint32_t u32Status;
	uint32_t u32Transferdone;
	
	// Get interrupt status.
	u32Status = PDMA_GET_INT_STATUS();
	
	// TDF-PDMA Channel Transfer Done Flag
	u32Transferdone = PDMA_GET_TD_STS();
	// write 1 to clear
	PDMA_CLR_TD_FLAG(u32Transferdone);

	// PDMA Read/Write Target Abort Interrupt Flag
	if(u32Status & PDMA_STATUS_ABTIF) 				
	{
		//PDMA Channel 2 Read/Write Target Abort Interrupt Status Flag
		if (PDMA_GET_ABORT_STS() & BIT2)  
		{
			PDMA_CLR_ABORT_FLAG(BIT2);
		}	
	}
	else if(u32Status & PDMA_STATUS_TDIF) 	// Transfer Done Interrupt Flag
	{						
		// I2S0 Tx done 
		if(u32Transferdone & (1<<I2S0_TX_PDMA_CH))
		{
			for ( i = 0 ; i < I2S_BUFF_LEN ; i++)
			{
				if(g_u16PlayBackReadPtr != g_u16PlayBackWritePtr)
				{
					if(++g_u16PlayBackReadPtr == MAX_USB_BUFFER_LEN)
						g_u16PlayBackReadPtr = 0;
				
					if (g_usbd_PlayMute)
					{
						g_au32I2S0TxBuff[g_u8I2S0TxBuffIndex][i] = 0x0;
					}
					else
					{
						g_au32I2S0TxBuff[g_u8I2S0TxBuffIndex][i] = g_au32USB_PlaybackBuff[g_u16PlayBackReadPtr];
					}
				}
				else
				{
					g_au32I2S0TxBuff[g_u8I2S0TxBuffIndex][i] = 0x0;
				}
			}
			
			// switch to another buffer row.
			g_u8I2S0TxBuffIndex ^= 0x1;
		}
		if (u32Transferdone & (1<<I2S0_RX_PDMA_CH))
		{
			if (g_usbd_RecMute)
			{
				for (i = 0 ; i < I2S_BUFF_LEN ; i++)
				{
					if (++g_u16RecordWritePtr >= MAX_USB_BUFFER_LEN) 
					{
						g_u16RecordWritePtr = 0;
					}
					
					g_au32USB_RecordBuff[g_u16RecordWritePtr] = 0;
				}
			}
			else
			{			
				if (g_u8AudioMicState == UAC_START_AUDIO_RECORD)
				{
					for (i = 0 ; i < I2S_BUFF_LEN ; i++)
					{
						if (++g_u16RecordWritePtr >= MAX_USB_BUFFER_LEN) 
						{
								g_u16RecordWritePtr = 0;
						}
						
						g_au32USB_RecordBuff[g_u16RecordWritePtr] = g_au32I2S0RxBuff[0][i];
					}
					
					g_u8AudioMicState = UAC_PROCESSING_AUDIO_RECORD;
				}
				else if (g_u8AudioMicState == UAC_PROCESSING_AUDIO_RECORD)
				{
					for (i = 0 ; i < I2S_BUFF_LEN ; i++)
					{
						if (++g_u16RecordWritePtr >= MAX_USB_BUFFER_LEN) 
						{
								g_u16RecordWritePtr = 0;
						}
						
						g_au32USB_RecordBuff[g_u16RecordWritePtr] = g_au32I2S0RxBuff[1][i];
					}
							
					g_u8AudioMicState = UAC_READY_AUDIO_RECORD;
				}
				else if (g_u8AudioMicState == UAC_READY_AUDIO_RECORD)
				{
					for (i = 0 ; i < I2S_BUFF_LEN ; i++)
					{
						if (++g_u16RecordWritePtr >= MAX_USB_BUFFER_LEN) 
						{
								g_u16RecordWritePtr = 0;
						}
						
						g_au32USB_RecordBuff[g_u16RecordWritePtr] = g_au32I2S0RxBuff[0][i];
					}
					
					g_u8I2S0RxBuffIndex = 1;
					g_u8AudioMicState = UAC_BUSY_AUDIO_RECORD;
				}
				// Renew the data from record buffer to PDMA buffer
				else if (g_u8AudioMicState == UAC_BUSY_AUDIO_RECORD)
				{
					for (i = 0 ; i < I2S_BUFF_LEN ; i++)
					{
						if (++g_u16RecordWritePtr >= MAX_USB_BUFFER_LEN) 
						{
								g_u16RecordWritePtr = 0;
						}
						
						g_au32USB_RecordBuff[g_u16RecordWritePtr] = g_au32I2S0RxBuff[g_u8I2S0RxBuffIndex][i];               
					}
					
					// Switch to another buffer row.
					g_u8I2S0RxBuffIndex ^= 1;
				}
			}
		}
	}
}
/* @ Copyright (C) 2021 Nuvoton Technology Corporation America */
