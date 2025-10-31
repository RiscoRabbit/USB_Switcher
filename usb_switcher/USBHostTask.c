#include "USBHostTask.h"
#include "USBtask.h"
#include "fstask.h"
#include "LuaTask.h"
#include "UARTtask.h"
#include "LEDtask.h"
#include "base64.h"
#include "CDCCmd.h"
#include "class/hid/hid.h"  // For HID_PROTOCOL_* constants
#include "tusb.h"  // For tud_cdc_write_str()
#include "MouseReportParser.h"
#include "ReportParser.h"
#include "GamepadReportParser.h"
#include "USBHostTask.h"
#include "configRead.h"

// External variables defined in USBtask.c
extern bool meta;
extern uint8_t metaKeyCode;

// Keyboard LED state tracking and device information
typedef struct {
    uint8_t dev_addr;
    uint8_t instance;
    bool is_keyboard;
    bool connected;
} keyboard_device_info_t;

static keyboard_device_info_t keyboard_devices[CFG_TUH_HID];
static uint8_t keyboard_device_count = 0;

// Function to send keyboard LED state to connected keyboards
void send_keyboard_led_state(uint8_t led_state)
{
    for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
        if (keyboard_devices[i].connected && keyboard_devices[i].is_keyboard) {
            uint8_t dev_addr = keyboard_devices[i].dev_addr;
            uint8_t instance = keyboard_devices[i].instance;
            
            // Send SET_REPORT request to keyboard
            bool success = tuh_hid_set_report(dev_addr, instance, 0, HID_REPORT_TYPE_OUTPUT, &led_state, 1);
        }
    }
}

// Keyboard report buffering for retry functionality
static buffered_keyboard_report_t keyboard_report_buffer[KEYBOARD_REPORT_BUFFER_SIZE];
static uint8_t kbd_buffer_write_index = 0;
static uint8_t kbd_buffer_read_index = 0;
static uint8_t kbd_buffer_count = 0;

// Mouse report buffering for retry functionality
static buffered_mouse_report_t mouse_report_buffer[MOUSE_REPORT_BUFFER_SIZE];
static uint8_t mouse_buffer_write_index = 0;
static uint8_t mouse_buffer_read_index = 0;
static uint8_t mouse_buffer_count = 0;

//--------------------------------------------------------------------+
// Keyboard Report Buffering Functions
//--------------------------------------------------------------------+

// Add keyboard report to buffer for retry
static bool buffer_keyboard_report(const hid_keyboard_report_t* report)
{
    if (kbd_buffer_count >= KEYBOARD_REPORT_BUFFER_SIZE) {
        printf("Warning: Keyboard report buffer full, dropping oldest report\n");
        // Drop oldest report by advancing read index
        kbd_buffer_read_index = (kbd_buffer_read_index + 1) % KEYBOARD_REPORT_BUFFER_SIZE;
        kbd_buffer_count--;
    }
    
    keyboard_report_buffer[kbd_buffer_write_index].report = *report;
    keyboard_report_buffer[kbd_buffer_write_index].timestamp = board_millis();
    keyboard_report_buffer[kbd_buffer_write_index].valid = true;
    
    kbd_buffer_write_index = (kbd_buffer_write_index + 1) % KEYBOARD_REPORT_BUFFER_SIZE;
    kbd_buffer_count++;
    
    // printf("Buffered keyboard report (buffer count: %d)\n", kbd_buffer_count);
    return true;
}

// Try to send all buffered keyboard reports
void try_send_buffered_keyboard_reports(void)
{
    const uint32_t REPORT_TIMEOUT_MS = 1000; // Timeout reports after 1 second
    uint32_t current_time = board_millis();
    
    while (kbd_buffer_count > 0) {
        buffered_keyboard_report_t* buffered = &keyboard_report_buffer[kbd_buffer_read_index];
        
        // Check if report has timed out
        if (current_time - buffered->timestamp > REPORT_TIMEOUT_MS) {
            printf("Keyboard report timed out, dropping\n");
            kbd_buffer_read_index = (kbd_buffer_read_index + 1) % KEYBOARD_REPORT_BUFFER_SIZE;
            kbd_buffer_count--;
            continue;
        }
        
        // Try to send the report
        if (tud_connected() && tud_hid_n_ready(0)) {
            bool success = tud_hid_n_keyboard_report(0, 1, buffered->report.modifier, buffered->report.keycode);
            if (success) {
                //printf("Successfully sent buffered keyboard report (remaining: %d)\n", kbd_buffer_count - 1);
                kbd_buffer_read_index = (kbd_buffer_read_index + 1) % KEYBOARD_REPORT_BUFFER_SIZE;
                kbd_buffer_count--;
            } else {
                printf("Failed to send buffered keyboard report, will retry\n");
                break; // Stop trying, interface still busy
            }
        } else {
            // Interface not ready, stop trying
            break;
        }
    }
}

