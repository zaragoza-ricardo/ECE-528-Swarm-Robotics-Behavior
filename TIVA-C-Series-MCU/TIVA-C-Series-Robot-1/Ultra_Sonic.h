/**
 * @file Ultra_Sonic.h
 *
 * @brief Source file for the Ultra Soncic Sensor driver.
 *
 * @note This driver assumes that the system clock's frequency is 50 MHz.
 *
 * @note 
 *
 * @author Ricardo Zaragoza
 */
#ifndef ULTRA_SONIC_H_
#define ULTRA_SONIC_H_

#include <stdint.h>
//Initilizae ultra sonic sensor
void Ultrasonic_Init(void);

// read pulse
uint32_t Ultrasonic_ReadPulse(void);

// return pulse width in CM
uint32_t Ultrasonic_ReadDistanceCM(void);
#endif