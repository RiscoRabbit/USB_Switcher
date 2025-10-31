#ifndef REPORT_PARSER_H
#define REPORT_PARSER_H

#include <stdint.h>
#include <stdbool.h>
#include "tusb_config.h"
#include "MouseReportParser.h"

typedef enum {
    BOOT = 0,
    PROTOCOL = 1
    // Add other parser types as needed
} protocol_t;


typedef struct {
    bool is_valid;
    uint16_t vid;
    uint16_t pid;
    protocol_t protocol;
    uint8_t dev_addr;
    uint8_t instance;
    mouse_report_parser_info_t* parser_info; // Pointer to mouse_report_parser_info_t
} defined_mouse_report_parser_info_t;

extern defined_mouse_report_parser_info_t interface_report_parser_info[CFG_TUH_HID];

extern defined_mouse_report_parser_info_t defined_report_parser_info[99];

int16_t extract_bits_from_report(const uint8_t* report, uint16_t report_len,
                                        uint16_t byte_index, uint8_t bit_offset, uint8_t bit_size);

#endif // REPORT_PARSER_H

