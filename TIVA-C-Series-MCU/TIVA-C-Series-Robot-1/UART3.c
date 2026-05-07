/**
 * @file UART3.c
 *
 * @brief Source code for the UART3 driver.
 *
 * This file contains the function definitions for the UART3 driver.
 *
 * @note For more information regarding the UART module, refer to the
 * Universal Asynchronous Receivers / Transmitters (UARTs) section
 * of the TM4C123GH6PM Microcontroller Datasheet.
 * Link: https://www.ti.com/lit/gpn/TM4C123GH6PM
 *
 * @note Assumes that the system clock (50 MHz) is used.
 *
 * @author Aaron Nanas
 */

#include "UART3.h"

void UART3_Init(void)
{
	// Enable the clock to UART3 module by setting the 
	// R3 bit (Bit 3) in the RCGCUART register
	SYSCTL->RCGCUART |= 0x08;
	
	// Enable the clock to Port C by setting the
	// R2 bit (Bit 2) in the RCGCGPIO register
	SYSCTL->RCGCGPIO |= 0x04;  // since the name for por C is called R2( 2nd bit), we create a mask of the 0000_0100b = 0x04
	
	// Configure the PC7 (U3TX) and PC6 (U3RX) pins to use the alternate function
	// by setting Bits 7 to 6 in the AFSEL register
	GPIOC->AFSEL |= 0xC0;
	
	// Clear the PMC7 (Bits 31 to 28) and PMC6 (Bits 27 to 24) fields in the PCTL register before configuration
	GPIOC->PCTL &= ~0xFF000000;  //GPIOPCTL is the Port Control register
	
	// Configure the PC7 pin to operate as a U3TX pin by writing 0x1 to the
	// PMC7 field (Bits 31 to 28) in the PCTL register
	// The 0x1 value is derived from Table 23-5 in the TM4C123G Microcontroller Datasheet
	GPIOC->PCTL |= 0x10000000;
	
	// Configure the PC6 pin to operate as a U3RX pin by writing 0x1 to the //Simplex communication:  allows tranfeer for information only one direction.
	// PMC6 field (Bits 27 to 24) in the PCTL register
	// The 0x1 value is derived from Table 23-5 in the TM4C123G Microcontroller Datasheet
	GPIOC->PCTL |= 0x01000000;
	
	// Enable the digital functionality for the PC7 and PC6 pins
	// by setting Bits 7 to 6 in the DEN register
	GPIOC->DEN |= 0xC0;
	
	// Disable the UART3 module before configuration by clearing
	// the UARTEN bit (Bit 0) in the CTL register				
	UART3->CTL &= ~0x0001;  // UART Control
	
	// Specify the UART clock source to use the system clock by
	// writing a value of 0x0 to the CS field (Bits 3 to 0) in the CC register
	UART3->CC &= ~0xF;  //clock configuration
	
	// Configure the UART3 module to use the system clock (50 MHz)
	// divided by 16 by clearing the HSE bit (Bit 5) in the CTL register
	UART3->CTL &= ~0x0020;
	
	
	
	//****** Baud Rate calculation example from professor ***********
	// Set the baud rate by writing to the DIVINT field (Bits 15 to 0)
	// and the DIVFRAC field (Bits 5 to 0) in the IBRD and FBRD registers, respectively.
	// The integer part of the calculated constant will be written to the IBRD register,
	// while the fractional part will be written to the FBRD register.
	// BRD = (System Clock Frequency) / (16 * Baud Rate)
	// BRDI = (50,000,000) / (16 * 9600) = 325.5208333 (IBRD = 325)
	// BRDF = ((0.5208333 * 64) + 0.5) = 33.8333 (FBRD = 33)
	
	
	//********* Baud Rate from From ESP32***************************//baud rate(BD) is the common data rate between the TX and RX
	// Set the baud rate by writing to the DIVINT field (Bits 15 to 0)
	// and the DIVFRAC field (Bits 5 to 0) in the IBRD and FBRD registers, respectively.
	// The integer part of the calculated constant will be written to the IBRD register,
	// while the fractional part will be written to the FBRD register.
	// BRD = (System Clock Frequency) / (16 * Baud Rate)
	// BRDI = (50,000,000) / (16 * 115200) = 27.12673611 (IBRD = 27)
	// BRDF = ((0.12673611 * 64) + 0.5) = 8.611 (FBRD = 8)
	UART3->IBRD = 27;
	UART3->FBRD = 8;
	
	// Unit for UART is bits per seconds(bps) or also reffered to as baud.
	// BR(baud rate) = 1/BD   (BD is bit duration)
	//the bit duration(BD) AKA (bit period TB) is the amount of time that the bit is present on the data line.
	//BD(bit duration) = 1/DR  (inverse of baud rate)
	//BDR(baud rate) = BRDI(integer) + BRDF(fractional)
	
	//********** calculate Data Rate *************//UART Frame: describes how bits are arranged in the AUART serial sequence.
	// ESP32 UART is configured with the following parameter
	//** 115200 = 115.2 kbps baud rate
	//** 8 Data bits						// data are known as payload
	//** no parity bits
	//** 1 stop bit         //stop, start, parity are overhead
	//** 10 Total bits(1 start, 1 stop, 8 data) needed to send 8 bits(1 byte) of data
	//** Data Rate = BD * ( Data bits / Total bits)
	//* Data Rate = 115.2kbps * (8bits /10 bits) = 92.16kbps(kilo bits)
	
	// How many KB(kilo Bytes)
	// 1 byte = 8 bits
	// 1 KB = 1024 bytes
	// Total bits = 1024 bytes * (8bits / byte) = 8192 bits
	// KB/s = (Data Rate) * (1 KB/ 8192 bits)
	// KB/s = (92160) * (1 KB/ 8192 bits) = 11.52KBs





	// Configure the data word length of the UART packet to be 8 bits by 
	// writing a value of 0x3 to the WLEN field (Bits 6 to 5) in the LCRH register
	UART3->LCRH |= 0x60;  //line control register. see UART lecture Recording minute 53
	
	// Enable the transmit and receive FIFOs by setting the FEN bit (Bit 4) in the LCRH register
	UART3->LCRH |= 0x10;
	
	// Select one stop bit to be transmitted at the end of a UART frame by
	// clearing the STP2 bit (Bit 3) in the LCRH register
	UART3->LCRH &= ~0x08;
	
	// Disable the parity bit by clearing the PEN bit (Bit 1) in the LCRH register
	UART3->LCRH &= ~0x02;
	
	
	// Enable the UART3 module after configuration by setting
	// the UARTEN bit (Bit 0) in the CTL register
	UART3->CTL |= 0x01;
}

char UART3_Input_Character(void)//constantly check for received FIFO, while it not empty read(return) as a bit data
{
	while((UART3->FR & UART3_RECEIVE_FIFO_EMPTY_BIT_MASK) != 0); //the status of the receive and transmit FIFOs can be checked using UART Flag Register USRTRFR
	
	return (char)(UART3->DR & 0xFF); // UART Data (UARTDR) register
}
 
void UART3_Output_Character(char data)//wait until is Full anymore,to send data
{
	while((UART3->FR & UART3_TRANSMIT_FIFO_FULL_BIT_MASK) != 0); //before reading Data, make sure that there is data avialable in the receive FIFO. RXFE =1 means that the receive FIFO is empty, RXFE=0, then start the read operation.
	
	UART3->DR = data;			//before transmiiting data, make sure transmit FIFO is not full. TXFF=1 means that the transmit FIFO is full, softare needs to wait until UART moudule transmit dta. TXFF=0 , then start the write operation.
}

int8_t UART3_DataAvailable(void)
{
    if ((UART3->FR & UART3_RECEIVE_FIFO_EMPTY_BIT_MASK) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

