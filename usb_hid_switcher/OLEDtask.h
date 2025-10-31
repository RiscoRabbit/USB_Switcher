#ifndef OLEDTASK_H
#define OLEDTASK_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "LEDtask.h"  // Include LEDtask to access LED state

// OLED state management
void setOLEDStateActive(void);
void setOLEDOff(void);
bool isOLEDActive(void);
bool isOLEDPresent(void);

// Function prototypes
void task_oled_function(void *pvParameters);

// OLED text display functions
void oled_display_text(uint32_t x, uint32_t y, uint32_t scale, const char *text);
void oled_display_text_centered(uint32_t y, uint32_t scale, const char *text);
void oled_clear_display(void);
void oled_update_display(void);

#endif // OLEDTASK_H