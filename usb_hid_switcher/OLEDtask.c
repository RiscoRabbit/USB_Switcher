#include "OLEDtask.h"
#include <string.h>

// Global OLED display instance
static ssd1306_t oled_display;
static bool oled_initialized = false;
static bool oled_device_present = false;  // Flag to track if OLED hardware is actually connected

// OLED timeout management
static TickType_t oled_last_active_time = 0;
static const TickType_t OLED_INACTIVE_TIMEOUT_MS = 60*60*1000; // 3 seconds timeout
static bool oled_is_active = true;

// Check if OLED device is present on I2C bus
static bool check_oled_presence(uint8_t address, i2c_inst_t *i2c) {
    uint8_t dummy_data = 0x00;
    int result = i2c_write_blocking(i2c, address, &dummy_data, 1, false);
    return result != PICO_ERROR_GENERIC;  // Device present if address is acknowledged
}

// Set OLED display orientation (180 degree rotation)
static void set_oled_rotation_180(ssd1306_t *display) {
    // Commands to rotate display 180 degrees (upside down)
    // Default in ssd1306.c: SET_SEG_REMAP | 0x01 (0xA1) and SET_COM_OUT_DIR | 0x08 (0xC8)
    // For 180 degree rotation, we need opposite: 0xA0 and 0xC0
    uint8_t rotation_cmds[] = {
        0x00,  // Command mode prefix
        0xA0,  // SET_SEG_REMAP - column 0 mapped to SEG0 (reverse of default 0xA1)
        0x00,  // Command mode prefix  
        0xC0   // SET_COM_OUT_DIR - scan from COM0 to COM[N] (reverse of default 0xC8)
    };
    
    // Send rotation commands via I2C
    for (int i = 0; i < 4; i += 2) {
        uint8_t cmd_data[2] = {rotation_cmds[i], rotation_cmds[i+1]};
        i2c_write_blocking(display->i2c_i, display->address, cmd_data, 2, false);
    }
    
    printf("OLED display rotated 180 degrees (upside down)\n");
}

// Initialize OLED display
static bool init_oled(void) {
    // Initialize I2C on pins 26 (SDA) and 27 (SCL)
    i2c_init(i2c1, 400000);  // 400kHz I2C clock
    gpio_set_function(26, GPIO_FUNC_I2C);  // SDA
    gpio_set_function(27, GPIO_FUNC_I2C);  // SCL
    gpio_pull_up(26);
    gpio_pull_up(27);
    
    printf("I2C initialized on pins 26 (SDA) and 27 (SCL)\n");
    
    // Check if OLED device is present before attempting initialization
    if (!check_oled_presence(0x3C, i2c1)) {
        printf("OLED device not detected at address 0x3C - skipping initialization\n");
        oled_device_present = false;
        oled_initialized = false;
        return false;
    }
    
    printf("OLED device detected at address 0x3C\n");
    oled_device_present = true;
    
    // Initialize SSD1306 display (128x32, I2C address 0x3C)
    oled_display.external_vcc = false;
    bool success = ssd1306_init(&oled_display, 128, 32, 0x3C, i2c1);
    
    if (success) {
        printf("SSD1306 OLED initialized successfully\n");
        // Clear the display
        ssd1306_clear(&oled_display);
        ssd1306_show(&oled_display);
        
        // Set display to upside down (180 degree rotation)
        set_oled_rotation_180(&oled_display);
        
        oled_initialized = true;
    } else {
        printf("Failed to initialize SSD1306 OLED\n");
        oled_initialized = false;
        oled_device_present = false;
    }
    
    return success;
}

// OLED state management functions
void setOLEDStateActive(void) {
    if (oled_device_present) {
        oled_is_active = true;
        oled_last_active_time = xTaskGetTickCount();
    }
}

void setOLEDOff(void) {
    if (oled_device_present) {
        oled_is_active = false;
    }
}

bool isOLEDActive(void) {
    return oled_device_present && oled_is_active;
}

bool isOLEDPresent(void) {
    return oled_device_present;
}

