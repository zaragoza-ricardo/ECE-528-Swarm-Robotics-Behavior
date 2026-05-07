/**
 * @file main.c
 *
 * @brief Main source code for the BLE_UART program.
 *
 * This file contains the main entry point for the BLE_UART program,
 * which is used to demonstrate the BLE_UART driver.
 *
 * It interfaces with the Adafruit Bluefruit LE UART Friend Bluetooth Low Energy (BLE) module, which uses the UART communication protocol.
 *  - Product Link: https://www.adafruit.com/product/2479
 *
 * @note For more information regarding the Enhanced Universal Serial Communication Interface (eUSCI),
 * refer to the MSP432Pxx Microcontrollers Technical Reference Manual
 *
 * @author Ricardo Zaragoza
 *
 */

#include "stdint.h"
#include "math.h"
#include "msp.h"

#include "inc/Clock.h"
#include "inc/CortexM.h"
#include "inc/GPIO.h"
#include "inc/EUSCI_A0_UART.h"
#include "inc/Motor.h"
#include "inc/BLE_UART.h"
#include "inc/Print_Binary.h"
#include "inc/SysTick_Interrupt.h"

#include <string.h>  // strcmp() parse_peer needs this
#include <stdio.h>  //sscanf() parse_peeer needs this
#include <stdbool.h>  //parse_peer nned htis


#include "inc/swarm.h"
#include "inc/Ultra_Sonic.h"

// Initialize constant PWM duty cycle values for the motors
#define PWM_NOMINAL 3000

// Mode selection: manual or autonomous
typedef enum
{
    MODE_MANUAL = 0,
    MODE_AUTONOMOUS = 1
} control_mode_t;

// This is the robot’s current operating mode(manual).
static control_mode_t g_control_mode = MODE_MANUAL;
//static control_mode_t g_control_mode = MODE_AUTONOMOUS;


// definition of constant-ID for Robot with MSP432
#define MY_ID 2

#define MAX_PEERS 4

static peer_state_t g_peer_table[MAX_PEERS] = {0};

static auto_state_t g_auto_state = AUTO_IDLE;  //

static uint32_t g_state_time_ms = 0;

static uint32_t g_last_manual_rx_ms = 0;

static uint32_t g_distance_cm = 0;  //read sensor data

// Initialize a global variable for SysTick and Timer A to keep track of elapse time in millisecods.
volatile uint32_t SysTick_ms_elapsed = 0;

// Interrupt service routine.
// Every time SysTick_Handler is executed, it increments the global variabled by 1.
// If  1000 ms have ellapsed, when we want to tottle P8.0
// to use ISR, we have to use the name given by the vendor
void SysTick_Handler(void)
{
    SysTick_ms_elapsed++;
//    if (SysTick_ms_elpased >= 1000)
//    {
//        P8->OUT ^= 0x01;
//        SysTick_ms_elpase = 0; //clear the variable when value is reached.
//
//    }
}



void Process_BLE_UART_Data(char BLE_UART_Buffer[])
{
    if(Check_BLE_UART_Data(BLE_UART_Buffer, "RGB LED GREEN"))
    {
        LED2_Output(RGB_LED_GREEN);
    }
    else if (Check_BLE_UART_Data(BLE_UART_Buffer, "RGB LED OFF"))
    {
        LED2_Output(RGB_LED_OFF);
    }
    else
    {
        printf("BLE UART Command Not Found\n");
    }
}





//function prototypes

// Handle input commands
void Handle_Manual_Command(char BLE_UART_Buffer[]);

//sense enviroment
void Update_Local_Sensors(void);

// Communicate state with peers
void Broadcast_My_State(void);

// Handle autonomous mode
void Run_Autonomous_StateMachine(void);