// Add mouse report to buffer for retry
static bool buffer_mouse_report(const mouse_report_t* report)
{
    if (mouse_buffer_count >= MOUSE_REPORT_BUFFER_SIZE) {
        printf("Warning: Mouse report buffer full, dropping oldest report\n");
        // Drop oldest report by advancing read index
        mouse_buffer_read_index = (mouse_buffer_read_index + 1) % MOUSE_REPORT_BUFFER_SIZE;
        mouse_buffer_count--;
    }
    
    mouse_report_buffer[mouse_buffer_write_index].report = *report;
    mouse_report_buffer[mouse_buffer_write_index].timestamp = board_millis();
    mouse_report_buffer[mouse_buffer_write_index].valid = true;
    
    mouse_buffer_write_index = (mouse_buffer_write_index + 1) % MOUSE_REPORT_BUFFER_SIZE;
    mouse_buffer_count++;
    
    // printf("Buffered mouse report (buffer count: %d)\n", mouse_buffer_count);
    return true;
}

// Try to send all buffered mouse reports
void try_send_buffered_mouse_reports(void)
{
    const uint32_t REPORT_TIMEOUT_MS = 1000; // Timeout reports after 1 second
    uint32_t current_time = board_millis();
    
    while (mouse_buffer_count > 0) {
        buffered_mouse_report_t* buffered = &mouse_report_buffer[mouse_buffer_read_index];
        
        // Check if report has timed out
        if (current_time - buffered->timestamp > REPORT_TIMEOUT_MS) {
            printf("Mouse report timed out, dropping\n");
            mouse_buffer_read_index = (mouse_buffer_read_index + 1) % MOUSE_REPORT_BUFFER_SIZE;
            mouse_buffer_count--;
            continue;
        }
        
        // Try to send the report
        if (tud_connected() && tud_hid_n_ready(1)) {
            bool success = tud_hid_n_mouse_report(1, 2, buffered->report.buttons,
                buffered->report.x, buffered->report.y,
                buffered->report.wheel, buffered->report.pan);
            if (success) {
                // printf("Successfully sent buffered mouse report (remaining: %d)\n", mouse_buffer_count - 1);
                mouse_buffer_read_index = (mouse_buffer_read_index + 1) % MOUSE_REPORT_BUFFER_SIZE;
                mouse_buffer_count--;
            } else {
                printf("Failed to send buffered mouse report, will retry\n");
                break; // Stop trying, interface still busy
            }
        } else {
            // Interface not ready, stop trying
            break;
        }
    }
}

//--------------------------------------------------------------------+
// USB Host Functions
//--------------------------------------------------------------------+

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode)
{
    for(uint8_t i=0; i<6; i++)
    {
        if (report->keycode[i] == keycode)  return true;
    }
    return false;
}



// Convert HID keycode to character for Meta key filenames
static char keycode_to_char(uint8_t keycode)
{
    
    // US keyboard layout mapping for A-Z keys
    if (keycode >= 0x04 && keycode <= 0x1d) {
        return 'A' + (keycode - 0x04);
    }
    // Number keys 1-9, 0
    else if (keycode >= 0x1e && keycode <= 0x26) {
        return '1' + (keycode - 0x1e);
    }
    else if (keycode == 0x27) {
        return '0';
    }
    // Some special keys
    else if (keycode == 0x2c) return ' '; // Space (not used for filename)
    else if (keycode == 0x2d) return '-'; // Minus
    else if (keycode == 0x2e) return '='; // Equal
    
    // Return null character for unsupported keys
    return '\0';
}

