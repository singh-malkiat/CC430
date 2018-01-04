// *************************************************************************************************
//
//	Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/ 
//	 
//	 
//	  Redistribution and use in source and binary forms, with or without 
//	  modification, are permitted provided that the following conditions 
//	  are met:
//	
//	    Redistributions of source code must retain the above copyright 
//	    notice, this list of conditions and the following disclaimer.
//	 
//	    Redistributions in binary form must reproduce the above copyright
//	    notice, this list of conditions and the following disclaimer in the 
//	    documentation and/or other materials provided with the   
//	    distribution.
//	 
//	    Neither the name of Texas Instruments Incorporated nor the names of
//	    its contributors may be used to endorse or promote products derived
//	    from this software without specific prior written permission.
//	
//	  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//	  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//	  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//	  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//	  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//	  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//	  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//	  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//	  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//	  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//	  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ******************************************************************************
// ------------------------------------------------------------------------------
// 	MagicCube - Firmware
//		
// 	 CC430F137 low power transmission
//   Modified by:  Malkiat Singh
//	
//	c-kuch@ti.com
//
//	Revision
//		0.10		2013-09-01		add to support TPS62740 &
//                                  change solar side assignment "CheckSide()"
//                                  [c.kuch]
//      0.10        2013-10-15      change project name (spez.: NR1x)
//      1.00        2013-11-14      Final Version
//
//	Plattform: Code Composer Studio  Version: 5.1.0.08032
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// system
#include "project.h"
//------------------------------------------------------------------------------
// Includes
#include "RF1A.h"
#include "cc430F5147.h"
#include "cc430x514x_ADC10.h"
#include "cc430x514x_PMM.h"
#include "cc430x514x_PMA.h"
#include "acceleration.h"
#include "as.h"
#include "bmp_as.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

//------------------------------------------------------------------------------

#define MODULE "EM001TPV"
#define PVCC_MIN 2100 //mV
#define PVCC_MAX 4000 //mV
#define wait 1 //startup wait (sec) 5
#define tcount 1 //offset time(s) = tcount * 2
#define BufferLength 48

#ifdef ISM_EU
    // 868MHz
    #define SETTING_FREQ2   0x21
    #define SETTING_FREQ1   0x62
    #define SETTING_FREQ0   0x76
#else
  #ifdef ISM_US
    // 914.99MHz
    #define SETTING_FREQ2   0x23
    #define SETTING_FREQ1   0x31
    #define SETTING_FREQ0   0x3B
  #else
    #ifdef ISM_LF
      // 433.92MHz
      #define SETTING_FREQ2   0x10
      #define SETTING_FREQ1   0xB0
      #define SETTING_FREQ0   0x3B
	#else
      #error "Wrong ISM band specified (valid are ISM_LF, ISM_EU and ISM_US)"
    #endif // ISM_LF
  #endif // ISM_US
#endif // ISM_EU

//------------------------------------------------------------------------------
// Function Prototypes
void initButton(void);
void initRadio(void);
void IntTransmit(void);
void ReadParameter(void);
void configure_ports(void);
void ItoA( int z, char* Buffer );
int CheckSide(void);
void TransmitAS(void);

//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// Global Variables
int iBufferLength;
int buffer[3];
int i, powerup;
int counter = wait;
int iSide1 = 0;
int iRandom;
int iAS_flag = 0;

char txBuffer[BufferLength], txBufferT[BufferLength];

// Variables used for FEC encoding and CRC calculation
unsigned short j, val, fecReg, fecOutput;
unsigned long intOutput;
unsigned short inputNum = 0, fecNum;
unsigned short checksum;


const unsigned char fecEncodeTable[] = {
  0, 3,
  1, 2,
  3, 0,
  2, 1,
  3, 0,
  2, 1,
  0, 3,
  1, 2   
};

// Variables used for multiply calculation
unsigned int Result_lower16;
unsigned int Result_upper16;


// Flag set in the ISR when a packet is sent or a button is pushed
extern unsigned char packetTransmit;    
unsigned char buttonPressed = 0;