int main(void)
{
    // Ensure that interrupts are disabled during initialization
    DisableInterrupts();

    // Initialize the 48 MHz Clock
    Clock_Init48MHz();

    // Initialize the RGB LED on the MSP432 microcontroller
    LED2_Init();

    // Initialize the EUSCI_A0_UART module
    EUSCI_A0_UART_Init_Printf();

    // Initialize the DC motors
     Motor_Init();

    // Initialize the Adafruit Bluefruit LE UART Friend module
    BLE_UART_Init();


    // Initialize a buffer that will be used to receive command string from the BLE module
    char BLE_UART_Buffer[BLE_UART_BUFFER_SIZE] = {0};

    // Itialize SysTick Interrupt with 48MHz clock cycle and priority 2
    SysTick_Interrupt_Init(SYSTICK_INT_NUM_CLK_CYCLES, SYSTICK_INT_PRIORITY);

    // Enable the interrupts used by  the modules
    EnableInterrupts();//


    // Provide a short delay after initialization and reset the BLE module
   // Clock_Delay1ms(1000);
    BLE_UART_Reset();

    // Send a message to the BLE module to check if the connection is stable
    BLE_UART_OutString("BLE UART Active \r\n");
    //Clock_Delay1ms(1000);


//     Motor_Forward(8000,8000);   // uncommne to  test motors.
     // Initialize Ultra Sonic sensor
     Ultra_Sonic_Init();

     g_last_manual_rx_ms = SysTick_ms_elapsed;

    while(1)
    {

        //TEST ULTRA SONIC SENSOR
//        uint32_t distance = Ultrasonic_ReadDistanceCM();
//        Clock_Delay1ms(60);
//        EUSCI_A0_UART_OutString("Motion Detected!\r\n");
//        EUSCI_A0_UART_OutUDec(distance);
//        EUSCI_A0_UART_OutString("cm\r\n");
//        Clock_Delay1ms(100);



//        // uncomment to test BLE 1st time connection
//        int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);
//
//        printf("BLE UART Data: ");
//
//        for (int i = 0; i < string_size; i++)
//        {
//            printf("%c", BLE_UART_Buffer[i]);
//        }
//
//        printf("\n");
//
//        Process_BLE_UART_Data(BLE_UART_Buffer);
//




// this is the working version
//        // Uncomment  this to test BLE command arrows on app and robot using Control Pad
//        if(BLE_UART_InPacket(BLE_UART_Buffer)) //if BLE_UART_InPacket() returns 1(true)
//        {
//            printf("BLE UART Data: %s\n", BLE_UART_Buffer);
//            Decode_Controller_Packet(BLE_UART_Buffer);
//        }
//




                // Read incoming UART packet from RX PIN 9.7 and also return string length
               // int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);

//                // Output to serial monitor
//                printf("BLE UART Data: ");
//                for (int i = 0; i < string_size; i++)
//                {
//                    printf("%c", BLE_UART_Buffer[i]);
//                }
//                printf("\n");

                //Process_BLE_UART_Data(BLE_UART_Buffer);                  // process commands states received from UART
                //Decode_ESP32_Packet(BLE_UART_Buffer);                   // proccess controller manual input packets





//           // Mode selection
//
//        if (g_control_mode == MODE_MANUAL)
//        {
//            printf("Before UART:\n");
//            int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);
//            printf("After UART:\n");
//
//
//            if (string_size > 0)
//            {
//                printf("BLE UART Data: ");
//                for (int i = 0; i < string_size; i++)
//                {
//                    printf("%c", BLE_UART_Buffer[i]);
//                }
//
//                    printf("\n");
//
//
//                    g_last_manual_rx_ms = SysTick_ms_elapsed;
//                    Handle_Manual_Command(BLE_UART_Buffer);
//                 }
//
//             else if (( SysTick_ms_elapsed - g_last_manual_rx_ms) >= 2000) // if not valid inputs in more tahn 2 sec
//             {
//                printf("Switching to AUTO\n");
//                g_control_mode = MODE_AUTONOMOUS;
//                g_auto_state = AUTO_IDLE;
//                g_state_time_ms = SysTick_ms_elapsed;
//              }
//        }
//        else
//        {
//           Run_Autonomous_StateMachine();
//        }



//
//        if (g_control_mode == MODE_MANUAL)
//                {
//                    if (BLE_UART_InPacket(BLE_UART_Buffer))
//                    {
//                        printf("BLE UART Data: %s\n", BLE_UART_Buffer);
//
//                        g_last_manual_rx_ms = SysTick_ms_elapsed;
//                        Handle_Manual_Command(BLE_UART_Buffer);
//                    }
//                    else if ((SysTick_ms_elapsed - g_last_manual_rx_ms) >= 2000)
//                    {
//                        printf("Switching to AUTO\n");
//
//                        g_control_mode = MODE_AUTONOMOUS;
//                        g_auto_state = AUTO_IDLE;
//                        g_state_time_ms = SysTick_ms_elapsed;
//                    }
//                }
//                else
//                {
//                    if (BLE_UART_InPacket(BLE_UART_Buffer))
//                    {
//                        printf("BLE UART Data: %s\n", BLE_UART_Buffer);
//
//                        g_last_manual_rx_ms = SysTick_ms_elapsed;
//                        Handle_Manual_Command(BLE_UART_Buffer);
//                    }
//                    else
//                    {
//                        Run_Autonomous_StateMachine();
//                    }
//                }
//


        // comment everyign and keep sthis blcok only to test timer interrupt
//        if (SysTick_ms_elapsed >= 2000)
//           {
//               printf("2 seconds reached\n");
//               g_control_mode = MODE_AUTONOMOUS;
//               Run_Autonomous_StateMachine();
//           }
//



//        // Print Tick Value
//        static uint32_t last_print = 0;
//
//            if ((SysTick_ms_elapsed - last_print) >= 500)
//            {
//                last_print = SysTick_ms_elapsed;
//                printf("tick = %lu\n", SysTick_ms_elapsed);
//            }






//                // BEST WORKING VERSION SO FAR
//
//            if (g_control_mode == MODE_MANUAL)
//            {
//                printf("Starting... in Manual Mode\n");
//
//                if (BLE_UART_DataAvailable())
//                {
//                    int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);
//
//                    if (string_size > 0)
//                    {
//                        printf("BLE UART Data: ");
//                        for (int i = 0; i < string_size; i++)
//                        {
//                            printf("%c", BLE_UART_Buffer[i]);
//                        }
//                        printf("\n");
//
//                        g_last_manual_rx_ms = SysTick_ms_elapsed;
//                        Handle_Manual_Command(BLE_UART_Buffer);
//                    }
//                }
//                else if ((SysTick_ms_elapsed - g_last_manual_rx_ms) >= 2000)
//                {
//                    printf("Switching to AUTO\n");
//                    g_control_mode = MODE_AUTONOMOUS;
//                    g_auto_state = AUTO_IDLE;
//                    g_state_time_ms = SysTick_ms_elapsed;
//                }
//            }
//            else
//            {
//                if (BLE_UART_DataAvailable())
//                    {
//                        int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);
//
//                        if (string_size > 0)
//                        {
//                            printf("Manual input detected, switching to MANUAL\n");
//                            g_last_manual_rx_ms = SysTick_ms_elapsed;
//                            Handle_Manual_Command(BLE_UART_Buffer);
//                        }
//                    }
//                else
//                {
//                    Run_Autonomous_StateMachine();
//
//
//                }
//            }
//
//
//





        if (g_control_mode == MODE_MANUAL)
        {
            //printf("Entering...Manual mode,\n");
            if (BLE_UART_DataAvailable())
            {
                int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);

                if (string_size > 0)
                {
                    g_last_manual_rx_ms = SysTick_ms_elapsed;
                    Handle_Manual_Command(BLE_UART_Buffer);
                }
            }
            else if ((SysTick_ms_elapsed - g_last_manual_rx_ms) >= 10000)
            {
                printf("Switching to AUTO\n");
                g_control_mode = MODE_AUTONOMOUS;
                g_auto_state = AUTO_IDLE;
                g_state_time_ms = SysTick_ms_elapsed;
                printf("AUTO MODE\r\n");
            }
        }
        else
        {
            //printf("Entering...Autonomous mode\n");
            if (BLE_UART_DataAvailable())
            {
                int string_size = BLE_UART_InString(BLE_UART_Buffer, BLE_UART_BUFFER_SIZE);

                if (string_size > 0)
                {
                    printf("Manual input detected, switching to MANUAL\n");
                    g_last_manual_rx_ms = SysTick_ms_elapsed;
                    Handle_Manual_Command(BLE_UART_Buffer);
                }
            }
            else
            {
                Update_Local_Sensors();
                Broadcast_My_State();
                Run_Autonomous_StateMachine();
                Clock_Delay1ms(60);
            }
        }








        }
}

