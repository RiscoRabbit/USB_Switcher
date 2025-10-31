#include "GamepadReportParser.h"
#include <stdio.h>
#include <string.h>
#include "fstask.h"  // For file operations
#include "CDCCmd.h"  // For fifo_push
#include "LuaTask.h" // For duplicate checking functions
#include "base64.h"  // For base64 encoding
#include "hardware/uart.h" // For UART communication

const gamepad_report_parser_info_t Samwa_400_JYP62U_gamepad_report_info = {
         .ReportID = 0xffff,
         .buttons_index = 4,     .buttons_bitpos = 4,      .buttons_size = 16,
    // pad 4bit
          .x_index = 0,           .x_bitpos = 0,            .x_size = 8,
          .y_index = 1,           .y_bitpos = 0,            .y_size = 8,
          .z_index = 2,           .z_bitpos = 0,            .z_size = 8,
          .rz_index = 3,          .rz_bitpos = 0,          .rz_size = 8,
          .hat_index = 4,         .hat_bitpos = 0,         .hat_size = 4
};

// Global gamepad state variables
parsed_gamepad_report_t current_gamepad_state = {0};
bool gamepad_state_updated = false;

// Previous button state for change detection
static uint16_t previous_gamepad_buttons = 0;

// External variables
extern uint8_t USB_output_switch; // From LuaTask.c
extern int device_id; // From USBtask.c

// Helper function to extract bits from a byte array
static uint32_t extract_bits(const uint8_t* data, uint16_t byte_index, uint8_t bit_pos, uint8_t bit_size) {
    if (bit_size == 0) return 0;
    
    uint32_t value = 0;
    uint8_t bits_read = 0;
    
    while (bits_read < bit_size) {
        uint8_t current_byte = data[byte_index];
        uint8_t bits_in_byte = 8 - bit_pos;
        uint8_t bits_to_read = (bit_size - bits_read) < bits_in_byte ? (bit_size - bits_read) : bits_in_byte;
        
        uint8_t mask = (1 << bits_to_read) - 1;
        uint8_t byte_value = (current_byte >> bit_pos) & mask;
        
        value |= (byte_value << bits_read);
        
        bits_read += bits_to_read;
        byte_index++;
        bit_pos = 0;
    }
    
    return value;
}

// Function to parse gamepad report according to parser info
bool parse_gamepad_report(const uint8_t* report, uint16_t len, 
                         const gamepad_report_parser_info_t* parser_info,
                         parsed_gamepad_report_t* parsed_report) {
    if (!report || !parser_info || !parsed_report) {
        return false;
    }

    if(len != 7) // for sanwa gamepad
    {
        return false;
    }

    
    // Initialize parsed report
    memset(parsed_report, 0, sizeof(parsed_gamepad_report_t));
    
    // Parse X axis
    if (parser_info->x_index != 0xffff && parser_info->x_index < len) {
        uint32_t raw_x = extract_bits(report, parser_info->x_index, parser_info->x_bitpos, parser_info->x_size);
        // Convert to signed 8-bit value
        parsed_report->x = (int8_t)(raw_x - 128);
    }
    
    // Parse Y axis
    if (parser_info->y_index != 0xffff && parser_info->y_index < len) {
        uint32_t raw_y = extract_bits(report, parser_info->y_index, parser_info->y_bitpos, parser_info->y_size);
        // Convert to signed 8-bit value
        parsed_report->y = (int8_t)(raw_y - 128);
    }
    
    // Parse Z axis
    if (parser_info->z_index != 0xffff && parser_info->z_index < len) {
        uint32_t raw_z = extract_bits(report, parser_info->z_index, parser_info->z_bitpos, parser_info->z_size);
        // Convert to signed 8-bit value
        parsed_report->z = (int8_t)(raw_z - 128);
    }
    
    // Parse RZ axis
    if (parser_info->rz_index != 0xffff && parser_info->rz_index < len && parser_info->rz_size > 0) {
        uint32_t raw_rz = extract_bits(report, parser_info->rz_index, parser_info->rz_bitpos, parser_info->rz_size);
        // Convert to signed 8-bit value
        parsed_report->rz = (int8_t)(raw_rz - 128);
    }
    
    // Parse Hat switch
    if (parser_info->hat_index != 0xffff && parser_info->hat_index < len) {
        parsed_report->hat = (uint8_t)extract_bits(report, parser_info->hat_index, parser_info->hat_bitpos, parser_info->hat_size);
    }
    
    // Parse buttons
    if (parser_info->buttons_index != 0xffff && parser_info->buttons_index < len) {
        parsed_report->buttons = (uint16_t)extract_bits(report, parser_info->buttons_index, parser_info->buttons_bitpos, parser_info->buttons_size);
    }
    
    return true;
}