// Convert HID keycode to string representation for Meta key filenames
static char* keycode_to_str(uint8_t keycode)
{
    // Function keys F1-F12
    if (keycode >= 0x3a && keycode <= 0x45) {
        static char f_key_strings[12][4] = {
            "F1", "F2", "F3", "F4", "F5", "F6",
            "F7", "F8", "F9", "F10", "F11", "F12"
        };
        return f_key_strings[keycode - 0x3a];
    }
    
    // US keyboard layout mapping for A-Z keys
    if (keycode >= 0x04 && keycode <= 0x1d) {
        static char letter_strings[26][2] = {
            "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
            "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
        };
        return letter_strings[keycode - 0x04];
    }
    
    // Number keys 1-9, 0
    if (keycode >= 0x1e && keycode <= 0x26) {
        static char number_strings[9][2] = {
            "1", "2", "3", "4", "5", "6", "7", "8", "9"
        };
        return number_strings[keycode - 0x1e];
    }
    if (keycode == 0x27) {
        static char zero_str[] = "0";
        return zero_str;
    }
    
    // Arrow keys
    if (keycode == 0x4f) {
        static char right_str[] = "Right";
        return right_str;
    }
    if (keycode == 0x50) {
        static char left_str[] = "Left";
        return left_str;
    }
    if (keycode == 0x51) {
        static char down_str[] = "Down";
        return down_str;
    }
    if (keycode == 0x52) {
        static char up_str[] = "Up";
        return up_str;
    }
    
    // Special keys
    if (keycode == 0x2c) {
        static char space_str[] = "Space";
        return space_str;
    }
    if (keycode == 0x2d) {
        static char minus_str[] = "Minus";
        return minus_str;
    }
    if (keycode == 0x2e) {
        static char equal_str[] = "Equal";
        return equal_str;
    }
    if (keycode == 0x28) {
        static char enter_str[] = "Enter";
        return enter_str;
    }
    if (keycode == 0x2a) {
        static char backspace_str[] = "Backspace";
        return backspace_str;
    }
    if (keycode == 0x2b) {
        static char tab_str[] = "Tab";
        return tab_str;
    }
    if (keycode == 0x29) {
        static char escape_str[] = "Escape";
        return escape_str;
    }
    
    // Page Up/Down, Home, End, Insert, Delete
    if (keycode == 0x4b) {
        static char pageup_str[] = "PageUp";
        return pageup_str;
    }
    if (keycode == 0x4e) {
        static char pagedown_str[] = "PageDown";
        return pagedown_str;
    }
    if (keycode == 0x4a) {
        static char home_str[] = "Home";
        return home_str;
    }
    if (keycode == 0x4d) {
        static char end_str[] = "End";
        return end_str;
    }
    if (keycode == 0x49) {
        static char insert_str[] = "Insert";
        return insert_str;
    }
    if (keycode == 0x4c) {
        static char delete_str[] = "Delete";
        return delete_str;
    }
    
    // Return empty string for unsupported keys
    static char empty_str[] = "";
    return empty_str;
}

