/**
 * @file PWM0_0.c
 *
 * @brief Source file for the PWM0_0 driver.
 *
 * This file contains the function definitions for the PWM0_0 driver.
 * It uses the Module 0 PWM Generator 0 to generate a PWM signal using the PB6 pin.
 *
 * @note This driver assumes that the system clock's frequency is 50 MHz.
 *
 * @note This driver assumes that the PWM_Clock_Init function has been called
 * before calling the PWM0_0_Init function.
 *
 * @author Aaron Nanas
 */

#include "PWM0_0.h"

void PWM0_0_Init(uint16_t period_constant, uint16_t duty_cycle)
{	
	// Return from the function if the specified duty_cycle is greater than
	// or equal to the given period. The duty cycle cannot exceed 99%.
	if (duty_cycle >= period_constant) return;
	
	// Enable the clock to PWM Module 0 by setting the
	// R0 bit (Bit 0) in the RCGCPWM register
	SYSCTL->RCGCPWM |= 0x01;
	
	// Enable the clock to GPIO Port B by setting the
	// R1 bit (Bit 1) in the RCGCGPIO register
	SYSCTL->RCGCGPIO |= 0x02;
	
	// Configure the PB6 pin to use the alternate function (M0PWM0)
	// by setting Bit 6 in the AFSEL register
	GPIOB->AFSEL |= 0x40;
	
	// Clear the PMC6 field (Bits 27 to 24) in the PCTL register
	GPIOB->PCTL &= ~0x0F000000;
	
	// Configure the PB6 pin to operate as a Module 0 PWM0 pin (M0PWM0)
	// by writing 0x4 to the PMC6 field (Bits 27 to 24) in the PCTL register
	// The 0x4 value is derived from Table 23-5 in the TM4C123G Microcontroller Datasheet
	GPIOB->PCTL |= 0x04000000;
	
	// Enable the digital functionality for the PB6 pin
	// by setting Bit 6 in the DEN register
	GPIOB->DEN |= 0x40;
	
	// Disable the Module 0 PWM Generator 0 block (PWM0_0) before 
	// configuration by clearing the ENABLE bit (Bit 0) in the PWM0CTL register
	PWM0->_0_CTL &= ~0x01;
	
	// Configure the counter for the PWM0_0 block to
	// use Count-Down mode by clearing the MODE bit (Bit 1)
	// in the PWM0CTL register. The counter will count from the load value
	// to 0, and then wrap back to the load value
	PWM0->_0_CTL &= ~0x02;
	
	// Set the ACTCMPAD field (Bits 7 to 6) to 0x3 in the PWM0GENA register
	// to drive the PWM signal high when the counter matches 
	// the comparator (i.e. the value in PWM0CMPA) while counting down
	PWM0->_0_GENA |= 0xC0;
	
	// Set the ACTLOAD field (Bits 3 to 2) to 0x2 in the PWM0GENA register
	// to drive the PWM signal low when the counter matches the value
	// in the PWM0LOAD register
	PWM0->_0_GENA |= 0x08;
	
	// Set the period by writing to the LOAD field (Bits 15 to 0) 
	// in the PWM0LOAD register. This determines the number of clock
	// cycles needed to count down to zero
	PWM0->_0_LOAD = (period_constant - 1);
	
	// Set the duty cycle by writing to the COMPA field (Bits 15 to 0)
	// in the PWM0CMPA register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_0_CMPA = (duty_cycle - 1);
	
	// Enable the PWM0_0 block after configuration by setting the
	// ENABLE bit (Bit 0) in the PWM0CTL register
	PWM0->_0_CTL |= 0x01;
	
	// Enable the PWM0_0 signal to be passed to the PB6 pin (M0PWM0)
	// by setting the PWM0EN bit (Bit 0) in the PWMENABLE register
	PWM0->ENABLE |= 0x01;
}

void PWM0_0_Update_Duty_Cycle(uint16_t duty_cycle)
{
	// Set the duty cycle by writing to the COMPA field (Bits 15 to 0)
	// in the PWM0CMPA register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_0_CMPA = (duty_cycle - 1);
}