// NUMBER_OF_BYTES_BEFORE_ECODING should be given the length of the 
// payload without CRC
#define NUMBER_OF_BYTES_BEFORE_ECODING    29  
#define NUMBER_OF_BYTES_AFTER_ECODING     (4 * (((NUMBER_OF_BYTES_BEFORE_ECODING + 2) / 2) + 1))
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//  void main(void)
//
//  Description:
//    This code generate a random packet of length 
//    NUMBER_OF_BYTES_BEFORE_ECODING, calculates and add the CRC to the packet, 
//    encode, and interleaved the data, and transmit one packet every time the 
//    button is pushed
//------------------------------------------------------------------------------
void main(void)
{  
  
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;   

  TA0CCTL0 = CCIE;                            // CCR0 interrupt enabled
  TA0CCR0 = 1024;							  // toogle every ~0.25s;
  TA0CTL = TASSEL_1 + MC_2 + TACLR;           // SMCLK, contmode, clear TAR
  
  
  SetVCore(2);
  configure_ports();
  
// ---------------------------------------------------------------------
  // Init acceleration sensor
  as_init();

  powerup = 0;
  
//----------------------------------------------------------------------------
// Loop for charge PV cap
  while (powerup < wait) {


	  // Turn off SVSH, SVSM
	  PMMCTL0_H = 0xA5;
	  SVSMHCTL = 0;
	  SVSMLCTL = 0;
	  PMMCTL0_H = 0x00;

    __bis_SR_register(LPM3_bits + GIE);       // Enter LPM3, enable interrupts
    __no_operation();
  }

  initRadio();
  
  
  
//----------------------------------------------------------------------------
// Loop for transmit data
  while (1) {
    __bis_SR_register(LPM3_bits + GIE);       // Enter LPM3, enable interrupts
    __no_operation();
  }
}   

