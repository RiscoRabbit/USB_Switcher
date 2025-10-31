

#include "ws2812.h"

// Initialize WS2812 PIO manually
void ws2812_init() {
    PIO pio = pio1;    // Use PIO1 to avoid conflict with PIO USB
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    
    // Set the GPIO function to PIO
    pio_gpio_init(pio, WS2812_PIN);
    
    // Set pin direction to output
    pio_sm_set_consecutive_pindirs(pio, sm, WS2812_PIN, 1, true);
    
    // Configure sideset pins
    sm_config_set_sideset_pins(&c, WS2812_PIN);
    
    // Configure output shift register
    sm_config_set_out_shift(&c, false, true, IS_RGBW ? 32 : 24);
    
    // Set FIFO join to TX only
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    // Calculate and set clock divider for 800kHz (WS2812 timing)
    float div = (float)clock_get_hz(clk_sys) / (800000 * (ws2812_T1 + ws2812_T2 + ws2812_T3));
    sm_config_set_clkdiv(&c, div);
    
    // Initialize and enable the state machine
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

