#include "ReportParser.h"
#include <stdio.h>
#include <string.h>

defined_mouse_report_parser_info_t interface_report_parser_info[CFG_TUH_HID] = {0};
defined_mouse_report_parser_info_t defined_report_parser_info[99] = {0};


// 連続するreportのbit列から指定されたビット範囲を抽出する関数
// 複数ビットを一度に処理する効率的な実装
int16_t extract_bits_from_report(const uint8_t* report, uint16_t report_len,
                                        uint16_t byte_index, uint8_t bit_offset, uint8_t bit_size)
{
    if (byte_index >= report_len || bit_size == 0 || bit_size > 16) {
        return 0;
    }

    uint16_t result = 0;
    uint8_t remaining_bits = bit_size;
    uint8_t current_bit_offset = bit_offset;
    uint16_t current_byte_index = byte_index;

    while (remaining_bits > 0 && current_byte_index < report_len) {
        uint8_t current_byte = report[current_byte_index];
        
        // 現在のバイトから取得可能なビット数を計算
        uint8_t bits_available_in_byte = 8 - current_bit_offset;
        uint8_t bits_to_extract = (remaining_bits < bits_available_in_byte) ? 
                                 remaining_bits : bits_available_in_byte;
        
        // マスクを作成して必要なビットを抽出
        uint8_t mask = (1 << bits_to_extract) - 1;
        uint8_t extracted_bits = (current_byte >> current_bit_offset) & mask;
        
        // 結果に追加（下位ビットから順に格納）
        result |= ((uint16_t)extracted_bits) << (bit_size - remaining_bits);
        
        // 次のバイトの準備
        remaining_bits -= bits_to_extract;
        current_bit_offset = 0;  // 次のバイトは最初から
        current_byte_index++;
    }

    // 符号拡張処理（最上位ビットが1の場合、負の値として扱う）
    if (bit_size < 16 && (result & (1 << (bit_size - 1)))) {
        // 符号拡張：上位ビットを1で埋める
        result |= (0xFFFF << bit_size);
    }

    return (int16_t)result;
}
