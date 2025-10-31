#ifndef GAMEPAD_REPORT_PARSER_H
#define GAMEPAD_REPORT_PARSER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct{
    uint16_t ReportID; // 0xffff if no ReportID
    uint16_t buttons_index; // 0xffff if not valid
    uint8_t  buttons_bitpos;
    uint8_t  buttons_size;
    uint16_t x_index; // 0xffff if not valid
    uint8_t  x_bitpos;
    uint8_t  x_size;
    uint16_t y_index; // 0xffff if not valid
    uint8_t  y_bitpos;
    uint8_t  y_size;
    uint16_t z_index; // 0xffff if not valid
    uint8_t  z_bitpos;
    uint8_t  z_size;
    uint16_t rz_index; // 0xffff if not valid
    uint8_t  rz_bitpos;
    uint8_t  rz_size;
    uint16_t hat_index; // 0xffff if not valid
    uint8_t  hat_bitpos;
    uint8_t  hat_size;
} gamepad_report_parser_info_t;

// Parsed gamepad report structure
typedef struct {
    int8_t x;           // X axis (-127 to 127)
    int8_t y;           // Y axis (-127 to 127)
    int8_t z;           // Z axis (-127 to 127)
    int8_t rz;          // RZ axis (-127 to 127)
    uint8_t hat;        // Hat switch (0-15)
    uint16_t buttons;   // Button states (16 buttons, bits 0-15)
} parsed_gamepad_report_t;

// External declaration of the Samwa gamepad parser info
extern const gamepad_report_parser_info_t Samwa_400_JYP62U_gamepad_report_info;

// Function to parse gamepad report
bool parse_gamepad_report(const uint8_t* report, uint16_t len, 
                         const gamepad_report_parser_info_t* parser_info,
                         parsed_gamepad_report_t* parsed_report);

// Function to process parsed gamepad report
void process_gamepad_report(uint8_t dev_addr, uint8_t instance, const parsed_gamepad_report_t* parsed_report);

// Global gamepad state variable for sharing between host and device
extern parsed_gamepad_report_t current_gamepad_state;
extern bool gamepad_state_updated;
extern bool has_gamepad_key;

#endif // GAMEPAD_REPORT_PARSER_H