#include "base64.h"

// Base64 decode table
static const uint8_t base64_decode_table[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 62,   0xFF, 0xFF, 0xFF, 63,
    52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
    15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
    41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// Base64 encode table
static const char base64_encode_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Base64 decode function
 * @param input Base64 encoded input string
 * @param input_len Length of input string
 * @param output Buffer to store decode result
 * @param output_len Size of output buffer
 * @return Number of decoded bytes, negative value on error
 */
int base64_decode(const char* input, size_t input_len, uint8_t* output, size_t output_len)
{
    if (!input || !output || input_len == 0) {
        return -1;
    }

    size_t decoded_len = 0;
    uint32_t buffer = 0;
    int bits_collected = 0;
    
    for (size_t i = 0; i < input_len; i++) {
        char c = input[i];
        
        // Skip whitespace and newlines
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            continue;
        }
        
        // Handle padding
        if (c == '=') {
            break;
        }
        
        // Get decode value
        uint8_t decode_val = base64_decode_table[(uint8_t)c];
        if (decode_val == 0xFF) {
            return -2; // Invalid character
        }
        
        buffer = (buffer << 6) | decode_val;
        bits_collected += 6;
        
        if (bits_collected >= 8) {
            if (decoded_len >= output_len) {
                return -3; // Output buffer too small
            }
            
            output[decoded_len++] = (buffer >> (bits_collected - 8)) & 0xFF;
            bits_collected -= 8;
        }
    }
    
    return (int)decoded_len;
}

/**
 * Base64 encode function
 * @param input Input data to encode
 * @param input_len Length of input data
 * @param output Buffer to store encode result
 * @param output_len Size of output buffer
 * @return Number of encoded characters, negative value on error
 */
int base64_encode(const uint8_t* input, size_t input_len, char* output, size_t output_len)
{
    if (!input || !output || input_len == 0) {
        return -1;
    }

    // Calculate required output size (4 characters for every 3 bytes, plus null terminator)
    size_t required_len = ((input_len + 2) / 3) * 4 + 1;
    if (output_len < required_len) {
        return -3; // Output buffer too small
    }

    size_t output_index = 0;
    
    for (size_t i = 0; i < input_len; i += 3) {
        uint32_t buffer = 0;
        int bytes_to_process = (input_len - i >= 3) ? 3 : (int)(input_len - i);
        
        // Pack bytes into buffer (big-endian)
        for (int j = 0; j < bytes_to_process; j++) {
            buffer = (buffer << 8) | input[i + j];
        }
        
        // Shift to align for 3-byte groups
        buffer <<= (3 - bytes_to_process) * 8;
        
        // Extract 6-bit groups and encode
        for (int j = 0; j < 4; j++) {
            if (j < ((bytes_to_process * 8 + 5) / 6)) {
                output[output_index++] = base64_encode_table[(buffer >> (18 - j * 6)) & 0x3F];
            } else {
                output[output_index++] = '='; // Padding
            }
        }
    }
    
    output[output_index] = '\0'; // Null terminator
    return (int)output_index;
}