/**
 * @file main.c
 *
 * @brief Main source code for the Autonomous Vehicle with manual override.
 *
 * This file contains the main entry point and function definitions for the Autonomous Vehicle  program for Robot 1.
 * It interfaces with the  LEDs, servo motor, DC motor, ESP32, Motor driver, and ultra sonic sensor for Robot 1.
 * 
 *	RC crawler(Robot 1) chassis and peripherals components
 *	https://www.axialadventure.com/product/1-24-scx24-1967-chevrolet-c10-4wd-truck-rtr/AXI00001V2.html
 *
 * To verify the pinout of the user LED, refer to the Tiva C Series TM4C123G LaunchPad User's Guide
 * Link: https://www.ti.com/lit/pdf/spmu296
 *
 * @author Ricardo Zaragoza
 */ 



#include <stdio.h>
#include <stdint.h>
#include <math.h>
 
#include "TM4C123GH6PM.h"
#include "SysTick_Delay.h"
#include "PWM_Clock.h"
#include "GPIO.h"
#include "PWM0_0.h"
#include "PWM0_4_5.h"
#include "UART0.h"
#include "UART3.h"
#include "Ultra_Sonic.h"
#include "Swarm.h"

#include <string.h>

// Initialize constant ID for Robot 1 with TIVA for RC crawler
#define MY_ID 1

// Max number of robots in SWARM
#define MAX_PEERS 4

// Peer robot state table
// static peer_state_t g_peer_table[MAX_PEERS] = {0};

// Current autonomous state
static auto_state_t g_auto_state = AUTO_IDLE;

// State start time
static uint32_t g_state_time_ms = 0;

// Last manual command time
static uint32_t g_last_manual_rx_ms = 0;


// Ultrasonic distance reading in cm
static uint32_t g_distance_cm = 0;


// Mode selection: manual or autonomous
typedef enum
{
    MODE_MANUAL = 0,
    MODE_AUTONOMOUS = 1
} control_mode_t;


// This is the robot’s current operating mode(manual).
static control_mode_t g_control_mode = MODE_MANUAL;


//******************* 50Hz Servo MOTOR - Period Constant ************************

// Initialize the Module 0 PWM Generator 0 Block (PWM0_0) to generate a PWM signal
// using the PB6 pin that has a frequency of 50 Hz with 50% duty cycle
// Period constant = ((50 MHz / 16) / 50 Hz) = 62500
// Duty cycle = (62500 * 0.50) = 31250
// 46875 is 75% duty cycle @ 50Hz
// 55000 is 88% duty cycle @ 50Hz

//Period Constant and PWM duty cycle for Servo Motor

#define PWM_Servo_Period					62500   //period constant ~2.0ms
#define PWM_Servo_Center          5050    //~1.5ms		middle												
#define PWM_Servo_Full_Right      6250    //~2.5ms      full right
#define PWM_Servo_Full_Left       3125    //~0.48ms    full left


//********** Period Constant and PWMs duty cycle  for DC motor and H-Bridge ********

#define MOTOR_PWM_PERIOD   390     // 8KHz                 <-------- uncomment to use
//#define MOTOR_PWM_PERIOD   195   // 16KHz     					 <-------- uncomment to use
//#define MOTOR_PWM_PERIOD   156   // 20KHz               <-------- uncomment to use also adjust PWM0_0_Init(Period constant, duty cycle)
#define MOTOR_MAX_TICKS   ((MOTOR_PWM_PERIOD * 90) / 100)
#define MOTOR_MIN_TICKS   1


// ---------- UART line buffer ----------
#define LINE_BUF_SIZE 128
	static char lineBuf[LINE_BUF_SIZE];
	static int  lineIndex = 0;




// mapt steering
static uint16_t mapSteerToDuty(float lx_norm)
{
    if (lx_norm >  1.0f) lx_norm =  1.0f;
    if (lx_norm < -1.0f) lx_norm = -1.0f;

    const float deadzone = 0.15f;

		if (fabsf(lx_norm) < deadzone) lx_norm = 0.0f;

    float dutyF;

    if (lx_norm >= 0.0f)
    {
        dutyF = (float)PWM_Servo_Center +
                lx_norm * (float)(PWM_Servo_Full_Right - PWM_Servo_Center);
    }
    else
    {
        dutyF = (float)PWM_Servo_Center +
                lx_norm * (float)(PWM_Servo_Center - PWM_Servo_Full_Left);
    }

    int duty = (int)(dutyF + 0.5f);

    if (duty < (int)PWM_Servo_Full_Left) duty = PWM_Servo_Full_Left;
    if (duty > (int)PWM_Servo_Full_Right) duty = PWM_Servo_Full_Right;

    return (uint16_t)duty;
}


