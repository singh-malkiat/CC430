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
#include "RF1A.h"
#include "cc430F5147.h"

unsigned char packetReceived = 0;
unsigned char packetTransmit = 0; 

unsigned char Strobe(unsigned char strobe)
{
  unsigned char statusByte, gdo_state;
  unsigned int i, j;
  
  // Check for valid strobe command 
  if((strobe == 0xBD) || ((strobe > RF_SRES) && (strobe < RF_SNOP)))
  {	  
	  // Clear the Status read flag 
	  RF1AIFCTL1 &= ~(RFSTATIFG);
	  
	  // Wait for radio to be ready for next instruction
	  while( !(RF1AIFCTL1 & RFINSTRIFG));

      // Write the strobe instruction
	  if ((strobe > RF_SRES) && (strobe < RF_SNOP))
	  {
                gdo_state = ReadSingleReg(IOCFG2);
                WriteSingleReg(IOCFG2,0x29);
                
                RF1AINSTRB = strobe; 
	  	if ((RF1AIN&0x04)== 0x04 )                // chip at sleep mode
                {
                  if((strobe == RF_SXOFF) || (strobe == RF_SPWD) ||(strobe == RF_SWOR))
                  {
                  }
                  else
            	  {
		    while ((RF1AIN&0x04)== 0x04);       // c-ready ?
		    for (j=0;j<76; j++) { for (i=0; i<12; i++); } // delay 76x10us = 760us (12MHz CPU clock)
	  	  }
                }
                WriteSingleReg(IOCFG2,gdo_state);
	  }
	  else		// chip active mode
	  {	
            RF1AINSTRB = strobe; 	   
	  }
          statusByte = RF1ASTATB;
          while( !(RF1AIFCTL1 & RFSTATIFG) );
  }
  return statusByte;
}

unsigned char ReadSingleReg(unsigned char addr)
{
  unsigned char data_out;
  
  // Check for valid configuration register address, 0x3E refers to PATABLE 
  if ((addr <= 0x2E) || (addr == 0x3E))
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_SNGLREGRD);    
  else
    // Send address + Instruction + 1 dummy byte (auto-read)
    RF1AINSTR1B = (addr | RF_STATREGRD);    
  
  while (!(RF1AIFCTL1 & RFDOUTIFG) );
  data_out = RF1ADOUT1B;                    // Read data and clears the RFDOUTIFG

  return data_out;
}

void WriteSingleReg(unsigned char addr, unsigned char value)
{   
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRB = (addr | RF_REGWR);			// Send address + Instruction

  RF1ADINB = value; 						// Write data in 

  __no_operation(); 
}
        

void ReadBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{
  unsigned int i;
  
  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for INSTRIFG
  RF1AINSTR1B = (addr | RF_REGRD);          // Send addr of first conf. reg. to be read 
                                            // ... and the burst-register read instruction
  for (i = 0; i < (count-1); i++)
  {
    while (!(RFDOUTIFG&RF1AIFCTL1));        // Wait for the Radio Core to update the RF1ADOUTB reg
    buffer[i] = RF1ADOUT1B;                 // Read DOUT from Radio Core + clears RFDOUTIFG
                                            // Also initiates auo-read for next DOUT byte
  }
  buffer[count-1] = RF1ADOUT0B;             // Store the last DOUT from Radio Core
}  

void WriteBurstReg(unsigned char addr, unsigned char *buffer, unsigned char count)
{  
  // Write Burst works wordwise not bytewise - known errata
  unsigned char i;

  while (!(RF1AIFCTL1 & RFINSTRIFG));       // Wait for the Radio to be ready for next instruction
  RF1AINSTRW = ((addr | RF_REGWR)<<8 ) + buffer[0]; // Send address + Instruction

  for (i = 1; i < count; i++)
  {
    RF1ADINB = buffer[i];                   // Send data
    while (!(RFDINIFG & RF1AIFCTL1));       // Wait for TX to finish
  } 
  i = RF1ADOUTB;                            // Reset RFDOUTIFG flag which contains status byte
}

void WriteSmartRFReg(const unsigned char SmartRFSetting[][2], unsigned char size)
{
  unsigned char i;
  for (i=0; i < (size); i++)
    WriteSingleReg(SmartRFSetting[i][0], SmartRFSetting[i][1]);
}

void ResetRadioCore (void)
{
  Strobe(RF_SRES);                          // Reset the Radio Core
  Strobe(RF_SNOP);                          // Reset Radio Pointer
}