void PMOD_SWT_Byte_Pattern(void)
       {
          uint8_t switch_status = Get_PMOD_SWT_Status();

          switch(switch_status)
          {
              // WT1 is pressed
              // 0001b = 0x1
              case 0x01:
              {
                  // Print binary 1010_1010 = 0xAA
                  BLE_UART_OutChar(0xAA);

              }
              break;

              // SWT 2 is pressed
              // 0010b = 0x2
              case 0x02:
              {
                  // Print binary 0001_0000 = 0x10
                  BLE_UART_OutChar(0x10);
              }
             break;

              // SWT 3 is pressed
              // 0100b = 0x4
              case 0x04:
              {
                  // Print binary 1111_0000
                  BLE_UART_OutChar(0xF0);
              }
              break;

              // SWT 4 is pressed
              // 1000b = 0x8
              case 0x08:
              {
                  // Print binary 0000_1111
                  BLE_UART_OutChar(0x0F);
              }
              break;

              default:
              {
                  //Print binary 0000_0000
                  BLE_UART_OutChar(0x00);
              }
              // one delay per cycle
              Clock_Delay1ms(1000);

          }
      }

void Decode_ESP32_Packet(char BLE_UART_Buffer[])
{
    int robot_id, lx, ly, brake, throttle;

    if (sscanf(BLE_UART_Buffer, "%d,%d,%d,%d,%d", &robot_id, &lx, &ly, &brake, &throttle) != 5)
    {
        printf("Parse failed\n");
        return;
    }


    if (robot_id != 0 && robot_id != MY_ID)  //press B on controller to active MY_ID
    {
        return;
    }

    //ADD LED PATERN TO TELL WHICH ID IS ON
    printf("lx=%d ly=%d brake=%d throttle=%d\n", lx, ly, brake, throttle);

    if (lx > 100 && lx <= 511)
    {
        Motor_Right(4500, 4500);
    }
    else if (lx < -100 && lx >= -511)
    {
        Motor_Left(4500, 4500);
    }
    else if (brake > 100 && brake <= 1023)
    {
        Motor_Backward(4500, 4500);
    }
    else if (throttle > 100 && throttle <= 1023)
    {
        Motor_Forward(4500, 4500);
    }
    else
    {
        Motor_Stop();
    }
}

