/**
 * @file PWM0_4_5.c
 *
 * @brief Source file for the PWM0_4_5 driver.
 *
 * This file contains the function definitions for the PWM0_4 .
 * It uses the Module 0 PWM Generator 2 to generate a PWM signal using the PE4 pin.
 * M0PWM4(PE4)
 
 * This file contains the function definitions for the PWM0_5 .
 * It uses the Module 0 PWM Generator 2 to generate a PWM signal using the PE5 pin.
 * M0PWM5(PE5).
 *
 * @note This driver assumes that the system clock's frequency is 50 MHz.
 *
 * @note This driver assumes that the PWM_Clock_Init function has been called
 * before calling the PWM0_4_5_Init function.
 *
 * @author Ricardo Zaragoza
 */

#include "PWM0_4_5.h"


//--------------------------> 20KHz DC  MOTOR / H-Bridge - Period Constant <-----------------------------
//   	             --Smooth/Silent; surgical precision, but might feel "weak" at start.--
//
// Initialize the Module 0 PWM Generator 2 Block (PWM0_4) and (PWM0_5) to generate a PWM signal
// using the PE4 and PE5 pin that has a frequency of 20 Hz with 50% duty cycle
//
// Period constant = ((50 MHz / 16) / 20000 Hz) = 156.25
// Duty cycle = (156.25 * 0.50) = 78.125   //50% duty cycle of a 50MHz
// 117.18 is 75% duty cycle
// 140.63 is 90% duty cycle
//
//------------------------> 1kHz - 4kHz	DC  MOTOR / H-Bridge - Period Constant <--------------- -----------
//										--High "Kick"; best for heavy crawling and starting on hills.--
//
// Period constant = ((50 MHz / 16) / 1000 Hz) = 3125    <------1KHz
// Duty cycle = (3125 * 0.50) = 1562   //50% duty cycle
// 2812 is 90% duty cycle
//
//-----------------------> 8kHz - 16kHz DC  MOTOR / H-Bridge - Period Constant <----------------------------
//											--Balanced; good mix of smoothness and starting power.--
//
// Period constant = ((50 MHz / 16) / 8000 Hz) = 390      <-----8KHz
// Duty cycle = (3125 * 0.50) = 195   //50% duty cycle
// 351 is 90% duty cycle
//



 
void PWM0_4_5_Init(uint16_t period_constant, uint16_t duty_cycle_1, uint16_t duty_cycle_2)  //<----uncomment this to use both PWM0_4 and PWM0_5
//void PWM0_4_Init(uint16_t period_constant, uint16_t duty_cycle_1)       <----uncomment this to use both PWM0_4 ONLY
{	
	// Return from the function if the specified duty_cycle is greater than
	// or equal to the given period. The duty cycle cannot exceed 99%.
	if (duty_cycle_1 >= period_constant) return;
	if (duty_cycle_2 >= period_constant) return;                                             //<----uncomment this to use both PWM0_4 and PWM0_5

	
	// Enable the clock to PWM Module 0 by setting the
	// R0 bit (Bit 0) in the RCGCPWM register
	SYSCTL->RCGCPWM |= 0x01;
	
	// Enable the clock to GPIO Port E by setting the
	// R4 bit (Bit 4) in the RCGCGPIO register
	SYSCTL->RCGCGPIO |= 0x10; //0001_0000b
	
	
	
	// Configure the PE4 pin to use the alternate function (M0PWM4)
	// by setting Bit 4 in the AFSEL register
	GPIOE->AFSEL |= 0x10; //0001_0000b
	
	// Configure the PE5 pin to use the alternate function (M0PWM5)
	// by setting Bit 5 in the AFSEL register
	GPIOE->AFSEL |= 0x20; //0010_0000b                                    //<----uncomment this to use both PWM0_4 and PWM0_5
	
	
	
	// Clear the PMC4 field (Bits 19 to 16) in the PCTL register
	//PMC4 Port Mux Control 4
	GPIOE->PCTL &= ~(0x000F0000);
	
	// Clear the PMC5 field (Bits 23 to 20) in the PCTL register
	//PMC5 Port Mux Control 5
	GPIOE->PCTL &= ~(0x00F00000);
	
	
	
	
	// Configure the PE4 pin to operate as a Module 0 PWM4 pin (M0PWM4)
	// by writing 0x4 to the PMC4 field (Bits 19 to 16) in the PCTL register
	// The 0x4 value is derived from Table 23-5 in the TM4C123G Microcontroller Datasheet
	GPIOE->PCTL |= (0x00040000);
	
	// Configure the PE5 pin to operate as a Module 0 PWM5 pin (M0PWM5)
	// by writing 0x4 to the PMC5 field (Bits 23 to 20) in the PCTL register
	// The 0x4 value is derived from Table 23-5 in the TM4C123G Microcontroller Datasheet
	GPIOE->PCTL |= (0x00400000);                                                        //<----uncomment this to use both PWM0_4 and PWM0_5
	

	
	// Enable the digital functionality for the PE4 pin 
	// by setting Bit 4 in the DEN register
	GPIOE->DEN |= 0x10; //0001_0000b
	
	// Enable the digital functionality for the PE5 pin 
	// by setting Bit 5 in the DEN register --->0010_0000b
	GPIOE->DEN |= 0x20;  //0010_0000b                                                      //<----uncomment this to use both PWM0_4 and PWM0_5
	
	
	
	// Disable the Module 0 PWM Generator 2 block (PWM0_4) before 
	// configuration by clearing the ENABLE bit (Bit 4) in the PWM0CTL register
	// PWM Block Enable is bit 0
	PWM0->_2_CTL &= ~0x01;
	
	// Configure the counter for the PWM0 block to
	// use Count-Down mode by clearing the MODE bit (Bit 1)
	// in the PWM0CTL register. The counter will count from the load value
	// to 0, and then wrap back to the load value
	// Counter Mode is bit 1
	PWM0->_2_CTL &= ~0x02; //clear bit
	
	
	
	// Set the ACTCMPAD field (Bits 7 to 6) to 0x3 in the PWM0GENA register
	// to drive the PWM signal high when the counter matches 
	// the comparator (i.e. the value in PWM0CMPA) while counting down
	PWM0->_2_GENA |= 0xC0;  
	
	// Set the ACTLOAD field (Bits 3 to 2) to 0x2 in the PWM0GENA register
	// to drive the PWM signal low when the counter matches the value
	// in the PWM0LOAD register
	PWM0->_2_GENA |= 0x08;
	

	// Set the ACTCMPBD field (Bits 11  to 10) to 0x3 in the PWM0GENB register
	// to drive the PWM signal low when the counter matches 
	// the comparator (i.e. the value in PWM0CMPB) while counting down
	PWM0->_2_GENB |= 0xC00;    //1100_0000_0000b =====>0xC00 = 11b                                           //<----uncomment this to use both PWM0_4 and PWM0_5
	
	// Set the ACTLOAD field (Bits 3 to 2) to 0x2 in the PWM0GENB register
	// to drive the PWM signal low when the counter matches the value
  // in the PWM0LOAD register
	PWM0->_2_GENB |= 0x08; //0000_1100b  =====> 0x2 = 10b                                            //<----uncomment this to use both PWM0_4 and PWM0_5
	
	
	// Set the period by writing to the LOAD field (Bits 15 to 0) 
	// in the PWM0LOAD register. This determines the number of clock
	// cycles needed to count down to zero
	PWM0->_2_LOAD = (period_constant - 1);
	
	
	// Set the duty cycle by writing to the COMPA field (Bits 15 to 0)
	// in the PWM0CMPA register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_2_CMPA = (duty_cycle_1 - 1); //comparator A
	
	// Set the duty cycle by writing to the COMPB field (Bits 15 to 0)
	// in the PWM0CMPB register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_2_CMPB = (duty_cycle_2 - 1); //comparator B                                          //<----uncomment this to use both PWM0_4 and PWM0_5
	
	
	
	// Enable the PWM0 block after configuration by setting the
	// ENABLE bit (Bit 0\n the PWM0CTL register
	PWM0->_2_CTL |= 0x01;
	

	
	// Disable the PWM0_4 signal to be passed to the PE4 pin (M0PWM4)
	// by setting the PWM0EN bit (Bit 4) in the PWMENABLE register
	//else motor will run everytime on TIVA reset if this is SET
	PWM0->ENABLE &= ~(0x10);
	
	// Disable the PWM0_5 signal to be passed to the PE5 pin (M0PWM5)
	// by setting the PWM0EN bit (Bit 5) in the PWMENABLE register
	//else motor will run everytime on TIVA reset if this is SET
	PWM0->ENABLE &= ~(0x20);                                                                 //<----uncomment this to use both PWM0_4 and PWM0_5
}

