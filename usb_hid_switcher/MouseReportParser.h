#ifndef MOUSE_REPORT_PARSER_H
#define MOUSE_REPORT_PARSER_H

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
    uint16_t wheel_index; // 0xffff if not valid
    uint8_t  wheel_bitpos;
    uint8_t  wheel_size;
    uint16_t pan_index; // 0xffff if not valid
    uint8_t  pan_bitpos;
    uint8_t  pan_size;
} mouse_report_parser_info_t;

typedef struct {
    uint16_t buttons;
    int16_t  x;
    int16_t  y;
    int8_t  wheel;
    int8_t  pan;
} mouse_report_t;

// 関数プロトタイプ
void mouse_report_parser(const mouse_report_parser_info_t* mouse_info,
                         const uint8_t* report, uint16_t report_len,
                         mouse_report_t* mouse_report);
void mouse_report_dscriptor_parser(const uint8_t* desc, uint16_t desc_len);

extern const mouse_report_parser_info_t boot_mouse_report_info;
extern const mouse_report_parser_info_t logicool_unified_receiver_mouse_report_info;
extern const mouse_report_parser_info_t logicool_G300s_receiver_mouse_report_info;



#endif // MOUSE_REPORT_PARSER_H