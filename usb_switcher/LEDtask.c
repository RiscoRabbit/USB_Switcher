#include "LEDtask.h"
#include "OLEDtask.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "LuaTask.h"

LEDstate state = Idle_state;
static TickType_t last_active_time = 0;
static const TickType_t INACTIVE_TIMEOUT_MS = 60*60*1000; // 600 seconds

void setLEDStateActive()
{
    if(USB_output_switch == 0)
    {
        state = Active_state;
        last_active_time = xTaskGetTickCount(); // Record the current time
        setOLEDStateActive();
    }
    else
    {
        state = Alternative_state;
        last_active_time = xTaskGetTickCount(); // Record the current time
        setOLEDStateActive();
    }
}   

LEDstate getLEDState()
{
    return state;
}

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio1, 0, pixel_grb << 8u);  // Use PIO1 to avoid conflict with PIO USB
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t) (r) << 8) |
           ((uint32_t) (g) << 16) |
           (uint32_t) (b);
}

// Set a single pixel color
void set_pixel_color(uint32_t color) {
    static uint32_t last_color = COLOR_OFF;
    if(last_color == color) {
        return;
    }
    last_color = color;
    put_pixel(color);
}

// Task 3 - LED
void task_led_function(void *pvParameters)
{
    (void)pvParameters; // Avoid unused parameter warning
    printf("Task LED started\n");
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait 0.5 seconds
    set_pixel_color(COLOR_OFF);
    vTaskDelay(pdMS_TO_TICKS(500)); // Wait 0.5 seconds
    
    // Initialize last_active_time
    last_active_time = xTaskGetTickCount();

    while (1)
    {
        TickType_t current_time = xTaskGetTickCount();
        TickType_t time_since_last_active = current_time - last_active_time;
        
        // Check if 10 seconds have passed since last activity
        if (time_since_last_active > pdMS_TO_TICKS(INACTIVE_TIMEOUT_MS))
        {
            // Timeout: Turn off LED and set sleeping state
            if (state != Sleeping_state) {
                printf("LED Timeout: Entering sleep state at tick %lu (inactive for %lu ms)\n", 
                       (unsigned long)current_time, 
                       (unsigned long)(time_since_last_active * portTICK_PERIOD_MS));
                state = Sleeping_state;
            }
            set_pixel_color(COLOR_OFF);
        }
        else
        {
            switch (state) {
                case Idle_state:
                    set_pixel_color(COLOR_GREEN);
                    break;
                case Alternative_idle_state:
                    set_pixel_color(COLOR_YELLOW);
                    break;
                case Sleeping_state:
                    set_pixel_color(COLOR_BLUE);
                    break;
                case Active_state:
                    set_pixel_color(COLOR_OFF);
                    vTaskDelay(pdMS_TO_TICKS(30)); // Off for 0.03 seconds
                    set_pixel_color(COLOR_GREEN);
                    vTaskDelay(pdMS_TO_TICKS(40)); // Active for 0.04 seconds
                    state = Idle_state; // Return to Idle state
                    break;
                case Alternative_state:
                    set_pixel_color(COLOR_OFF);
                    vTaskDelay(pdMS_TO_TICKS(30)); // Off for 0.03 seconds
                    set_pixel_color(COLOR_YELLOW);
                    vTaskDelay(pdMS_TO_TICKS(40)); // Active for 0.04 seconds
                    state = Alternative_idle_state; // Return to Idle state
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms delay when active
    }
}