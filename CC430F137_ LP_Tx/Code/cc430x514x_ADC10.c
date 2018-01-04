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
//****************************************************************************//
// Function Library for setting the PMA
//
//  
//  
// 
//****************************************************************************//
#include "cc430F5147.h"
#include "cc430x514x_ADC10.h"

int results[2];

void read_voltage(int *buffer)
{
  	P3OUT |= BIT0;							  // Output high switch Q1 on
 	P3DIR |= BIT0;							  // Set P3.0 to output direction

    __delay_cycles(75);                       // Delay (~75us)

    ADC10CTL0 = ADC10SHT_2 + ADC10ON;         // 16 ADC10CLKs; ADC ON
    ADC10CTL1 = ADC10SHP + ADC10CONSEQ_0;     // s/w trig, single ch/conv
    ADC10CTL2 = ADC10RES;                     // 10-bit conversion results
    ADC10MCTL0 = ADC10SREF_0 + ADC10INCH_11;  // AVcc/2

    // Configure internal reference
    while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_0+REFON;               // Select internal ref = 1.5V
                                              // Internal Reference ON
    __delay_cycles(75);                       // Delay (~75us) for Ref to settle

    ADC10CTL0 |= ADC10ENC + ADC10SC;          // Sampling and conversion start
    while (ADC10CTL1 & ADC10BUSY);            // ADC10BUSY?
    __no_operation();                       // For debug only
    buffer[0] = (int)((ADC10MEM0 * 2.0f / 1024) * 2.11 * 1000);
    ADC10CTL0 &= ~(ADC10ENC + ADC10SC);       // Sampling stop
    __no_operation();        				  // For debug only
    REFCTL0 &= ~(REFVSEL_0+REFON);            // Shutdown

    ADC10MCTL0 = ADC10SREF_1 + ADC10INCH_2;   // A2

    ADC10CTL0 |= ADC10ENC + ADC10SC;          // Sampling and conversion start
    while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
    REFCTL0 |= REFVSEL_2+REFON;               // Select internal ref = 2.5V
                                              // Internal Reference ON
    __delay_cycles(75);                       // Delay (~75us) for Ref to settle

    ADC10CTL0 |= ADC10ENC + ADC10SC;          // Sampling and conversion start
    while (ADC10CTL1 & ADC10BUSY);            // ADC10BUSY?
    __no_operation();                       // For debug only
    buffer[1] = (int)((ADC10MEM0 * 2.0f / 1024) * 2.4 * 1000);
    __no_operation();                       // For debug only

    REFCTL0 &= ~(REFVSEL_2+REFON);            // Shutdown

//  	P2OUT = 0x00;
// 	P2DIR = 0xFF;							  // Set P2.x to output direction

    P3OUT &= ~BIT0;							  // Output high switch Q1 off
 	P3DIR &= ~BIT0;							  // Set P3.0 to input direction

 	P2OUT &= ~BIT2;
 	P2DIR &= ~BIT2;							  // Set P2.2 to output direction
}