void IntTransmit(void)
{
    Transmit( (unsigned char*)txBuffer, iBufferLength);
    Strobe( RF_SXOFF );                          // Strobe RF Powerdown   
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void ReadParameter(void)
{

	if (counter >= tcount)
	{
		read_voltage( (int*)buffer);
	}
	else
	{
		buffer[1] = PVCC_MIN-100;
	} 
	  
  	if (buffer[1] < PVCC_MIN-200)
  	{
  		powerup = 0;
 		PMMCTL0_H = 0xA5;	// PMM Password
 		PMMCTL0_L = 0x04;	// BOR Reset
  	}
  	if (buffer[1] < PVCC_MIN)
  	{
   		counter++;
  	}
  	else
  	{
  	    TransmitAS();
  	}
}

void TransmitAS()
{
	int iSide;
  		get_acceleration();
 		iSide = CheckSide();
  		if (iSide > 0 && iSide < 7 && iSide != iSide1) {
  			switch (iSide)  // NR2x
  			{
				case 0:
					iSide = 0;
					break;
  				case 1:
  					txBuffer[1] = '1';
  					txBuffer[2] = '1';
  					break;
  				case 2:
  					txBuffer[1] = '1';
  					txBuffer[2] = '2';
  					break;
  				case 3:
  					txBuffer[1] = '1';
  					txBuffer[2] = '3';
  					break;
  				case 4:
  					txBuffer[1] = '1';
  					txBuffer[2] = '4';
  					break;
  				case 5:
  					txBuffer[1] = '1';
  					txBuffer[2] = '5';
  					break;
  				case 6:
  					txBuffer[1] = '1';
  					txBuffer[2] = '6';
  					break;
  			}
  			iSide1 = iSide;
  			txBuffer[0] = 0x03;
  			txBuffer[3] = 0x00;
  			Transmit( (unsigned char*)txBuffer, 0x04);
  		    Strobe( RF_SXOFF );                          // Strobe RF Powerdown

  			counter = 1;
  		}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

int CheckSide(void)
{
	if (sAccel.xyz[0] > 55 &&  sAccel.xyz[0] < 70){
		return(4);
	}
	else if (sAccel.xyz[0] > 180 &&  sAccel.xyz[0] < 200){
  	  	return(3);
  	}
  	else if (sAccel.xyz[1] > 55 &&  sAccel.xyz[1] < 70)	{
  	  	return(1);
  	}
  	else if (sAccel.xyz[1] > 180 &&  sAccel.xyz[1] < 200)	{
  		return(6);
  	}
  	else if (sAccel.xyz[2] > 55 &&  sAccel.xyz[2] < 70)	{
  	  	return(2);
  	}
  	else if (sAccel.xyz[2] > 180 &&  sAccel.xyz[2] < 200)	{
  	  	return(5);
  	}
	return(0);
}
//------------------------------------------------------------------------------
//  void initRadio(void)
//
//  Description:
//    REset and configure radio
//
//------------------------------------------------------------------------------
void initRadio(void)
{
  ResetRadioCore();     
  WriteSingleReg(IOCFG2,    0x29);
  WriteSingleReg(IOCFG1,    0x2E);                          
  WriteSingleReg(FIFOTHR,   0x07);                          
  WriteSingleReg(SYNC1,     0xD3);                          
  WriteSingleReg(SYNC0,     0x91);                          
  WriteSingleReg(PKTLEN,    0xFF);
  WriteSingleReg(PKTCTRL1,  0x04);                          
  WriteSingleReg(PKTCTRL0,  0x05);                          
  WriteSingleReg(ADDR,      0x00);                          
  WriteSingleReg(CHANNR,    0x00);                          
  WriteSingleReg(FSCTRL1,   0x0C);                          
  WriteSingleReg(FSCTRL0,   0x00);                          
  WriteSingleReg(FREQ2,     SETTING_FREQ2);
  WriteSingleReg(FREQ1,     SETTING_FREQ1);
  WriteSingleReg(FREQ0,     SETTING_FREQ0);
  WriteSingleReg(MDMCFG4,   0x2D);                          
  WriteSingleReg(MDMCFG3,   0x3B);                          
  WriteSingleReg(MDMCFG2,   0x13);                          
  WriteSingleReg(MDMCFG1,   0x22);                          
  WriteSingleReg(MDMCFG0,   0xF8);                          
  WriteSingleReg(DEVIATN,   0x62);                          
  WriteSingleReg(MCSM2,     0x07);                          
  WriteSingleReg(MCSM1,     0x30);                          
  WriteSingleReg(MCSM0,     0x10);                          
  WriteSingleReg(FOCCFG,    0x1D);                          
  WriteSingleReg(BSCFG,     0x1C);                          
  WriteSingleReg(AGCCTRL2,  0xC7);                          
  WriteSingleReg(AGCCTRL1,  0x00);                          
  WriteSingleReg(AGCCTRL0,  0xB0);                          
  WriteSingleReg(WOREVT1,   0x87);                          
  WriteSingleReg(WOREVT0,   0x6B);                          
  WriteSingleReg(WORCTRL,   0xFB);                          
  WriteSingleReg(FREND1,    0xB6);                          
  WriteSingleReg(FREND0,    0x10);                          
  WriteSingleReg(FSCAL3,    0xEA);                          
  WriteSingleReg(FSCAL2,    0x2A);                          
  WriteSingleReg(FSCAL1,    0x00);                          
  WriteSingleReg(FSCAL0,    0x1F);                          
  WriteSingleReg(FSTEST,    0x59);                          
  WriteSingleReg(PTEST,     0x7F);                          
  WriteSingleReg(TEST2,     0x88);                          
  WriteSingleReg(TEST1,     0x31);                          
  WriteSingleReg(TEST0,     0x09);                          
  
  WritePATable(0x7E51);  // 0dbm
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//  void ItoA( int z, char* Buffer )
//
//  Description:
//    Convert integer to string
//
//------------------------------------------------------------------------------
void ItoA( int z, char* Buffer )
{
  int i = 0;
  int j;
  char tmp;
  unsigned u;
    // Is the number negativ?
    // Remove the -
    if( z < 0 ) {
      Buffer[0] = '-';
      Buffer++;
      u = ( (unsigned)-(z+1) ) + 1;
    }
    else {
      u = (unsigned)z;
    }
    do {
      Buffer[i++] = '0' + u % 10;
      u /= 10;
    } while( u > 0 );

    // den String in sich spiegeln
    for( j = 0; j < i / 2; ++j ) {
      tmp = Buffer[j];
      Buffer[j] = Buffer[i-j-1];
      Buffer[i-j-1] = tmp;
    }
    Buffer[i] = 0;
}
//------------------------------------------------------------------------------

// Timer1 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
	if (powerup > wait)
	{
		ReadParameter();
		__bic_SR_register_on_exit(LPM3_bits); // Exit active
	}
	else
	{
  		powerup++;
		if (powerup == wait) 
		{
			counter = wait;
			__bic_SR_register_on_exit(LPM3_bits); // Exit active
		}
	}
	TA0CCR0 = 1024;
}

//------------------------------------------------------------------------------
// *************************************************************************************************
// @fn          PORT2_ISR
// @brief       Interrupt service routine for
//
//              - acceleration sensor CMA_INT
//
// @param       none
// @return      none
// *************************************************************************************************
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{
    __bic_SR_register_on_exit(LPM3_bits); // Exit active
}