void WritePATable(int iPatable)
{
	unsigned char valueRead = 0;
    /* Write the power output to the PA_TABLE and verify the write operation.  */

    /* wait for radio to be ready for next instruction */
    while( !(RF1AIFCTL1 & RFINSTRIFG));
    RF1AINSTRW = iPatable; // PA Table write (burst)
    
    /* wait for radio to be ready for next instruction */
    while( !(RF1AIFCTL1 & RFINSTRIFG));
      RF1AINSTR1B = RF_PATABRD;                 // -miguel read & write
    
    while( !(RF1AIFCTL1 & RFDOUTIFG));
    valueRead  = RF1ADOUTB;
}

void Transmit(unsigned char *buffer, unsigned char length)
{
  RF1AIES |= BIT9;                          
  RF1AIFG &= ~BIT9;                         // Clear pending interrupts
  RF1AIE |= BIT9;                           // Enable RFIFG9 TX end-of-packet interrupts 
  
  /* RF1AIN_9 => Rising edge indicates SYNC sent/received and
   *             Falling edge indicates end of packet.
   *             Configure it to interrupt on falling edge.
   */    
  WriteBurstReg(RF_TXFIFOWR, buffer, length);     
  
  Strobe( RF_STX );                         // Strobe STX   
}

void ReceiveOn(void)
{  
  RF1AIFG &= ~BIT4;                         // Clear a pending interrupt
  RF1AIE  |= BIT4;                          // Enable the interrupt 
  
  Strobe( RF_SIDLE );
  Strobe( RF_SRX );                      
}

void ReceiveOff(void)
{
  RF1AIE &= ~BIT4;                          // Disable RX interrupts
  RF1AIFG &= ~BIT4;                         // Clear pending IFG

  Strobe( RF_SIDLE );
  Strobe( RF_SFRX);      /* Flush the receive FIFO of any residual data */
}

// Called if an interface error has occured. No interface errors should 
// exist in application code, so this is intended to be used for debugging
// or to catch errant operating conditions. 
static void RF1A_interface_error_handler(void)
{
  switch(__even_in_range(RF1AIFERRV,8))
  {
    case 0: break;                          // No error
    case 2:                                 // Low core voltage error     
      P1OUT &= ~BIT0;						// 00 = on LED's [D2,D1]
      P3OUT &= ~BIT6; 
      __no_operation();
      break; 
    case 4:                                 // Operand Error
      P1OUT |= BIT0;						// 01 = on LED's [D2,D1]
      P3OUT &= ~BIT6; 
      __no_operation();
      break;  
    case 6:                                 // Output data not available error 
      P1OUT &= ~BIT0;						// 10 = on LED's [D2,D1]
      P3OUT |= BIT6; 
      __no_operation();
      break;
    case 8:                                 // Operand overwrite error
      P1OUT |= BIT0;						// 11 = on LED's [D2,D1]
      P3OUT |= BIT6; 
      __no_operation();
      break; 
  }
}

// If RF1A_interrupt_handler is called, an interface interrupt has occured.
static void RF1A_interrupt_handler(void)
{
  // RF1A interrupt is pending
  switch(__even_in_range(RF1AIFIV,14))
  {
    case  0: break;                         // No interrupt pending
    case  2:                                // RFERRIFG 
      RF1A_interface_error_handler();
    case  4: break;                         // RFDOUTIFG
    case  6: break;                         // RFSTATIFG
    case  8: break;                         // RFDINIFG
    case 10: break;                         // RFINSTRIFG
    case 12: break;                         // RFRXIFG
    case 14: break;                         // RFTXIFG
  }
}

#pragma vector=CC1101_VECTOR
__interrupt void CC1101_ISR(void)
{
  switch(__even_in_range(RF1AIV,32))        // Prioritizing Radio Core Interrupts 
  {
    case  0:                                // No RF core interrupt pending                                            
      RF1A_interrupt_handler();             // means RF1A interface interrupt pending
      break; 
    case  2: break;                         // RFIFG0 
    case  4: break;                         // RFIFG1
    case  6: break;                         // RFIFG2
    case  8: break;                         // RFIFG3
    case 10:                                // RFIFG4 - RX end-of-packet
      packetReceived = 1;    
      break;
    case 12: break;                         // RFIFG5
    case 14: break;                         // RFIFG6          
    case 16: break;                         // RFIFG7
    case 18: break;                         // RFIFG8
    case 20:                                // RFIFG9 - TX end-of-packet
      packetTransmit = 1;                   
      break;
    case 22: break;                         // RFIFG10
    case 24: break;                         // RFIFG11
    case 26: break;                         // RFIFG12
    case 28: break;                         // RFIFG13
    case 30: break;                         // RFIFG14
    case 32: break;                         // RFIFG15
  }  
  __bic_SR_register_on_exit(LPM0_bits);     
}
