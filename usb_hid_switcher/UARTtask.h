#ifndef UARTTASK_H
#define UARTTASK_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "tusb.h"
#include "bsp/board.h"
#include "class/hid/hid.h"
#include "base64.h"
#include "MouseReportParser.h"
#include "GamepadReportParser.h"

// UART configuration
#define UART_ID uart1
#define UART_BAUD_RATE (115200*8)
#define UART_TX_PIN 4
#define UART_RX_PIN 5

// Buffer sizes
#define UART_BUFFER_SIZE 256
#define UART_MESSAGE_SIZE 16

// UART message structure for mouse data
typedef struct {
    uint8_t header[2];      // "!M"
    char base64_data[9];    // Base64 encoded mouse data (8 chars + null terminator)
} uart_mouse_message_t;

// UART message structure for keyboard data
typedef struct {
    uint8_t header[2];      // "!K"
    char base64_data[13];   // Base64 encoded keyboard data (12 chars + null terminator)
} uart_keyboard_message_t;

// UART message structure for gamepad data
typedef struct {
    uint8_t header[2];      // "!G"
    char base64_data[13];   // Base64 encoded gamepad data (12 chars + null terminator for 8 bytes of data)
} uart_gamepad_message_t;

// Function declarations
void uart_task_init(void);
void uart_task(void);
void uart_process_received_data(void);
bool uart_parse_mouse_message(const char* message, mouse_report_t* mouse_report);
bool uart_parse_keyboard_message(const char* message, hid_keyboard_report_t* keyboard_report);
bool uart_parse_gamepad_message(const char* message, parsed_gamepad_report_t* gamepad_report);

#endif // UARTTASK_H