// Parse and process UART control commands
static uint8_t handleLine(char* s)
{
    char mode[16];
    int robot_id, lx, ly, brake, throttle;
    int ok = 0;

    if (strncmp(s, "MANUAL,", 7) == 0)
    {
        ok = sscanf(s, "MANUAL,%d,%d,%d,%d,%d",
                    &robot_id, &lx, &ly, &brake, &throttle);
        strcpy(mode, "MANUAL");
    }
    else if (strncmp(s, "AUTO,", 5) == 0)
    {
        ok = sscanf(s, "AUTO,%d,%d,%d,%d,%d",
                    &robot_id, &lx, &ly, &brake, &throttle);
        strcpy(mode, "AUTO");
    }
    else
    {
        UART0_Output_String("Bad prefix\r\n");
        return 0;
    }

    UART0_Output_String("RAW: ");
    UART0_Output_String(s);
    UART0_Output_String("\r\n");

    UART0_Output_String("A\r\n");

    UART0_Output_String("ok=");
    UART0_Output_Unsigned_Decimal((uint32_t)ok);
    UART0_Output_Newline();

    if (ok != 5)
    {
        UART0_Output_String("Parse failed\r\n");
        return 0;
    }

    UART0_Output_String("C\r\n");

    if (robot_id != 0 && robot_id != MY_ID)
    {
        return 0;
    }

    float lx_norm = (float)lx / 512.0f;
    lx_norm = -lx_norm;

    uint16_t steerDuty = mapSteerToDuty(lx_norm);
    PWM0_0_Update_Duty_Cycle(steerDuty);

    UART0_Output_String("D\r\n");

    if (brake < 0) brake = 0;
    if (brake > 1023) brake = 1023;
    if (throttle < 0) throttle = 0;
    if (throttle > 1023) throttle = 1023;

    const uint16_t trigger_deadzone = 30;

    if (throttle <= trigger_deadzone && brake <= trigger_deadzone)
    {
        RGB_LED_Output(RGB_LED_OFF);
        PWM_Motor_Stop();
    }
    else if (throttle > brake)
    {
        int duty = MOTOR_MIN_TICKS +
                   (int)(((uint32_t)throttle * (MOTOR_MAX_TICKS - MOTOR_MIN_TICKS)) / 1023u);
        if (duty < 1) duty = 1;
        PWM_Motor_Forward(duty);
        RGB_LED_Output(RGB_LED_RED);
    }
    else if (brake > throttle)
    {
        int duty = MOTOR_MIN_TICKS +
                   (int)(((uint32_t)brake * (MOTOR_MAX_TICKS - MOTOR_MIN_TICKS)) / 1023u);
        if (duty < 1) duty = 1;
        PWM_Motor_Backward(duty);
        RGB_LED_Output(RGB_LED_RED);
    }
    else
    {
        RGB_LED_Output(RGB_LED_OFF);
        PWM_Motor_Stop();
    }

    UART0_Output_String("E\r\n");
    return 1;
}




// function protypes
void Run_Autonomous_StateMachine(void);
void Update_Local_Sensors(void);