// Once this fuction works correctly, replacement for Decode_ESP32_Packet()
void Handle_Manual_Command(char BLE_UART_Buffer[])
{
    char mode[16]; // string array for mode selection label. Stores up to 15 chars plus the null terminator
    int robot_id, lx, ly, brake, throttle;

    /*// "15[^,]" means read characters into mode until a comma ','
   if (sscanf(BLE_UART_Buffer, "%15[^,],%d,%d,%d,%d,%d", mode, &robot_id, &lx, &ly, &brake, &throttle) != 6)*/

    // Parse extracts as compared to sscanf
    if (!Parse_Peer_Message(BLE_UART_Buffer, mode, &robot_id, &lx, &ly, &brake, &throttle))
    {
        printf("Parse failed\n");
        return;
    }

    // Decide wether the robot is now in manual or autonomous mode
    if (strcmp(mode, "MANUAL") == 0)
    {
        g_control_mode = MODE_MANUAL;
    }
        else if (strcmp(mode, "AUTO") == 0)
        {
            g_control_mode = MODE_AUTONOMOUS;
            g_auto_state = AUTO_FORWARD;
        }
        else
        {
            printf("Unknown mode: %s\n", mode);
            return;
        }

        // Check the packet is meant for this robot
        if (robot_id != 0 && robot_id != MY_ID)
        {
            return;
        }

        // if mode is Auto, stop here
        if (g_control_mode == MODE_AUTONOMOUS)
        {
            return;
        }

        // Manual control
        printf("mode=%s lx=%d ly=%d brake=%d throttle=%d\n",mode, lx, ly, brake, throttle);

        if (lx > 100 && lx <= 511)
        {
            Motor_Right(4500, 4500);
        }
        else if (lx < -100 && lx >= -511)
        {
            Motor_Left(4500, 4500);
        }
        else if (brake > 100 && brake <= 1023)
        {
            Motor_Backward(4500, 4500);
        }
        else if (throttle > 100 && throttle <= 1023)
        {
            Motor_Forward(4500, 4500);
        }
        else
        {
            Motor_Stop();
        }
    }