void PWM0_4_Update_Duty_Cycle(uint16_t duty_cycle_1)
{
//	//make sure duty cycle is correct
//	 if (duty_cycle_1 < 1)
//   duty_cycle_1 = 1;
//	
	// Set the duty cycle by writing to the COMPA field (Bits 15 to 0)
	// in the PWM0CMPA register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_2_CMPA = (duty_cycle_1 - 1);
}

void PWM0_5_Update_Duty_Cycle(uint16_t duty_cycle_2)                                    // <----uncomment this to use both PWM0_4 and PWM0_5
{
//	// make sure duty cycle is correct
//	if (duty_cycle_2 < 1)
//  duty_cycle_2 = 1;
	
	// Set the duty cycle by writing to the COMPB field (Bits 15 to 0)
	// in the PWM0CMPB register. When the counter matches the value in this register,
	// the PWM signal will be driven high
	PWM0->_2_CMPB = (duty_cycle_2 - 1);
}




//0010_0000b PE5
//0001_0000b PE4

void PWM_Motor_Forward(uint16_t duty_cycle)
{ 
	//if (duty_cycle < 1) duty_cycle = 1;

	 PWM0_4_Update_Duty_Cycle(1);
	
	 // disable PWM on pin PE4
	 PWM0->ENABLE &= ~(0x10);
	
   // Load new duty
   PWM0_5_Update_Duty_Cycle(duty_cycle);

   // Re-enable PWM  on PE5     ---> 0001_0000b
   PWM0->ENABLE |= 0x20;
}

void PWM_Motor_Backward(uint16_t duty_cycle)
{
	

	
	PWM0_5_Update_Duty_Cycle(1);
	// disable PWM on pin PE5  ---> 0010_0000b
	 PWM0->ENABLE &= ~(0x20);    
	
	
	// Load new duty
  PWM0_4_Update_Duty_Cycle(duty_cycle);

  // Re-enable PWM  on PE4     ---> 0001_0000b
  PWM0->ENABLE |= 0x10;

}

void PWM_Motor_Stop(void)
{
	  // Set both duties low
    PWM0_4_Update_Duty_Cycle(1);
    PWM0_5_Update_Duty_Cycle(1);

    // Disable both PWM outputs
    PWM0->ENABLE &= ~(0x10);
    PWM0->ENABLE &= ~(0x20);
}
