#include "USBtask.h"
#include "USBHostTask.h"
#include "USBDeviceTask.h"
#include "fstask.h"
#include "CDCCmd.h"
#include "LuaTask.h"
#include "UARTtask.h"
#include "configRead.h"
#include <stdlib.h>
#include <string.h>
#include "LEDtask.h"

// Version information
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

// Global variables
uint32_t device_usb_status = USB_NOT_MOUNTED;
hid_vibration_report_t vibration_state;
uint8_t const keycode2ascii[128][2] = { HID_KEYCODE_TO_ASCII };

uint8_t metaKeyCode = 0x39; // Metaキーの状態を保持する変数

// Meta mode state (needs to be accessible from USBHostTask.c)
bool meta = false;

// Local static variables - changed to global for USBDeviceTask.c access
bool has_gamepad_key = false;

// HID Protocol state for each instance (0=Boot, 1=Report) - changed to global for USBDeviceTask.c access
uint8_t hid_protocol[3] = {1, 1, 1}; // Default to Report mode for all instances

uint8_t default_hid_protocol = USE_DEVICE_PROTOCOL;
int device_id = 0; // Default device ID

/**
 * Function to display current protocol settings
 */
void show_protocol_settings(void) {
    printf("Current HID Protocol settings:\n");
    for (int i = 0; i < 3; i++) {
        printf("  Instance %d: %s mode\n", i, hid_protocol[i] == 0 ? "BOOT" : "REPORT");
    }
}

// Task 1 - Core1 function
void task1_function(void)
{
    printf("Task 1 started on Core1\n");
    // PIO USB is now initialized in main() before FreeRTOS starts
    multicore_lockout_victim_init();

    while (1)
    {
        tud_task(); // tinyusb device task
        hid_task();

        tuh_task();

        usb_host_task(); // Process buffered keyboard reports

        uart_task(); // UART task for receiving mouse data from another Pico

    }
}

void setMetaKeyCode(uint8_t code) {
    metaKeyCode = code;
}