#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "ws2812.pio.h"

// WS2812 LED configuration
#define WS2812_PIN 16        // GPIO pin connected to WS2812 data line
#define NUM_PIXELS 1         // Number of WS2812 LEDs in the chain
#define IS_RGBW false        // Set to true if using RGBW LEDs

// Color definitions (GRB format for WS2812)
#define COLOR_RED     0x000300
#define COLOR_GREEN   0x010000  
#define COLOR_BLUE    0x000004
#define COLOR_WHITE   0x010202
#define COLOR_OFF     0x000000
#define COLOR_YELLOW  0x010300
#define COLOR_CYAN    0x010003
#define COLOR_MAGENTA 0x000203

void ws2812_init();
