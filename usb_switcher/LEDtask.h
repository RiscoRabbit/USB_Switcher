#ifndef LEDTASK_H
#define LEDTASK_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.h"

// LED state enumeration
typedef enum {
    Idle_state,
    Alternative_idle_state,
    Sleeping_state,
    Active_state,
    Alternative_state
} LEDstate;

// Function prototypes
void task_led_function(void *pvParameters);
void set_pixel_color(uint32_t color);
void setLEDStateActive();
LEDstate getLEDState(); // Add function to get current LED state

#endif // LEDTASK_H