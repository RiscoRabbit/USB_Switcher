/**
 * OLED Text Display Example
 * 
 * This file demonstrates how to use the new OLED text display functions.
 * Include this in your application code to display text on the OLED at specific coordinates.
 */

#include "OLEDtask.h"

/**
 * @brief Example function showing how to use the OLED text display functions
 * 
 * This function demonstrates various ways to display text on the OLED:
 * - Display text at specific coordinates
 * - Display centered text
 * - Clear and update the display
 */
void oled_text_display_example(void) {
    // Only proceed if OLED is present and initialized
    if (!isOLEDPresent()) {
        printf("OLED not available for text display example\n");
        return;
    }
    
    // Example 1: Clear the display
    oled_clear_display();
    
    // Example 2: Display text at specific coordinates
    oled_display_text(0, 0, 1, "Hello World!");     // Top-left, normal size
    oled_display_text(0, 8, 1, "Line 2");           // Second line, normal size
    oled_display_text(0, 16, 2, "BIG");             // Third line, 2x size
    
    // Update the display to show all the text
    oled_update_display();
    
    // Wait 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Example 3: Clear and display centered text
    oled_clear_display();
    oled_display_text_centered(4, 1, "Centered Text");
    oled_display_text_centered(12, 1, "Another Line");
    oled_display_text_centered(20, 2, "BIG");
    oled_update_display();
    
    // Wait 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Example 4: Display coordinates information
    oled_clear_display();
    oled_display_text(0, 0, 1, "X:0 Y:0");
    oled_display_text(64, 0, 1, "X:64");
    oled_display_text(0, 24, 1, "Y:24");
    oled_display_text(64, 24, 1, "Corner");
    oled_update_display();
    
    // Wait 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Example 5: Display a counter with coordinates
    for (int i = 0; i < 10; i++) {
        oled_clear_display();
        
        char counter_str[32];
        snprintf(counter_str, sizeof(counter_str), "Count: %d", i);
        oled_display_text_centered(8, 2, counter_str);
        
        char position_str[32];
        snprintf(position_str, sizeof(position_str), "Pos: %d,%d", 10, 20);
        oled_display_text(10, 20, 1, position_str);
        
        oled_update_display();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    // Clear display at the end
    oled_clear_display();
    oled_update_display();
}

/**
 * @brief Simple function to display a message at specified coordinates
 * @param x X coordinate
 * @param y Y coordinate
 * @param message Message to display
 */
void display_message_at(uint32_t x, uint32_t y, const char* message) {
    if (!isOLEDPresent()) {
        return;
    }
    
    oled_clear_display();
    oled_display_text(x, y, 1, message);
    oled_update_display();
}

/**
 * @brief Simple function to display a centered message
 * @param y Y coordinate
 * @param message Message to display
 */
void display_centered_message(uint32_t y, const char* message) {
    if (!isOLEDPresent()) {
        return;
    }
    
    oled_clear_display();
    oled_display_text_centered(y, 1, message);
    oled_update_display();
}