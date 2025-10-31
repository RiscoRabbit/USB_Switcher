#include "FreeRTOS.h"
#include "task.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp/board.h"
#include "tusb.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pio_usb.h"

#include "ws2812.h"
#include "LEDtask.h"
#include "USBtask.h"
#include "USBHostTask.h"

#include "LuaTask.h"
#include "OLEDtask.h"
#include "UARTtask.h"

#include "CDCCmd.h"

//#define USBHost1_Pin_DP 9 // for RiscoRabbit ver 1.0
//#define USBHost2_Pin_DP 11 // for RiscoRabbit ver 1.0

#define USBHost1_Pin_DP 12 // for RP2350 USB A

// #define USBHost1_Pin_DP 6 // for RiscoRabbit ver 1.1
// #define USBHost2_Pin_DP 8 // for RiscoRabbit ver 1.1
// #define USBHost3_Pin_DP 10 // for RiscoRabbit ver 1.1
// #define USBHost4_Pin_DP 12 // for RiscoRabbit ver 1.1


//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+

// FreeRTOS task functions are now in their respective files

//--------------------------------------------------------------------+
// FreeRTOS Tasks
//--------------------------------------------------------------------+

// Main tasks are implemented in separate files:
// - task2_function in LuaTask.c
// - task3_function in LEDtask.c  
// - task1_function in USBtask.c

// Task - template
void task_X_function(void *pvParameters)
{
    (void)pvParameters; // Avoid unused parameter warning
    printf("Task X started\n");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1秒待機
        //printf("X\n");
    }
}

// Task - template
void task_tud_function(void *pvParameters)
{
    (void)pvParameters; // Avoid unused parameter warning
    printf("Task TintUSB Device started\n");
    while (1)
    {
        cdc_cmd_task();
        vTaskDelay(pdMS_TO_TICKS(1)); // 1秒待機
    }
}