// Generate Meta-X filename and execute Lua script if it exists
static void handle_meta_key(uint8_t keycode)
{
    char* key_str = keycode_to_str(keycode);
    if (key_str[0] == '\0') {
        // printf("META Key pressed: 0x%02x (not supported for script execution)\n", keycode);
        return;
    }
    
    // Handle special Meta key combinations for USB output switching
    if (keycode == 0x11) { // N key (keycode 0x11)
        USB_output_switch = 1;
        printf("META+N: USB_output_switch set to 1 (UART mode)\n");
        // Send C11 to UART1
        uart_puts(uart1, "C11\n");
        return;
    } else if (keycode == 0x10) { // M key (keycode 0x10)
        USB_output_switch = 0;
        printf("META+M: USB_output_switch set to 0 (USB mode)\n");
        // Send C10 to UART1
        uart_puts(uart1, "C10\n");
        return;
    }
    
    // Generate filename in format "Meta-A"
    char filename[16];
    snprintf(filename, sizeof(filename), "Meta-%s", key_str);
    
    // Meta mode key press detected but Lua macro execution is disabled
    printf("META Key pressed: %s (Lua macro execution disabled)\n", key_str);
    
    // Note: Lua macro execution has been disabled for Meta mode keys
    // The following code has been commented out to prevent Lua macro execution:
    /*
    // Check if file exists by trying to read it
    char buffer[1]; // Just check existence, don't need to read content
    int result = fstask_read_file(filename, buffer, 1);
    
    if (result >= 0) {
        // File exists, add to Lua execution queue with duplicate checking
        //printf("Found Lua script: %s, adding to queue...\n", filename);
        
        // Create a file execution command for the FIFO queue
        char file_command[64];
        snprintf(file_command, sizeof(file_command), "FILE:%s", filename);
        
        // Use enhanced fifo_push with duplicate checking
        if (fifo_push_with_duplicate_check(file_command)) {
            // printf("Lua script %s added to execution queue\n", filename);
        } else {
            // Check if it was rejected due to duplicate or queue full
            if (fifo_get_count() >= 16) { // FIFO_BUFFER_SIZE is 16
                printf("Error: Execution queue is full, cannot add %s\n", filename);
            } else {
                printf("Script %s is already executing or queued - skipping\n", filename);
            }
        }
    } else {
        printf("Lua script %s not found\n", filename);
    }
    */
}

typedef struct {
    uint8_t modifier;
    int8_t  reserved;
    uint8_t keycode[6];
} keyboard_report_t;


// convert hid keycode to ascii and print
void process_kbd_report(uint8_t dev_addr, hid_keyboard_report_t const *report, uint16_t len)
{
    (void) len; //suppress unused variable warning
    setLEDStateActive();

    (void) dev_addr;
    static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

    // Check for CapsLock key press/release
    bool capslock_pressed_now = find_key_in_report(report, 0x39); // CapsLock keycode is 0x39
    bool capslock_pressed_prev = find_key_in_report(&prev_report, 0x39);
    
    if (capslock_pressed_now && !capslock_pressed_prev) {
        // CapsLock pressed
        // uart_puts(uart0, "META\r\n");
        meta = true;
    } else if (!capslock_pressed_now && capslock_pressed_prev) {
        // CapsLock released
        // uart_puts(uart0, "meta\r\n");
        meta = false;
    }

    if(!meta)
    {
        // Check all key positions and detect all keys released state
        bool current_has_keys = false;
        bool prev_had_keys = false;
        
        // Process all possible key positions (check both current and previous reports)
        for(uint8_t i=0; i<6; i++)
        {
            uint8_t current_keycode = report->keycode[i];
            uint8_t prev_keycode = prev_report.keycode[i];
            
            // Track if any keys are present in current or previous reports
            if (current_keycode != 0) current_has_keys = true;
            if (prev_keycode != 0) prev_had_keys = true;
            
            // Check for key press (key in current report but not in previous)
            /* if (current_keycode && !find_key_in_report(&prev_report, current_keycode))
            {
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                uint8_t ch = keycode2ascii[current_keycode][is_shift ? 1 : 0];

                if (ch) {
                    printf("%c", ch);
                } else {
                    printf("Key pressed: [Special] (keycode: 0x%02x)\n", current_keycode);
                }
            } */
        }


        // Create modified report without CapsLock for device output
        hid_keyboard_report_t* modified_report = (hid_keyboard_report_t*)report;

        if(USB_output_switch == 0) // USB出力の場合だけ、UART出力する
        {
            // First, try to send any buffered reports
            try_send_buffered_keyboard_reports();
            
            // Now try to send the current report
            if (tud_connected() && tud_hid_n_ready(0)) {
                // Send modified report (without CapsLock) to HID device
                bool success = tud_hid_n_keyboard_report(0, 1, modified_report->modifier, modified_report->keycode);
                if (!success) {
                    // printf("Warning: Failed to send keyboard report to USB host (interface busy), buffering for retry\n");
                    buffer_keyboard_report(modified_report);
                } else {
                    // Optional: Debug successful transmission
                    // printf("Keyboard report sent successfully\n");
                }
            } else {
                if (!tud_connected()) {
                    // printf("Warning: USB device not connected, buffering keyboard report\n");
                    buffer_keyboard_report(modified_report);
                } else {
                    // printf("Warning: USB keyboard interface not ready, buffering keyboard report\n");
                    buffer_keyboard_report(modified_report);
                }
            }
        }
        else
        {
            uint8_t base64_output[16];
            base64_output[0] = 'K';
            base64_output[1] = '0' + USB_output_switch;

            base64_encode((uint8_t*)modified_report, sizeof(hid_keyboard_report_t), (char*)&(base64_output[2]), sizeof(base64_output) - 1);
            base64_output[14] = '\n';
            base64_output[15] = 0;

            uart_puts(uart1, (char*)base64_output);
            /*for(int i = 0; i < sizeof(hid_keyboard_report_t); i++)
            {
                printf("%02X ", ((uint8_t*)modified_report)[i]);
            }
            printf("%s\n", base64_output);*/
            // Check if all keys are released after processing individual keys
            if (!current_has_keys && modified_report->modifier == 0x00)
            {
                //printf("All keys released\n");
                // Send all keys released message to UART1
                uart_puts(uart1, "0\n");
            }

        }


    }
    else
    {
        // If in META mode, handle key presses for Lua script execution
        for(uint8_t i=0; i<6; i++)
        {
            uint8_t keycode = report->keycode[i];
            if ( keycode )
            {
                if ( !find_key_in_report(&prev_report, keycode) )
                {
                    // Key pressed in META mode - check for corresponding Lua script
                    handle_meta_key(keycode);
                }
            }
        }
    }
    prev_report = *report;
}