//    // Autonomou mode vehicle
//void Run_Autonomous_StateMachine(void)
//{
//    uint32_t elapsed = SysTick_ms_elapsed - g_state_time_ms;
//
//    switch (g_auto_state)
//    {
//        case AUTO_IDLE:
//            Motor_Stop();
//            if (elapsed >= 1000)
//            {
//                printf("AUTO_FORWARD.\n");
//                g_auto_state = AUTO_FORWARD;
//                g_state_time_ms = SysTick_ms_elapsed;
//            }
//            break;
//
//        case AUTO_FORWARD:
//            Motor_Forward(4500, 4500);
//            if (elapsed >= 2000)
//            {
//                printf("AUTO_WAIT.\n");
//                g_auto_state = AUTO_WAIT;
//                g_state_time_ms = SysTick_ms_elapsed;
//            }
//            break;
//
//        case AUTO_WAIT:
//            Motor_Stop();
//            if (elapsed >= 1000)
//            {
//                printf("AUTO_LEFT.\n");
//                g_auto_state = AUTO_TURN_LEFT;
//                g_state_time_ms = SysTick_ms_elapsed;
//            }
//            break;
//
//        case AUTO_TURN_LEFT:
//            Motor_Left(4500, 4500);
//            if (elapsed >= 1000)
//            {
//                printf("AUTO_RIGHT.\n");
//                g_auto_state = AUTO_TURN_RIGHT;
//                g_state_time_ms = SysTick_ms_elapsed;
//            }
//            break;
//
//        case AUTO_TURN_RIGHT:
//            Motor_Right(4500, 4500);
//            if (elapsed >= 1000)
//            {
//                printf("AUTO_FORWARD.\n");
//                g_auto_state = AUTO_FORWARD;
//                g_state_time_ms = SysTick_ms_elapsed;
//            }
//            break;
//
//        default:
//            printf("Invalid auto state. Resetting to AUTO_IDLE.\n");
//            Motor_Stop();
//            g_auto_state = AUTO_IDLE;
//            g_state_time_ms = SysTick_ms_elapsed;
//            break;
//    }
//}

const char* AutoStateToString(auto_state_t state);

//void Run_Autonomous_StateMachine(void)
//{
//    uint32_t elapsed = SysTick_ms_elapsed - g_state_time_ms;
//
//   // printf("distance=%lu state=%d\n", g_distance_cm, g_auto_state);
//
//    switch (g_auto_state)
//    {
//        case AUTO_IDLE:
//            Motor_Stop();
//
//            if (g_distance_cm > 15)
//            {
//                g_auto_state = AUTO_FORWARD;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            break;
//
//        case AUTO_FORWARD:
//            if (g_distance_cm == 0)
//            {
//                Motor_Stop();
//                g_auto_state = AUTO_STOP;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            else if (g_distance_cm > 15)
//            {
//                Motor_Forward(2500, 2500);
//            }
//            else if (g_distance_cm > 4)
//            {
//                Motor_Forward(2500,2500);
//            }
//            else
//            {
//                Motor_Stop();
//                g_auto_state = AUTO_STOP;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            break;
//
//        case AUTO_STOP:
//                Motor_Stop();
//
//            if (elapsed >= 300)
//            {
//                g_auto_state = AUTO_TURN_LEFT;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            break;
//
//        case AUTO_TURN_LEFT:
//            Motor_Left(220,220);
//
//            if (elapsed >= 500)
//            {
//                Motor_Stop();
//                g_auto_state = AUTO_FORWARD;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            break;
//
//        case AUTO_TURN_RIGHT:
//            Motor_Right(220,220);
//
//            if (elapsed >= 500)
//            {
//                Motor_Stop();
//                g_auto_state = AUTO_FORWARD;
//                g_state_time_ms = SysTick_ms_elapsed;            }
//            break;
//
//        default:
//            Motor_Stop();
//            g_auto_state = AUTO_IDLE;
//            g_state_time_ms = SysTick_ms_elapsed;
//            break;
//    }
//}
//