// OLED Task - display Hello World on SSD1306
void task_oled_function(void *pvParameters)
{
    (void)pvParameters; // Avoid unused parameter warning
    
    printf("OLED Task started on Core0\n");
    
    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Initialize OLED display
    if (!init_oled()) {
        printf("OLED initialization failed, task will continue without display\n");
    }
    
    // Initialize OLED timer
    oled_last_active_time = xTaskGetTickCount();
    
    // Display "SWITCH 0" on startup
    if (oled_initialized && oled_device_present) {
        ssd1306_clear(&oled_display);
        ssd1306_draw_string(&oled_display, 8, 12, 2, "SWITCH 0");
        ssd1306_show(&oled_display);
        printf("OLED: Displaying 'SWITCH 0' on startup\n");
    }
    
    int counter = 0;
    static bool oled_display_is_on = true;
    
    while (1)
    {
        if (oled_initialized && oled_device_present) {
            TickType_t current_time = xTaskGetTickCount();
            TickType_t time_since_last_active = current_time - oled_last_active_time;
            
            // Check if 3 seconds have passed since last OLED activity
            if (time_since_last_active > pdMS_TO_TICKS(OLED_INACTIVE_TIMEOUT_MS) || !oled_is_active) {
                if (oled_display_is_on) {
                    printf("OLED Timeout: Powering off display after 3 seconds of inactivity\n");
                    ssd1306_poweroff(&oled_display);
                    oled_display_is_on = false;
                }
            } else if (oled_is_active) {
                // Turn on display if it was off and we're now active
                if (!oled_display_is_on) {
                    printf("OLED: Powering on display (activity detected)\n");
                    ssd1306_poweron(&oled_display);
                    oled_display_is_on = true;
                }
            }
        } else {
            // OLED not available - just update counter silently
            counter++;
        }
        
        // Task delay - longer delay when display is off or not present to save CPU cycles
        if (!oled_display_is_on || !oled_device_present) {
            vTaskDelay(pdMS_TO_TICKS(100)); // 100ms delay when off or not present
        } else {
            vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay when active (slower refresh)
        }
    }
}

// OLED text display functions

/**
 * @brief Display text at specified coordinates on OLED
 * @param x X coordinate (0-127 for 128 pixel width display)
 * @param y Y coordinate (0-31 for 32 pixel height display)
 * @param scale Font scaling factor (1 = normal size, 2 = 2x size, etc.)
 * @param text Text string to display
 */
void oled_display_text(uint32_t x, uint32_t y, uint32_t scale, const char *text) {
    if (!oled_initialized || !oled_device_present || !text) {
        return;
    }
    
    // Display the text at specified coordinates
    ssd1306_draw_string(&oled_display, x, y, scale, text);
    
    // Set OLED as active
    setOLEDStateActive();
}

/**
 * @brief Display text centered horizontally at specified Y coordinate
 * @param y Y coordinate (0-31 for 32 pixel height display)
 * @param scale Font scaling factor (1 = normal size, 2 = 2x size, etc.)
 * @param text Text string to display
 */
void oled_display_text_centered(uint32_t y, uint32_t scale, const char *text) {
    if (!oled_initialized || !oled_device_present || !text) {
        return;
    }
    
    // Calculate text width (approximately 6 pixels per character for default font at scale 1)
    uint32_t text_length = strlen(text);
    uint32_t char_width = 6;  // Default font character width
    uint32_t text_width = text_length * char_width * scale;
    
    // Calculate centered X position
    uint32_t display_width = oled_display.width;
    uint32_t x = (text_width < display_width) ? (display_width - text_width) / 2 : 0;
    
    // Display the text at calculated centered position
    ssd1306_draw_string(&oled_display, x, y, scale, text);
    
    // Set OLED as active
    setOLEDStateActive();
}

/**
 * @brief Clear the OLED display buffer
 */
void oled_clear_display(void) {
    if (!oled_initialized || !oled_device_present) {
        return;
    }
    
    ssd1306_clear(&oled_display);
    
    // Set OLED as active
    setOLEDStateActive();
}

/**
 * @brief Update the OLED display to show buffered content
 */
void oled_update_display(void) {
    if (!oled_initialized || !oled_device_present) {
        return;
    }
    
    ssd1306_show(&oled_display);
    
    // Set OLED as active
    setOLEDStateActive();
}