void processed_mouse_report_print(uint8_t const * report, uint16_t len, mouse_report_t mouse_report)
{
    printf("M:");
    for(int i = 0; i < len; i++)
    {
        printf(" %02X", ((uint8_t*)report)[i]); 
    }
    printf("  ");

    printf("Buttons:%02X X:%-4d Y:%-4d W:%-2d P:%-2d\n",
            mouse_report.buttons,
            mouse_report.x, mouse_report.y,
            mouse_report.wheel, mouse_report.pan);
}

// send mouse report
void process_mouse_report(uint8_t instance, uint8_t const * report, uint16_t len)
{
    mouse_report_t mouse_report;
    //mouse_report_parser(&boot_mouse_report_info, (const uint8_t*)report, len, &mouse_report);
    if(interface_report_parser_info[instance].is_valid == false)
    {
        // No parser info available for this device
        printf("No parser info for device instance %u, skipping report processing\n", instance);
        return;
    }
    if(default_hid_protocol == HID_PROTOCOL_REPORT && interface_report_parser_info[instance].parser_info == NULL && len > 4)
    {
        if(tud_cdc_connected())
        {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "[Error] No parser info for device instance %u. Ignoring report processing.\r\n", instance);
            tud_cdc_write_str(buffer);
            
            tud_cdc_write_str("Mouse Report:");
            for(int i = 0; i < len; i++)
            {
                snprintf(buffer, sizeof(buffer), " %02X", ((uint8_t*)report)[i]);
                tud_cdc_write_str(buffer);
            }
            tud_cdc_write_str("\r\n");
        }

        //mouse_report_parser(&boot_mouse_report_info, report, len, &mouse_report);
        return;
    }
    else
    {
        // Use custom parser for this device
        mouse_report_parser(interface_report_parser_info[instance].parser_info, report, len, &mouse_report);
        // processed_mouse_report_print(report, len, mouse_report);
    }
    setLEDStateActive();

    if(USB_output_switch == 0) // USB出力の場合だけ、UART出力する
    {
        // First, try to send any buffered mouse reports
        try_send_buffered_mouse_reports();
        
        // Now try to send the current mouse report
        if (tud_connected() && tud_hid_n_ready(1)) {
            // send mouse report to host
            bool success = tud_hid_n_mouse_report(1, 2, mouse_report.buttons,
                mouse_report.x, mouse_report.y,
                mouse_report.wheel, mouse_report.pan);
            if (!success) {
                // printf("Warning: Failed to send mouse report to USB host (interface busy), buffering for retry\n");
                buffer_mouse_report(&mouse_report);
            } else {
                // Optional: Debug successful transmission
                // printf("Mouse report sent successfully\n");
            }
        } else {
            if (!tud_connected()) {
                // printf("Warning: USB device not connected, buffering mouse report\n");
                buffer_mouse_report(&mouse_report);
            } else {
                // printf("Warning: USB mouse interface not ready, buffering mouse report\n");
                buffer_mouse_report(&mouse_report);
            }
        }
    }
    else
    {
        // send mouse report to uart1
        uint8_t base64_output[16];
        base64_output[0] = 'M';
        base64_output[1] = '0' + USB_output_switch;
        base64_encode((uint8_t*)&mouse_report, sizeof(mouse_report_t), (char*)&(base64_output[2]), sizeof(base64_output) - 1);
        /* printf("UART: mouse report sent: %s %d\n", base64_output, sizeof(mouse_report_t)); */

        base64_output[14] = '\n';
        base64_output[15] = 0;
        
        uart_puts(uart1, (char*)base64_output);
        /* printf("%s", base64_output); */
    }
}

