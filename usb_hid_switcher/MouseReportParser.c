#include "MouseReportParser.h"
#include "ReportParser.h"
#include <stdio.h>
#include <string.h>




const mouse_report_parser_info_t boot_mouse_report_info = {
         .ReportID = 0xffff,
    .buttons_index = 0,     .buttons_bitpos = 0,      .buttons_size = 5,
    // pad 3bit
          .x_index = 1,           .x_bitpos = 0,            .x_size = 8,
          .y_index = 2,           .y_bitpos = 0,            .y_size = 8,
      .wheel_index = 3,       .wheel_bitpos = 0,        .wheel_size = 8,
        .pan_index = 0xffff,    .pan_bitpos = 0,          .pan_size = 8
};

const mouse_report_parser_info_t logicool_unified_receiver_mouse_report_info = {
         .ReportID = 0x02,
    .buttons_index = 1,     .buttons_bitpos  = 0,     .buttons_size = 16,
          .x_index = 3,           .x_bitpos  = 0,           .x_size = 12,
          .y_index = 4,            .y_bitpos = 4,           .y_size = 12,
      .wheel_index = 6,        .wheel_bitpos = 0,       .wheel_size = 8,
        .pan_index = 7,          .pan_bitpos = 0,         .pan_size = 8
};

const mouse_report_parser_info_t logicool_G300s_receiver_mouse_report_info = {
         .ReportID = 0xffff,
    .buttons_index = 0,      .buttons_bitpos = 0,    .buttons_size = 16,
          .x_index = 2,            .x_bitpos = 0,          .x_size = 16,
          .y_index = 4,            .y_bitpos = 0,          .y_size = 16,
      .wheel_index = 6,        .wheel_bitpos = 0,      .wheel_size = 8,
        .pan_index = 0xffff,     .pan_bitpos = 0,        .pan_size = 0
};


void mouse_report_parser(const mouse_report_parser_info_t* mouse_info,
                         const uint8_t* report, uint16_t report_len,
                         mouse_report_t* mouse_report)
{
    // Initialize output structure
    memset(mouse_report, 0, sizeof(mouse_report_t));

    if(report_len <= 4)
    {
        // Standard Boot Mouse Report
        mouse_report->buttons = report[0];
        mouse_report->x = (int8_t)report[1];
        mouse_report->y = (int8_t)report[2];
        if(report_len == 4)
        {
        mouse_report->wheel = (int8_t)report[3];
        }
        mouse_report->pan = 0;
        return;
    }

    // Extract buttons
    if(mouse_info->buttons_index != 0xffff)
    {
        uint16_t byte_index = mouse_info->buttons_index;
        uint8_t bitpos = mouse_info->buttons_bitpos;
        uint8_t size = mouse_info->buttons_size;

        int16_t buttons = extract_bits_from_report(report, report_len, byte_index, bitpos, size);
        mouse_report->buttons = (uint16_t)buttons;  // ボタンは符号なしとして扱う
    }
    else
    {
        mouse_report->buttons = 0;
    }

    // Extract X coordinate
    if(mouse_info->x_index != 0xffff && mouse_info->x_index + mouse_info->x_size / 8 < report_len)
    {
        int16_t value = extract_bits_from_report(report, report_len, 
                                                 mouse_info->x_index, 
                                                 mouse_info->x_bitpos, 
                                                 mouse_info->x_size);
        mouse_report->x = value;
    }

    // Extract Y coordinate  
    if(mouse_info->y_index != 0xffff && mouse_info->y_index + mouse_info->y_size / 8 < report_len)
    {
        int16_t value = extract_bits_from_report(report, report_len,
                                                 mouse_info->y_index,
                                                 mouse_info->y_bitpos,
                                                 mouse_info->y_size);
        mouse_report->y = value;
    }

    // Extract wheel
    if(mouse_info->wheel_index != 0xffff && mouse_info->wheel_index + mouse_info->wheel_size / 8 < report_len)
    {
        int16_t value = extract_bits_from_report(report, report_len,
                                                 mouse_info->wheel_index,
                                                 mouse_info->wheel_bitpos,
                                                 mouse_info->wheel_size);
        mouse_report->wheel = value;
    }

    // Extract pan (horizontal wheel)
    if(mouse_info->pan_index != 0xffff && mouse_info->pan_index + mouse_info->pan_size / 8 < report_len)
    {
        int16_t value = extract_bits_from_report(report, report_len,
                                                 mouse_info->pan_index,
                                                 mouse_info->pan_bitpos,
                                                 mouse_info->pan_size);
        mouse_report->pan = value;
    }
}