// Handle gamepad button press for Pad-X file execution
static void handle_gamepad_button_press(int button_number)
{
    // Generate filename in format "Pad-1", "Pad-2", etc.
    char filename[16];
    snprintf(filename, sizeof(filename), "Pad-%d", button_number);
    
    // Check if file exists by trying to read it
    char buffer[1]; // Just check existence, don't need to read content
    int result = fstask_read_file(filename, buffer, 1);
    
    if (result >= 0) {
        // File exists, add to Lua execution queue with duplicate checking
        printf("[Gamepad] Found Lua script: %s, adding to queue...\n", filename);
        
        // Create a file execution command for the FIFO queue
        char file_command[64];
        snprintf(file_command, sizeof(file_command), "FILE:%s", filename);
        
        // Use enhanced fifo_push with duplicate checking
        if (fifo_push_with_duplicate_check(file_command)) {
            printf("[Gamepad] Lua script %s added to execution queue\n", filename);
        } else {
            // Check if it was rejected due to duplicate or queue full
            if (fifo_get_count() >= 16) { // FIFO_BUFFER_SIZE is 16
                printf("[Gamepad] Error: Execution queue is full, cannot add %s\n", filename);
            } else {
                printf("[Gamepad] Script %s is already executing or queued - skipping\n", filename);
            }
        }
    } else {
        printf("[Gamepad] Script %s not found\n", filename);
    }
}

// Function to process parsed gamepad report
void process_gamepad_report(uint8_t dev_addr, uint8_t instance, const parsed_gamepad_report_t* parsed_report) {
    /* printf("[%u:%u] Parsed Gamepad - X:%d Y:%d Z:%d RZ:%d Hat:%u Buttons:0x%04X\n",
           dev_addr, instance, 
           parsed_report->x, parsed_report->y, parsed_report->z, parsed_report->rz,
           parsed_report->hat, parsed_report->buttons); */
    
    // Check for button press events (compare with previous state)
    uint16_t current_buttons = parsed_report->buttons;
    uint16_t button_changes = current_buttons ^ previous_gamepad_buttons;  // XOR to find changes
    uint16_t newly_pressed = button_changes & current_buttons;  // Buttons that just got pressed
    
    // Check each button (1-16) for new presses
    for (int button = 1; button <= 16; button++) {
        uint16_t button_mask = 1 << (button - 1);
        if (newly_pressed & button_mask) {
            printf("[Gamepad] Button %d pressed\n", button);
            handle_gamepad_button_press(button);
        }
    }
    
    // Update previous button state for next comparison
    previous_gamepad_buttons = current_buttons;
    
    // Update global gamepad state for USB device to use
    current_gamepad_state = *parsed_report;
    gamepad_state_updated = true;
    has_gamepad_key = true; // Indicate that gamepad is active
    
    // Send gamepad data to UART0 regardless of USB_output_switch value
    {
        // Prepare gamepad data for UART transmission
        // Pack gamepad data into 8 bytes: x, y, z, rz, hat, buttons(2 bytes), reserved
        uint8_t gamepad_data[8];
        gamepad_data[0] = (uint8_t)parsed_report->x;     // X axis
        gamepad_data[1] = (uint8_t)parsed_report->y;     // Y axis
        gamepad_data[2] = (uint8_t)parsed_report->z;     // Z axis
        gamepad_data[3] = (uint8_t)parsed_report->rz;    // RZ axis
        gamepad_data[4] = parsed_report->hat;            // Hat switch
        gamepad_data[5] = (uint8_t)(parsed_report->buttons & 0xFF);        // Buttons low byte
        gamepad_data[6] = (uint8_t)((parsed_report->buttons >> 8) & 0xFF); // Buttons high byte
        gamepad_data[7] = 0;                             // Reserved/unused

        // Encode to base64
        char base64_output[16]; // 8 bytes -> 12 chars base64 + null terminator + padding
        int encoded_length = base64_encode(gamepad_data, sizeof(gamepad_data), base64_output, sizeof(base64_output));
        
        if (encoded_length > 0) {
            // Send gamepad report to UART1 in format: "GX<base64_data>\n"
            // where X is the device_id
            char uart_message[32];
            snprintf(uart_message, sizeof(uart_message), "G%d%s\n", device_id, base64_output);
            
            // Send to UART1
            uart_puts(uart1, uart_message);
            
            /* printf("UART1: gamepad report sent: %s\n", uart_message); */
        } else {
            printf("UART1: Failed to encode gamepad data to base64\n");
        }
    }
    
    // Here you can add additional logic to:
    // 1. Apply any transformations or mappings
    // 2. Filter or validate the data
    // 3. Handle device-specific processing
}