void usb_host_task(void)
{
    // Poll every 10ms to retry sending buffered keyboard reports
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    // Try to send any buffered keyboard reports
    if (kbd_buffer_count > 0) {
        try_send_buffered_keyboard_reports();
    }
    
    // Try to send any buffered mouse reports
    if (mouse_buffer_count > 0) {
        try_send_buffered_mouse_reports();
    }
}

//--------------------------------------------------------------------+
// USB Host HID Callbacks (these must be global)
//--------------------------------------------------------------------+

unsigned int lastReceivedLengthZero[CFG_TUH_HID] = { 0 };

// Invoked when device with hid interface is mounted
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
    (void)desc_report;
    (void)desc_len;
    lastReceivedLengthZero[instance] = 0;

    // Interface protocol (hid_interface_protocol_enum_t)
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    uint16_t vid, pid;
    tuh_vid_pid_get(dev_addr, &vid, &pid);

    interface_report_parser_info[instance].is_valid = true;
    interface_report_parser_info[instance].vid = vid;
    interface_report_parser_info[instance].pid = pid;
    interface_report_parser_info[instance].dev_addr = dev_addr;
    interface_report_parser_info[instance].instance = instance;

    // Check for mouse devices and try to find parser in defined_report_parser_info
    const char* device_type = "Unknown";
    bool has_custom_parser = false;
    
    if(itf_protocol == HID_ITF_PROTOCOL_MOUSE)
    {
        device_type = "Mouse";
        // First try to find a custom parser for this VID/PID
        void* custom_parser = find_device_parser(vid, pid);
        if (custom_parser != NULL) {
            interface_report_parser_info[instance].parser_info = custom_parser;
            has_custom_parser = true;
        }
        if(tud_cdc_connected())
        {
            char buffer[128];
            sprintf(buffer, "Mouse device mounted. VID: %04x, PID: %04x, Instance: %u, Custom Parser: %s\n",
                   vid, pid, instance, has_custom_parser ? "Yes" : "No");
            tud_cdc_write_str((char*)buffer);
        }
    }
    else if(itf_protocol == HID_ITF_PROTOCOL_KEYBOARD)
    {
        device_type = "Keyboard";
        
        // Register this keyboard device
        for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
            if (!keyboard_devices[i].connected) {
                keyboard_devices[i].dev_addr = dev_addr;
                keyboard_devices[i].instance = instance;
                keyboard_devices[i].is_keyboard = true;
                keyboard_devices[i].connected = true;
                keyboard_device_count++;
                printf("Registered keyboard device [%u:%u] (total: %d)\n", dev_addr, instance, keyboard_device_count);
                break;
            }
        }
        if(tud_cdc_connected() || true)
        {
            char buffer[128];
            sprintf(buffer, "Keyboard device mounted. VID: %04x, PID: %04x, Instance: %u\n",
                   vid, pid, instance);
            tud_cdc_write_str((char*)buffer);
        }
    }
    else if(itf_protocol == HID_ITF_PROTOCOL_NONE)
    {
        device_type = "Gamepad/Generic";
        if(tud_cdc_connected())
        {
            char buffer[128];
            sprintf(buffer, "Gamepad/Generic HID device mounted. VID: %04x, PID: %04x, Instance: %u\n",
                   vid, pid, instance);
            tud_cdc_write_str((char*)buffer);
        }
    }

    // Single consolidated log line per device
    printf("[%04x:%04x] %s on Interface%u %s%s\n", 
           vid, pid, device_type, instance,
           has_custom_parser ? "(Custom Parser) " : "",
           tuh_hid_set_protocol(dev_addr, instance, default_hid_protocol) ? "Ready" : "Ready (Protocol warning)");

    // Start receiving reports for all HID devices
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        printf("Error: cannot request initial report for interface %u\n", instance);
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    
    printf("[%u] HID Interface%u (%s) is unmounted - stopping all report requests\n", 
           dev_addr, instance, protocol_str[itf_protocol]);
    
    // Handle keyboard disconnection
    if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD) {
        // Remove this keyboard from our tracking
        for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
            if (keyboard_devices[i].connected && 
                keyboard_devices[i].dev_addr == dev_addr && 
                keyboard_devices[i].instance == instance) {
                keyboard_devices[i].connected = false;
                keyboard_devices[i].is_keyboard = false;
                keyboard_device_count--;
                printf("Unregistered keyboard device [%u:%u] (remaining: %d)\n", dev_addr, instance, keyboard_device_count);
                break;
            }
        }
    }
    
    // Handle gamepad disconnection
    if (itf_protocol == HID_ITF_PROTOCOL_NONE) {
        has_gamepad_key = false;
        gamepad_state_updated = true; // Trigger zero report send
        printf("Gamepad disconnected - clearing state\n");
    }

    interface_report_parser_info[instance].is_valid = false;
}


// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    // Check if device is still connected - len=0 often indicates disconnection
    if (len == 0)
    {
        printf("[%u] HID Interface%u: Zero-length report received %u times.\n", 
               dev_addr, instance, lastReceivedLengthZero[instance]);
        lastReceivedLengthZero[instance] ++;
        if(lastReceivedLengthZero[instance] >= 20)
        {
            printf("[%u] HID Interface%u: Consecutive zero-length reports, assuming device disconnected. Stopping report requests.\n", 
                   dev_addr, instance);
            return;
        }
        if ( !tuh_hid_receive_report(dev_addr, instance) )
        {
            printf("[%u] HID Interface%u: Error requesting next report - device may be disconnected\n", 
                dev_addr, instance);
        }
        return;
    }
    lastReceivedLengthZero[instance] = 0;

    // Check if device is still mounted
    if (!tuh_hid_mounted(dev_addr, instance))
    {
        printf("[%u] HID Interface%u: Device no longer mounted, stopping report requests\n", 
               dev_addr, instance);
        return;
    }

    switch(itf_protocol)
    {
        case HID_ITF_PROTOCOL_KEYBOARD:
            process_kbd_report(dev_addr, (hid_keyboard_report_t const*) report, len );
        break;

        case HID_ITF_PROTOCOL_MOUSE:
            process_mouse_report(instance, report, len);
        break;

        case HID_ITF_PROTOCOL_NONE:
        default:
            // Handle gamepad and other HID devices (usually protocol = None)
            // Try to parse as Samwa gamepad first
            {
                setLEDStateActive();
                parsed_gamepad_report_t parsed_report;
                if (parse_gamepad_report(report, len, &Samwa_400_JYP62U_gamepad_report_info, &parsed_report)) {
                    process_gamepad_report(dev_addr, instance, &parsed_report);
                } else {
                    // If parsing fails, show raw data
                    /* printf("[%u:%u] Generic HID report (len=%u): ", dev_addr, instance, len);
                    for(int i = 0; i < len; i++)
                    {
                        printf("%02X ", report[i]); 
                    }
                    printf("\n"); */
                }
            }
            sleep_ms(1); // Small delay to avoid overwhelming output
        break;
    }

    // Continue to request next report only if device is still connected and mounted
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        printf("[%u] HID Interface%u: Error requesting next report - device may be disconnected\n", 
               dev_addr, instance);
    }
}