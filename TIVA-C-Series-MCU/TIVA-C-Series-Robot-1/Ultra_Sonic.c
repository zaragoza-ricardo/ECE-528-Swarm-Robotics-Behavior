/**
 * @file Ultra_Sonic.c
 *
 * @brief Source file for the Ultra_Sonic.
 *
 
 * @author Ricardo Zaragoza
 */




#include "TM4C123GH6PM.h"  
#include "SysTick_Delay.h"   // SysTick delay functions

#include "Ultra_Sonic.h"


// Initialize PC4 (Trigger) and PC5 (Echo)
void Ultrasonic_Init(void) 
	{
    SYSCTL->RCGCGPIO |= 0x04;            // Enable clock to Port C using GPIO
    while(!(SYSCTL->PRGPIO & 0x04));     // Wait until ready

    GPIOC->DIR |= 0x10;      // PC4 output
    GPIOC->DIR &= ~0x20;     // PC5 input  

    GPIOC->DEN |= 0x30;      // digital enable PC4 + PC5
    GPIOC->PDR |= 0x20;      // weak pull-down on PC5 (Echo)
    GPIOC->AFSEL &= ~0x30;   // GPIO function for PC4 & PC5
	}

// Send trigger pulse, wait for echo, measure the high pulse
// and return the pulse duratio read in approximate microseconds
uint32_t Ultrasonic_ReadPulse(void) 
	{
    uint32_t count = 0;					// hold measured pulse
    uint32_t timeout = 30000;   // ~30ms max wait, timeout counter

    GPIOC->DATA |= 0x10;        // Trigger HIGH by drivig PC4 high starting trigger pulse
    SysTick_Delay1us(10);				// Keep trigger pin high for 10 microseconds
    GPIOC->DATA &= ~0x10;       // Trigger LOW, drive PC4 low, ending trigger pulse

    // Wait until Echo PC5 goes HIGH.
    while(!(GPIOC->DATA & 0x20)) 
    {
        if(timeout == 0) return 0;  // No echo
        timeout--;
    }

    // Measure pulse width
    count = 0;   // reset counter before timing pulse width
    while(GPIOC->DATA & 0x20) 
    {
				if(timeout == 0) return 0;  // No echo
        timeout--;
        count++;
        SysTick_Delay1us(1);
    }

    return count;
}

    
// Convert pulse width to distance in cm
uint32_t Ultrasonic_ReadDistanceCM(void) 
	{
    uint32_t pulse = Ultrasonic_ReadPulse();
    return pulse / 58;  // approximate distance in cm datasheet page 3
		
		
}
	
	
