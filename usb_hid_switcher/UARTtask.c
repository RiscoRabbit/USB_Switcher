#include "UARTtask.h"
#include "USBtask.h"
#include "MouseReportParser.h"

// Static variables for UART task
static uint8_t uart_buffer[UART_BUFFER_SIZE];
static uint16_t uart_buffer_index = 0;
static bool uart_initialized = false;

// Initialize UART1 for communication
void uart_task_init(void)
{
    if (uart_initialized) return;
    
    // Initialize UART1
    uart_init(UART_ID, UART_BAUD_RATE);
    
    // Set up UART pins
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Configure UART settings
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_hw_flow(UART_ID, false, false);
    uart_set_fifo_enabled(UART_ID, true);
    
    printf("UART1 initialized for mouse and keyboard data reception\n");
    printf("TX Pin: %d, RX Pin: %d, Baud: %d\n", UART_TX_PIN, UART_RX_PIN, UART_BAUD_RATE);
    
    uart_initialized = true;
}

// Main UART task - should be called regularly from main loop
void uart_task(void)
{
    if (!uart_initialized) {
        uart_task_init();
        return;
    }
    
    // Process any received UART data
    uart_process_received_data();
}

// Process received UART data
void uart_process_received_data(void)
{
    // Check if data is available on UART1
    while (uart_is_readable(UART_ID))
    {
        uint8_t received_char = uart_getc(UART_ID);
        /* if(tud_cdc_connected())
        {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%c", received_char);
            buffer[2] = '\0';
            tud_cdc_write_str(buffer);
            tud_cdc_write_flush();
        } */

        // Add character to buffer
        if (uart_buffer_index < UART_BUFFER_SIZE - 1) {
            uart_buffer[uart_buffer_index] = received_char;
            uart_buffer_index++;
            
            // Check for complete message (newline or carriage return)
            if (received_char == '\n' || received_char == '\r' || received_char == '\0') {
                // Null-terminate the message
                uart_buffer[uart_buffer_index - 1] = '\0';
                
                // Process the complete message if it's not empty
                if (uart_buffer_index > 1) {
                    uint8_t* base64_data_input = uart_buffer[15];
                    uint8_t decoded_data[8];
                    int decoded_length = base64_decode((char*)&(uart_buffer[2]), uart_buffer_index-3, decoded_data, sizeof(decoded_data));

                    /*
                    if(tud_cdc_connected())
                    {
                        char buffer[256];
                        snprintf(buffer, sizeof(buffer), "UART: received HID report: %s %d %d\r\n", uart_buffer, uart_buffer_index, decoded_length);
                        tud_cdc_write_str(buffer);
                        
                        for(int i = 0; i < decoded_length; i++) {
                            snprintf(buffer, sizeof(buffer), " %02X", decoded_data[i]);
                            tud_cdc_write_str(buffer);
                        }
                        tud_cdc_write_str("\r\n");
                        tud_cdc_write_flush();
                    }
                    */

                    // Check if this is a keyboard message starting with "K"
                    if (uart_buffer[0] == '0')
                    {
                        hid_keyboard_report_t keyboard_report;
                        uint8_t nokey[13] = "AAAAAAAAAAA=";
                        /* if (uart_parse_keyboard_message((char*)&(uart_buffer[2]), &keyboard_report))
                        {
                            tud_cdc_write_str("UART: reset all key\n");
                            tud_hid_n_keyboard_report(0, 1, keyboard_report.modifier, keyboard_report.keycode);
                        } */
                    }
                    int dev_id = uart_buffer[1] - '0';
                    if (dev_id != device_id && dev_id != 0) {
                        // Not for this device ID
                        uart_buffer_index = 0;
                        continue;
                    }

                    // Check if this is a keyboard message starting with "K"
                    if (uart_buffer[0] == 'K')
                    {
                        /* if(tud_cdc_connected())
                        {
                            tud_cdc_write_str("UART: received keyboard report\n");
                            tud_cdc_write_flush();
                        } */

                        hid_keyboard_report_t keyboard_report;
                        keyboard_report.modifier = decoded_data[0];
                        keyboard_report.reserved = decoded_data[1];
                        for (int i = 0; i < 6; i++) {
                            keyboard_report.keycode[i] = decoded_data[2 + i];
                        }

                        if (uart_parse_keyboard_message((char*)&(uart_buffer[2]), &keyboard_report)) {
                             // Send keyboard report to host
                            tud_hid_n_keyboard_report(0, 1, keyboard_report.modifier, keyboard_report.keycode);
                            /* printf("UART: Keyboard report sent to host\n"); */
                            /* tud_cdc_write_str("UART: Keyboard report sent to host\n");
                            tud_cdc_write_flush(); */
                            setLEDStateActive();
                        } else {
                            printf("UART: Failed to parse keyboard message: %s\n", uart_buffer);
                            /* tud_cdc_write_flush();
                            tud_cdc_write_str("UART: Failed to parse keyboard message\n"); */

                        }
                    }

                    // Check if this is a mouse message starting with "M"
                    if (uart_buffer[0] == 'M')
                    {
                        mouse_report_t mouse_report;
                        if (uart_parse_mouse_message((char*)&(uart_buffer[2]), &mouse_report)) {
                            // Send mouse report to host
                            tud_hid_n_mouse_report(1, 2, mouse_report.buttons,
                                mouse_report.x, mouse_report.y,
                                mouse_report.wheel, mouse_report.pan);
                            /* printf("UART: Mouse report sent to host\n");
                            tud_cdc_write_str("UART: Mouse report sent to host\n");
                            tud_cdc_write_flush(); */
                            setLEDStateActive();

                        } else {
                            printf("UART: Failed to parse mouse message: %s\n", uart_buffer);
                            tud_cdc_write_str("UART: Failed to parse mouse message\n");
                            tud_cdc_write_flush();

                        }
                    }

                    // Check if this is a gamepad message starting with "G"
                    if (uart_buffer[0] == 'G')
                    {
                        parsed_gamepad_report_t gamepad_report;
                        if (uart_parse_gamepad_message((char*)&(uart_buffer[2]), &gamepad_report)) {
                            // Update global gamepad state
                            current_gamepad_state = gamepad_report;
                            gamepad_state_updated = true;
                            has_gamepad_key = true;
                            
                            /* printf("UART: Gamepad report received - X:%d Y:%d Z:%d RZ:%d Hat:%u Buttons:0x%04X\n",
                                   gamepad_report.x, gamepad_report.y, gamepad_report.z, gamepad_report.rz,
                                   gamepad_report.hat, gamepad_report.buttons); */
                            setLEDStateActive();

                        } else {
                            printf("UART: Failed to parse gamepad message: %s\n", uart_buffer);
                            tud_cdc_write_str("UART: Failed to parse gamepad message\n");
                            tud_cdc_write_flush();
                        }
                    }



                }
                
                // Reset buffer for next message
                uart_buffer_index = 0;
            }
        } else {
            // Buffer overflow - reset buffer
            printf("UART buffer overflow, resetting\n");
            uart_buffer_index = 0;
        }
    }
}

