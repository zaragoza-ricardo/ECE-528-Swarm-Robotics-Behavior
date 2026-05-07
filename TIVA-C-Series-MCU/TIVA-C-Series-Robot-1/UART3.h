/**
 * @file UART3.h
 *
 * @brief Header file for the UART3 driver.
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

#include "TM4C123GH6PM.h"

#define UART3_RECEIVE_FIFO_EMPTY_BIT_MASK 0x10
#define UART3_TRANSMIT_FIFO_FULL_BIT_MASK 0x20

/**
 * @brief Carriage return character
 */
#define UART3_CR   0x0D
/**
 * @brief Line feed character
 */
#define UART3_LF   0x0A
/**
 * @brief Back space character
 */
#define UART3_BS   0x08
/**
 * @brief escape character
 */
#define UART3_ESC  0x1B
/**
 * @brief space character
 */
#define UART3_SP   0x20
/**
 * @brief delete character
 */
#define UART3_DEL  0x7F

/**
 * @brief The UART3_Init function initializes the UART3 module.
 *
 * This function configures the UART3 module with the following configuration:
 *
 * - Parity: Disabled
 * - Bit Order: Least Significant Bit (LSB) first
 * - Character Length: 8 data bits
 * - Stop Bits: 1
 * - UART Clock Source: System Clock (50 MHz)
 * - Baud Rate: 9600
 *
 * @note The PC7 (TX) and PC6 (RX) pins are used for UART communication via USB.
 *
 * @return None
 */
void UART3_Init(void);

/**
 * @brief The UART3_Input_Character function reads a character from the UART data register.
 *
 * This function waits until a character is available in the UART receive buffer
 * from the serial terminal input and returns the received character as a char type.
 *
 * @param None
 *
 * @return The received character from the serial terminal as a char type.
 */
char UART3_Input_Character(void);

/**
 * @brief The UART3_Output_Character function transmits a character via UART to the serial terminal.
 *
 * This function waits until the UART transmit buffer is ready to accept
 * a new character and then writes the specified character in the transmit buffer to the serial terminal.
 *
 * @param data The character to be transmitted to the serial terminal.
 *
 * @return None
 */
void UART3_Output_Character(char data);


int8_t UART3_DataAvailable(void);