int main(void)
 {
	//Initialize the SysTick timer used to provide blocking delay functions
	SysTick_Delay_Init();
	
	//Initialize LED lights for GPIO
	RGB_LED_Init();
	
	// Initialize the PWM clock to use the PWM clock divisor as the source
	// and update the PWM clock frequency to 50 MHz / 16 = 3.125 MHz
	PWM_Clock_Init();
		
	// Initialize the Module 0 PWM Generator 0 Block (PWM0_0) to generate a PWM signal
	// using the PB6 pin that has a frequency of 50 Hz with 50% duty cycle
	// Period constant = ((50 MHz / 16) / 50 Hz) = 62500
	// Duty cycle = (62500 * 0.50) = 31250
	PWM0_0_Init(62500, PWM_Servo_Center);  
	 
	//-------------------------- 20kHz  -------
	//Initialize the Module 0 PWM Generator 2 Block (PWM0_4) and (PWM0_5) to generate a PWM signal
	//using the PE4 pin and PE5 pin that has a frequency of 20KHz with 0% duty cycle
	 //
	// Period constant = ((50 MHz / 16) / 20000 Hz) = 156.25
	// Duty cycle = (156.25 * 0.50) = 78   //50% duty cycle
	
	//PWM0_4_5_Init(156, 1, 1 );  //at 20kHz with 0% duty cycle  <----------ENABLE 20kHz
	//PWM0_4_5_Init(62500, 1, 1 ); // at 50Hz with 0% duty cycle


	//------------------------ 1kHz - 4kHz	-------- 
	//High "Kick"; best for heavy crawling and starting on hills.
	
	//Initialize the Module 0 PWM Generator 2 Block (PWM0_4) and (PWM0_5) to generate a PWM signal
	//using the PE4 pin and PE5 pin that has a frequency of 1KHz with 0% duty cycle
	
	// Period constant = ((50 MHz / 16) / 1000 Hz) = 3125
	// Duty cycle = (3125 * 0.50) = 1562   // 50% duty cycle
	// Dut cycle = (3125 * 0.90) =  2812   // 90% duty cycle
	//PWM0_4_5_Init(3125, 1, 1 );  //at 1kHz with 0% duty cycle   <----------ENABLE  1kHz


	//---------------------- 8kHz - 16kHz ------- 
	//Balanced; good mix of smoothness and starting power.
	
	//Initialize the Module 0 PWM Generator 2 Block (PWM0_4) and (PWM0_5) to generate a PWM signal
	//using the PE4 pin and PE5 pin that has a frequency of 8KHz with 0% duty cycle
	
	// Period constant = ((50 MHz / 16) / 8000 Hz) = 390
	// Duty cycle = (3125 * 0.50) = 195   // 50% duty cycle
	// Duty cycle = (3125 * 0.90) = 351   // 90% duty cycle
	
	 PWM0_4_5_Init(390, 1,1);  //at 8KHz @ Zero percent(%) duty cycle              <----------Initialize with 8KHz PWM signal
  
	
	//PWM0_4_5_Init(195, 1,1);  //at 16KHz @ Zero percent(%) duty cycle            <----------Initialize with 16kHz PWM signal

  // 1KHz 2812 is 90% duty cycle
	//PWM0_4_5_Init(2812, 1,1);  //at 1KHz @ Zero percent(%) duty cycle            <----------Initialize with 1KHz PWM signal
	
	//Initilize UART0 for debuggin
	UART0_Init();
	
	//Initilizae UART3 for ESP32 to TIVA serial communiaction
	UART3_Init();
	
	// Initialize Ultra sonic sensor
	Ultrasonic_Init();
	

// get current time
g_last_manual_rx_ms = SysTick_GetMillis();


// Polling method for Robot 1: continuously read incoming UART data
while (1)
{
    while (UART3_DataAvailable())
    {
        char c = UART3_Input_Character();

        if (c == '\r')
        {
        }
        else if (c == '\n')
        {
            if (lineIndex > 0)
            {
                lineBuf[lineIndex] = '\0';

                if (handleLine(lineBuf))
                {
                    g_last_manual_rx_ms = SysTick_GetMillis();

                    if (g_control_mode == MODE_AUTONOMOUS)
                    {
                        g_control_mode = MODE_MANUAL;
                        g_auto_state = AUTO_IDLE;
                        PWM0_0_Update_Duty_Cycle(PWM_Servo_Center);
                        UART0_Output_String("Switching to MANUAL\r\n");
                    }
                }
            }

            lineIndex = 0;
        }
        else
        {
            if (lineIndex < (LINE_BUF_SIZE - 1))
            {
                lineBuf[lineIndex++] = c;
            }
            else
            {
                lineIndex = 0;
            }
        }
    }

    if (g_control_mode == MODE_MANUAL)
    {
        if ((SysTick_GetMillis() - g_last_manual_rx_ms) >= 10000)
        {
            UART0_Output_String("Switching to AUTO\r\n");
            g_control_mode = MODE_AUTONOMOUS;
            g_auto_state = AUTO_IDLE;
            g_state_time_ms = SysTick_GetMillis();
            lineIndex = 0;
        }
    }
    else
    {
        if (!UART3_DataAvailable())
        {
            Update_Local_Sensors();
            Run_Autonomous_StateMachine();
            SysTick_Delay1ms(1);
        }
    }
}
}

  
// Runs the finite state machine for autonomous mode
void Run_Autonomous_StateMachine(void)
{	
		//store previous sate to detect changes
		static auto_state_t old_state = AUTO_IDLE;
		
		// calcuate elapsed time since entering current state
    uint32_t elapsed = SysTick_GetMillis() - g_state_time_ms;

	
		//print debug when state changes
	    if (old_state != g_auto_state)
    {
        char msg[60];
        snprintf(msg, sizeof(msg), "STATE=%d DIST=%u\r\n", g_auto_state, g_distance_cm);
        UART0_Output_String(msg);
        old_state = g_auto_state;
    }
		
		//update state base on sensor ditance readings
    switch (g_auto_state)
    {
        case AUTO_IDLE:
            PWM_Motor_Stop();

            if (g_distance_cm > 15)
            {
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_GetMillis();
            }
            else if (g_distance_cm > 0 && g_distance_cm <= 4)
            {
                g_auto_state = AUTO_REVERSE;
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_FORWARD:
            if (g_distance_cm == 0)
            {
                PWM_Motor_Stop();
                g_auto_state = AUTO_STOP;
                g_state_time_ms = SysTick_GetMillis();
            }
            else if (g_distance_cm <= 4)
            {
                PWM_Motor_Stop();
                g_auto_state = AUTO_REVERSE;
                g_state_time_ms = SysTick_GetMillis();
            }
            else if (g_distance_cm <= 15)
            {
                PWM_Motor_Forward(150);
            }
            else
            {
                PWM_Motor_Forward(300);
            }
            break;

        case AUTO_STOP:
            PWM_Motor_Stop();

            if (elapsed >= 300)
            {
                g_auto_state = AUTO_REVERSE;
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_REVERSE:
            PWM_Motor_Backward(150);

            if (elapsed >= 600)
            {
                PWM_Motor_Stop();
                g_auto_state = AUTO_CHECK_LEFT;
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_CHECK_LEFT:
            PWM0_0_Update_Duty_Cycle(PWM_Servo_Full_Left);
            PWM_Motor_Stop();

            if (elapsed >= 500)
            {
                if (g_distance_cm > 15)
                {
                    g_auto_state = AUTO_TURN_LEFT;
                }
                else
                {
                    g_auto_state = AUTO_CHECK_RIGHT;
                }
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_CHECK_RIGHT:
            PWM0_0_Update_Duty_Cycle(PWM_Servo_Full_Right);
            PWM_Motor_Stop();

            if (elapsed >= 700)
            {
                if (g_distance_cm > 15)
                {
                    g_auto_state = AUTO_TURN_RIGHT;
                }
                else
                {
                    g_auto_state = AUTO_REVERSE;
                }
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_TURN_LEFT:
            PWM0_0_Update_Duty_Cycle(PWM_Servo_Full_Left);
            PWM_Motor_Forward(150);

            if (elapsed >= 700)
            {
                PWM0_0_Update_Duty_Cycle(PWM_Servo_Center);
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        case AUTO_TURN_RIGHT:
            PWM0_0_Update_Duty_Cycle(PWM_Servo_Full_Right);
            PWM_Motor_Forward(150);

            if (elapsed >= 700)
            {
                PWM0_0_Update_Duty_Cycle(PWM_Servo_Center);
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_GetMillis();
            }
            break;

        default:
            PWM_Motor_Stop();
            PWM0_0_Update_Duty_Cycle(PWM_Servo_Center);
            g_auto_state = AUTO_IDLE;
            g_state_time_ms = SysTick_GetMillis();
            break;
    }
}

//read sensor
void Update_Local_Sensors(void)
{
    uint32_t d = Ultrasonic_ReadDistanceCM();

    if (d == 0)
    {
        return;
    }

    if (d < 2)
    {
        return;
    }

    g_distance_cm = d;
}