// Parse mouse message from base64 encoded UART data
bool uart_parse_mouse_message(const char* message, mouse_report_t* mouse_report)
{
    if (!message || !mouse_report) return false;
    
    int message_length = strlen(message);
    if (message_length != 12) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "UART: mouse message receive failed, %s expected 12 bytes, got %d\n",message, message_length);
        tud_cdc_write_str(buffer);
        return false;
    }
    
    // Extract base64 data (skip "M" prefix)
    const char* base64_data = &message[0];
    
    // Decode base64 data
    uint8_t decoded_data[8]; // Should contain: buttons, x, y, wheel, pan
    int decoded_length = base64_decode(base64_data, strlen(base64_data), decoded_data, sizeof(decoded_data));
    
    if (decoded_length != 8) {
        printf("UART: Base64 decode failed, expected 8 bytes, got %d\n", decoded_length);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "UART: Base64 decode failed, %s expected 8 bytes, got %d\n",message, decoded_length);
        tud_cdc_write_str(buffer);
        return false;
    }
    
    // Parse the decoded data into mouse report structure
    mouse_report->buttons = (int16_t)(decoded_data[0] | (decoded_data[1] << 8));
    mouse_report->x = (int16_t)(decoded_data[2] | (decoded_data[3] << 8));       // Cast to signed
    mouse_report->y = (int16_t)(decoded_data[4] | (decoded_data[5] << 8));       // Cast to signed
    mouse_report->wheel = (int8_t)(decoded_data[6]);   // Cast to signed
    mouse_report->pan = (int8_t)(decoded_data[7]);     // Cast to signed
    
    return true;
}