//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void)
{
    // Set system clock to 120MHz (multiple of 12MHz required for USB)
    set_sys_clock_khz(120000, true);
    sleep_ms(10);
    
    board_init();
    
    tusb_init();
    sleep_ms(500); // tusb_init()のあと、少し待ってから開始
    stdio_init_all();
    
    // Initialize UART0 for fallback output
    uart_init(uart0, 115200);
    gpio_set_function(0, GPIO_FUNC_UART); // RX (pin 0)
    gpio_set_function(1, GPIO_FUNC_UART); // TX (pin 1)

    printf(VERSION_STRING"\n");


    // rpt040 test board pin 14
    // rp2350 test board pin 9,14

    // Configure PIO USB for host mode BEFORE starting FreeRTOS
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = USBHost1_Pin_DP;         // Data+ pin for PIO USB
    pio_cfg.pio_tx_num = 0;     // Use PIO0 for TX
    pio_cfg.sm_tx = 1;          // Use SM1 for TX (avoid SM0 conflict) 
    pio_cfg.pio_rx_num = 0;     // Use PIO0 for RX
    pio_cfg.sm_rx = 2;          // Use SM2 for RX
    pio_cfg.sm_eop = 3;         // Use SM3 for EOP detection
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
    
    // Initialize host stack for PIO USB (roothub port1)
    gpio_set_function(USBHost1_Pin_DP  , GPIO_FUNC_PIO0);  // PIOに強制的に切り替える
    gpio_set_function(USBHost1_Pin_DP+1, GPIO_FUNC_PIO0);  // PIOに強制的に切り替える
    tuh_init(1);
    printf("USB Host initialized on GPIO %d/%d\n", USBHost1_Pin_DP, USBHost1_Pin_DP+1);

    bool res;
#ifdef USBHost2_Pin_DP
    gpio_set_function(USBHost2_Pin_DP  , GPIO_FUNC_PIO0);  // PIOに強制的に切り替える
    gpio_set_function(USBHost2_Pin_DP+1, GPIO_FUNC_PIO0);
    res = pio_usb_host_add_port(USBHost2_Pin_DP, PIO_USB_PINOUT_DPDM); // Port 2
    if(res != 0)
        printf("USB Host Port 2 initialization failed on GPIO %d/%d\n", USBHost2_Pin_DP, USBHost2_Pin_DP+1);
    else
        printf("USB Host initialized on GPIO %d/%d\n", USBHost2_Pin_DP, USBHost2_Pin_DP+1);
#endif
#ifdef USBHost3_Pin_DP
    gpio_set_function(USBHost3_Pin_DP  , GPIO_FUNC_PIO0);  // PIOに強制的に切り替える
    gpio_set_function(USBHost3_Pin_DP+1, GPIO_FUNC_PIO0);
    res = pio_usb_host_add_port(USBHost3_Pin_DP, PIO_USB_PINOUT_DPDM); // Port 3
    if(res != 0)
        printf("USB Host Port 3 initialization failed on GPIO %d/%d\n", USBHost3_Pin_DP, USBHost3_Pin_DP+1);
    else
        printf("USB Host initialized on GPIO %d/%d\n", USBHost3_Pin_DP, USBHost3_Pin_DP+1);
#endif
#ifdef USBHost4_Pin_DP
    gpio_set_function(USBHost4_Pin_DP  , GPIO_FUNC_PIO0);  // PIOに強制的に切り替える
    gpio_set_function(USBHost4_Pin_DP+1, GPIO_FUNC_PIO0);
    res = pio_usb_host_add_port(USBHost4_Pin_DP, PIO_USB_PINOUT_DPDM); // Port 4
    if(res != 0)
        printf("USB Host Port 4 initialization failed on GPIO %d/%d\n", USBHost4_Pin_DP, USBHost4_Pin_DP+1);
    else
        printf("USB Host initialized on GPIO %d/%d\n", USBHost4_Pin_DP, USBHost4_Pin_DP+1);
#endif

    // Initialize WS2812 LED
    ws2812_init();
    printf("WS2812 LED initialized on GPIO %d\n", WS2812_PIN);

    // Launch Core1 first so it can be locked as a victim
    multicore_launch_core1(task1_function);
    
    // Wait a bit for Core1 to start
    sleep_ms(100);
    
    // Now lock Core1 as victim while doing file operations
    multicore_lockout_start_blocking();
    fstask_mount_and_init(); // File system read/write from core1
    // Read configuration file
    read_config_file();
    // Scan and read all device definition files
    scan_and_read_device_definitions();
    // Show the current protocol settings
    show_protocol_settings();
    multicore_lockout_end_blocking();


    BaseType_t result;

    // Create FreeRTOS tasks for Core0
    
    /* if(xTaskCreate(task_lua_function, "Task_Lua", 4096, NULL, 2, NULL) != pdPASS) {
        printf("Failed to create Task_Lua\n");
        return 0;
    } */

    if(xTaskCreate(task_led_function, "Task3_LED", 4096, NULL, 2, NULL) != pdPASS) {
        printf("Failed to create Task_LED\n");
        return 0;
    }

    if(xTaskCreate(task_oled_function, "Task_OLED", 2048, NULL, 2, NULL) != pdPASS) {
        printf("Failed to create Task_OLED\n");
        return 0;
    }
    
    if(xTaskCreate(task_tud_function, "Task_TUD", 1024, NULL, 3, NULL) != pdPASS) {
        printf("Failed to create Task_TUD\n");
        return 0;
    }

    if(xTaskCreate(task_X_function, "Task_X", 1024, NULL, 3, NULL) != pdPASS) {
        printf("Failed to create Task_X\n");
        return 0;
    }
    
    // Core1 was already launched earlier
    // Check if task creation was successful
    vTaskStartScheduler();

    // Never reached when scheduler starts
    printf("Scheduler failed to start, falling back to main loop\n");

    return 0;
}