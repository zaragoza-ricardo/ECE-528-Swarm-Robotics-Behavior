/**
 * @file Ultra_Sonic.c
 *
 * @brief Source file for the Ultra_Sonic.
 *

 * @author Ricardo Zaragoza
 */




#include "../inc/Ultra_Sonic.h"
#include "msp.h"
#include "../inc/Clock.h"


//#include "SysTick_Delay.h"   // SysTick delay functions



// Initialize P8.2 (Trigger) and P8.3 (Echo)
void Ultra_Sonic_Init(void)
    {
    // configure P8.2 and P8.3 as GPIO by clearing  SEL1 and SEL0 register for Port 8
    P8->SEL0 &= ~0x0C;         // 0000_1100b = 0x0C;
    P8->SEL1 &= ~0x0C;         // 0000_1100b = 0x0C;

    // Configure P8.2 as output GPIO by setting the Direction register for pin P8.2 (Bit 2).
    P8->DIR |= 0x04;   // 0000_0100b

    // Configure P8.3 is input GPIO by clearing the Direction register for pin P8.3 (Bit 3).
    P8->DIR &= ~0x08;  // 0000_1000b

    // Resistor enable register for P8.3
    P8->REN |= 0x08;

    // When a pin is configured as input and REN is enabled. OUT chooses whether the internal
    // resistor is Pull-Up = bit 1, and Pull-Down = bit 0
    P8->OUT &= ~0x08;       //pull-down resistor

    // Initialize the trigger output low
    P8->OUT &= ~0x04;
    }


// Send trigger pulse, wait for echo, measure the high pulse
// and return the pulse duration in approximate microseconds
uint32_t Ultrasonic_ReadPulse(void)
{
    uint32_t count = 0;
    uint32_t timeout = 30000;

    // Ensure trigger starts low
    P8->OUT &= ~0x04;
    Clock_Delay1us(2);

    // Trigger HIGH on P8.2
    P8->OUT |= 0x04;
    Clock_Delay1us(10);
    P8->OUT &= ~0x04;

    // Wait for Echo HIGH on P8.3
    while (!(P8->IN & 0x08))
    {
        if (timeout == 0) return 0;
        timeout--;
        Clock_Delay1us(1);
    }

    // Measure Echo HIGH pulse width
    timeout = 30000;
    count = 0;
    while (P8->IN & 0x08)
    {
        if (timeout == 0) return 0;
        timeout--;
        count++;
        Clock_Delay1us(1);
    }

    return count;
}

// Convert pulse width to distance in cm
uint32_t Ultrasonic_ReadDistanceCM(void)
    {
    uint32_t pulse = Ultrasonic_ReadPulse();
    return pulse / 58;  // approximate distance in cm datasheet page 3


}