// Parse keyboard message from base64 encoded UART data
bool uart_parse_keyboard_message(const char* message, hid_keyboard_report_t* keyboard_report)
{
    if (!message || !keyboard_report) return false;
    
    // Message should start with "K" followed by base64 characters
    if (strlen(message) < 9) {
        return false;
    }
    
    // Extract base64 data (skip "K" prefix)
    const char* base64_data = message;
    
    // Decode base64 data
    uint8_t decoded_data[8]; // Standard HID keyboard report is 8 bytes
    int decoded_length = base64_decode(base64_data, strlen(base64_data), decoded_data, sizeof(decoded_data));
    
    if (decoded_length != 8) {

        printf("UART: Keyboard base64 decode failed, expected 8 bytes, got %d\n", decoded_length);

        char buffer[256];
        snprintf(buffer, sizeof(buffer), "UART: Keyboard base64 decode failed, %s expected 8 bytes, got %d\n",message, decoded_length);
        tud_cdc_write_str(buffer);
        return false;
    }
    
    // Parse the decoded data into keyboard report structure
    // Standard HID keyboard report format:
    // Byte 0: Modifier keys (Ctrl, Shift, Alt, GUI)
    // Byte 1: Reserved (usually 0)
    // Bytes 2-7: Keycode array (up to 6 simultaneous keys)
    keyboard_report->modifier = decoded_data[0];
    keyboard_report->reserved = decoded_data[1];
    for (int i = 0; i < 6; i++) {
        keyboard_report->keycode[i] = decoded_data[2 + i];
    }
    
    return true;
}

// Parse gamepad message from base64 encoded UART data
bool uart_parse_gamepad_message(const char* message, parsed_gamepad_report_t* gamepad_report)
{
    if (!message || !gamepad_report) return false;
    
    int message_length = strlen(message);
    if (message_length != 12) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "UART: gamepad message receive failed, %s expected 12 bytes, got %d\n", message, message_length);
        tud_cdc_write_str(buffer);
        return false;
    }
    
    // Extract base64 data
    const char* base64_data = &message[0];
    
    // Decode base64 data
    uint8_t decoded_data[8]; // Should contain: x, y, z, rz, hat, buttons(2 bytes), reserved
    int decoded_length = base64_decode(base64_data, strlen(base64_data), decoded_data, sizeof(decoded_data));
    
    if (decoded_length != 8) {
        printf("UART: Gamepad Base64 decode failed, expected 8 bytes, got %d\n", decoded_length);
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "UART: Gamepad Base64 decode failed, %s expected 8 bytes, got %d\n", message, decoded_length);
        tud_cdc_write_str(buffer);
        return false;
    }
    
    // Parse the decoded data into gamepad report structure
    gamepad_report->x = (int8_t)decoded_data[0];      // X axis
    gamepad_report->y = (int8_t)decoded_data[1];      // Y axis  
    gamepad_report->z = (int8_t)decoded_data[2];      // Z axis
    gamepad_report->rz = (int8_t)decoded_data[3];     // RZ axis
    gamepad_report->hat = decoded_data[4];            // Hat switch
    gamepad_report->buttons = (uint16_t)(decoded_data[5] | (decoded_data[6] << 8));  // Buttons (2 bytes)
    // decoded_data[7] is reserved/unused
    
    return true;
}