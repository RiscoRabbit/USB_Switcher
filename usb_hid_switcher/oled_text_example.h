#ifndef OLED_TEXT_EXAMPLE_H
#define OLED_TEXT_EXAMPLE_H

#include "OLEDtask.h"

/**
 * @brief Example function showing how to use the OLED text display functions
 */
void oled_text_display_example(void);

/**
 * @brief Simple function to display a message at specified coordinates
 * @param x X coordinate
 * @param y Y coordinate
 * @param message Message to display
 */
void display_message_at(uint32_t x, uint32_t y, const char* message);

/**
 * @brief Simple function to display a centered message
 * @param y Y coordinate
 * @param message Message to display
 */
void display_centered_message(uint32_t y, const char* message);

#endif // OLED_TEXT_EXAMPLE_H