void Run_Autonomous_StateMachine(void)
{
    uint32_t elapsed = SysTick_ms_elapsed - g_state_time_ms;

    printf("distance=%lu state=%d\n", g_distance_cm, g_auto_state);

    switch (g_auto_state)
    {
        case AUTO_IDLE:
            Motor_Stop();

            if (g_distance_cm > 15)
            {
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            else if (g_distance_cm > 0 && g_distance_cm <= 4)
            {
                g_auto_state = AUTO_REVERSE;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        case AUTO_FORWARD:
            if (g_distance_cm == 0 || g_distance_cm <= 4)
            {
                Motor_Stop();
                g_auto_state = AUTO_REVERSE;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            else if (g_distance_cm <= 15)
            {
                Motor_Forward(1800, 1800);
            }
            else
            {
                Motor_Forward(2500, 2500);
            }
            break;

        case AUTO_REVERSE:
            Motor_Backward(1800, 1800);

            if (elapsed >= 600)
            {
                Motor_Stop();
                g_auto_state = AUTO_CHECK_LEFT;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        case AUTO_CHECK_LEFT:
            Motor_Left(1800, 1800);

            if (elapsed >= 400)
            {
                Motor_Stop();
                Update_Local_Sensors();

                if (g_distance_cm > 15)
                {
                    g_auto_state = AUTO_TURN_LEFT;
                }
                else
                {
                    g_auto_state = AUTO_CHECK_RIGHT;
                }
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        case AUTO_CHECK_RIGHT:
            Motor_Right(1800, 1800);

            if (elapsed >= 800)
            {
                Motor_Stop();
                Update_Local_Sensors();

                if (g_distance_cm > 15)
                {
                    g_auto_state = AUTO_TURN_RIGHT;
                }
                else
                {
                    g_auto_state = AUTO_REVERSE;
                }
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        case AUTO_TURN_LEFT:
            Motor_Left(2200, 2200);

            if (elapsed >= 500)
            {
                Motor_Stop();
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        case AUTO_TURN_RIGHT:
            Motor_Right(2200, 2200);

            if (elapsed >= 500)
            {
                Motor_Stop();
                g_auto_state = AUTO_FORWARD;
                g_state_time_ms = SysTick_ms_elapsed;
            }
            break;

        default:
            Motor_Stop();
            g_auto_state = AUTO_IDLE;
            g_state_time_ms = SysTick_ms_elapsed;
            break;
    }
}

//function implemention
bool Parse_Peer_Message(const char *msg,
                        char *mode,
                        int *robot_id,
                        int *lx,
                        int *ly,
                        int *brake,
                        int *throttle)
{
    if (msg == 0 || mode == 0 || robot_id == 0 || lx == 0 || ly == 0 || brake == 0 || throttle == 0)
    {
        return false;
    }

    int parsed = sscanf(msg, "%15[^,],%d,%d,%d,%d,%d",
                        mode, robot_id, lx, ly, brake, throttle);

    if (parsed != 6)
    {
        return false;
    }

    if (strcmp(mode, "MANUAL") != 0 && strcmp(mode, "AUTO") != 0)
    {
        return false;
    }

    return true;
}

// function implemention
void Update_Peer_Table(int robot_id,
                       auto_state_t state,
                       int lx,
                       int ly,
                       int brake,
                       int throttle)
{
    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (g_peer_table[i].valid && g_peer_table[i].robot_id == robot_id)
        {
            g_peer_table[i].state = state;
            g_peer_table[i].lx = lx;
            g_peer_table[i].ly = ly;
            g_peer_table[i].brake = brake;
            g_peer_table[i].throttle = throttle;
            return;
        }
    }

    for (int i = 0; i < MAX_PEERS; i++)
    {
        if (!g_peer_table[i].valid)
        {
            g_peer_table[i].robot_id = robot_id;
            g_peer_table[i].state = state;
            g_peer_table[i].lx = lx;
            g_peer_table[i].ly = ly;
            g_peer_table[i].brake = brake;
            g_peer_table[i].throttle = throttle;
            g_peer_table[i].valid = 1;
            return;
        }
    }
}


// function implementation
void Broadcast_My_State(void)
{
    printf("MY STATE:" " MY_ID = %d" " g_auto_sate = %d\n", MY_ID, g_auto_state);
}


void Update_Local_Sensors(void)
{
    g_distance_cm = Ultrasonic_ReadDistanceCM();
    //printf("distance=%lu\n", g_distance_cm);

}
