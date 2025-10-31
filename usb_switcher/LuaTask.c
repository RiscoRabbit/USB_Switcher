#include "LuaTask.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"        // For UART1 communication
#include "fstask.h"
#include "USBtask.h"
#include "CDCCmd.h"
#include "GamepadReportParser.h"  // For gamepad control functions
#include "OLEDtask.h"             // For OLED display functions
#include "base64.h"               // For base64 encoding

/*
 * Lua Keyboard Sample Code Examples
 * 
 * Example 1: Press and release 'A' key with 10ms delay
 * --------------------------------------------------
 * keypress(0x04)   -- Press 'A' key (HID keycode 0x04)
 * sleep(10)        -- Wait 10 milliseconds
 * keyrelease(0x04) -- Release 'A' key
 * 
 * Example 2: Type "Hello" with delays between each character
 * ----------------------------------------------------------
 * keypress(0x0B)   -- H
 * sleep(50)
 * keyrelease(0x0B)
 * sleep(50)
 * keypress(0x08)   -- E
 * sleep(50)
 * keyrelease(0x08)
 * sleep(50)
 * keypress(0x0F)   -- L
 * sleep(50)
 * keyrelease(0x0F)
 * sleep(50)
 * keypress(0x0F)   -- L
 * sleep(50)
 * keyrelease(0x0F)
 * sleep(50)
 * keypress(0x12)   -- O
 * sleep(50)
 * keyrelease(0x12)
 * 
 * Example 3: Ctrl+A (Select All) combination
 * -------------------------------------------
 * keypress(0xE0)   -- Left Ctrl
 * keypress(0x04)   -- A
 * sleep(100)       -- Hold combination for 100ms
 * keyrelease(0x04) -- Release A first
 * keyrelease(0xE0) -- Release Ctrl
 * 
 * Example 4: Multiple keys simultaneously
 * ---------------------------------------
 * keypress(0x04)   -- A
 * keypress(0x05)   -- B
 * keypress(0x06)   -- C
 * sleep(500)       -- Hold all keys for 500ms
 * keyrelease(0x04) -- Release A
 * keyrelease(0x05) -- Release B
 * keyrelease(0x06) -- Release C
 * 
 * Example 5: Type string with automatic character mapping
 * -------------------------------------------------------
 * type("Hello", 10)     -- Type "Hello" with 10ms delays
 * type("Hello!", 50)    -- Type "Hello!" with 50ms delays (! requires shift)
 * type("User@123", 100) -- Type "User@123" with 100ms delays (@ requires shift)
 * 
 * Gamepad Functions Sample Code Examples
 * ======================================
 * 
 * Example 6: Check if gamepad button is pressed
 * ----------------------------------------------
 * if gamepad_get_button("1") then
 *     print("Button 1 is pressed")
 * end
 * 
 * Example 7: Get gamepad hat switch state
 * ----------------------------------------
 * local hat = gamepad_get_hat()
 * if hat == 0 then
 *     print("Hat: Center")
 * elseif hat == 1 then
 *     print("Hat: Up")
 * elseif hat == 3 then
 *     print("Hat: Right")
 * elseif hat == 5 then
 *     print("Hat: Down")
 * elseif hat == 7 then
 *     print("Hat: Left")
 * end
 * 
 * Example 8: Get individual analog stick values
 * ----------------------------------------------
 * local x = gamepad_get_x()    -- Left stick X (-127 to 127)
 * local y = gamepad_get_y()    -- Left stick Y (-127 to 127)
 * local z = gamepad_get_z()    -- Right stick X or trigger (-127 to 127)
 * local rz = gamepad_get_rz()  -- Right stick Y or trigger (-127 to 127)
 * 
 * print("Left stick: " .. x .. ", " .. y)
 * print("Right stick/triggers: " .. z .. ", " .. rz)
 * 
 * Example 9: Get all analog values at once
 * -----------------------------------------
 * local analog = gamepad_get_analog()
 * print("X: " .. analog.x .. ", Y: " .. analog.y)
 * print("Z: " .. analog.z .. ", RZ: " .. analog.rz)
 * 
 * Example 10: Gamepad-based keyboard control
 * -------------------------------------------
 * -- Use left stick for WASD movement
 * local x = gamepad_get_x()
 * local y = gamepad_get_y()
 * 
 * if x < -64 then
 *     keypress(0x04)  -- A (left)
 * elseif x > 64 then
 *     keypress(0x07)  -- D (right)
 * end
 * 
 * if y < -64 then
 *     keypress(0x1A)  -- W (up)
 * elseif y > 64 then
 *     keypress(0x16)  -- S (down)
 * end
 * 
 * Gamepad Output Control Examples
 * ===============================
 * 
 * Example 11: Press and release gamepad buttons (analog sticks inherit real gamepad values)
 * ------------------------------------------------------------------------------
 * gamepad_press_button(2)   -- Press button 2 (analog sticks use real gamepad input)
 * sleep(100)                -- Hold for 100ms
 * gamepad_release_button(2) -- Release button 2 (analog sticks still use real gamepad input)
 * 
 * Example 12: Auto-fire gamepad button (analog sticks follow real gamepad)
 * -------------------------------------------------------------------------
 * while gamepad_get_button("1") do  -- While button 1 is held
 *     gamepad_press_button(2)       -- Press button 2 (inherits analog values)
 *     sleep(10)                     -- Brief press
 *     gamepad_release_button(2)     -- Release button 2 (inherits analog values)
 *     sleep(90)                     -- Wait (100ms total cycle)
 * end
 * 
 * Example 13: Set analog stick positions
 * ---------------------------------------
 * gamepad_set_analog("x", 127)     -- Full right on X axis
 * gamepad_set_analog("y", -127)    -- Full up on Y axis
 * sleep(1000)                      -- Hold position for 1 second
 * gamepad_set_analog("x", 0)       -- Return to center
 * gamepad_set_analog("y", 0)
 * 
 * Example 14: Set hat switch direction
 * ------------------------------------
 * gamepad_set_hat(1)  -- Up
 * sleep(500)
 * gamepad_set_hat(3)  -- Right
 * sleep(500)
 * gamepad_set_hat(5)  -- Down
 * sleep(500)
 * gamepad_set_hat(7)  -- Left
 * sleep(500)
 * gamepad_set_hat(0)  -- Center
 * 
 * Example 15: Reset gamepad controls (preserve analog stick positions)
 * -------------------------------------------------------------------
 * gamepad_reset()  -- Clear buttons and hat, but keep analog sticks at real gamepad positions
 * 
 * Common HID Keycodes:
 * A=0x04, B=0x05, C=0x06, D=0x07, E=0x08, F=0x09, G=0x0A, H=0x0B
 * I=0x0C, J=0x0D, K=0x0E, L=0x0F, M=0x10, N=0x11, O=0x12, P=0x13
 * Q=0x14, R=0x15, S=0x16, T=0x17, U=0x18, V=0x19, W=0x1A, X=0x1B
 * Y=0x1C, Z=0x1D
 * 1=0x1E, 2=0x1F, 3=0x20, 4=0x21, 5=0x22, 6=0x23, 7=0x24, 8=0x25, 9=0x26, 0=0x27
 * ENTER=0x28, ESCAPE=0x29, BACKSPACE=0x2A, TAB=0x2B, SPACE=0x2C
 * LEFT_CTRL=0xE0, LEFT_SHIFT=0xE1, LEFT_ALT=0xE2, LEFT_GUI=0xE3
 * 
 * Hat Switch Values:
 * 0=Center, 1=Up, 2=Up-Right, 3=Right, 4=Down-Right, 5=Down, 6=Down-Left, 7=Left, 8=Up-Left
 */

// Override Lua output functions to use CDC (undef first to avoid warnings)
#undef lua_writestring
#undef lua_writeline
#define lua_writestring(s,l)   cdc_write_string((s), (l))
#define lua_writeline()        cdc_write_line()

uint8_t USB_output_switch = 0; // USB出力切替 0:USB1, 1:UART経由

//--------------------------------------------------------------------+
// CDC Output Functions for Lua
//--------------------------------------------------------------------+

// Buffer for CDC output
static char cdc_output_buffer[256];
static size_t cdc_buffer_pos = 0;

// Custom CDC write function
void cdc_write_char(char c) {
    if (tud_cdc_connected()) {
        // If buffer is full or we hit a newline, flush it
        if (cdc_buffer_pos >= sizeof(cdc_output_buffer) - 1 || c == '\n') {
            if (cdc_buffer_pos > 0) {
                tud_cdc_write(cdc_output_buffer, cdc_buffer_pos);
                tud_cdc_write_flush();
                cdc_buffer_pos = 0;
            }
            if (c == '\n') {
                tud_cdc_write("\r\n", 2);  // Convert LF to CRLF for terminal compatibility
                tud_cdc_write_flush();
                return;
            }
        }
        
        // Add character to buffer
        cdc_output_buffer[cdc_buffer_pos++] = c;
    }
    // Also output to UART as fallback
    putchar_raw(c);
}

// Custom CDC write string function  
void cdc_write_string(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        cdc_write_char(str[i]);
    }
}

// Custom CDC write line function
void cdc_write_line(void) {
    cdc_write_char('\n');
}

//--------------------------------------------------------------------+
// Lua Custom Functions
//--------------------------------------------------------------------+

// Global keyboard state to track pressed keys
static uint8_t lua_keyboard_modifier = 0;
static uint8_t lua_keyboard_keycodes[6] = {0}; // HID allows up to 6 simultaneous keys
static bool lua_keyboard_dirty = false; // Flag to indicate state change

// Global mouse state for Lua control
static uint8_t lua_mouse_buttons = 0;
static int8_t lua_mouse_x = 0;
static int8_t lua_mouse_y = 0;
static int8_t lua_mouse_wheel = 0;
static int8_t lua_mouse_pan = 0;
static bool lua_mouse_dirty = false; // Flag to indicate state change

// Global gamepad output state for Lua control
static int8_t lua_gamepad_x = 0;
static int8_t lua_gamepad_y = 0;
static int8_t lua_gamepad_z = 0;
static int8_t lua_gamepad_rz = 0;
static uint8_t lua_gamepad_hat = 0;
static uint16_t lua_gamepad_buttons = 0;
static bool lua_gamepad_dirty = false; // Flag to indicate state change
static bool lua_gamepad_active = false; // Flag to indicate Lua gamepad control is active
static bool lua_gamepad_force_zero = false; // Flag to force sending zero report once

// Keyboard language settings
typedef enum {
    KEYBOARD_LANG_US,  // US English 104-key
    KEYBOARD_LANG_JA   // Japanese 109-key
} keyboard_language_t;

static keyboard_language_t current_keyboard_lang = KEYBOARD_LANG_US; // Default to US English

// Character to HID keycode mapping structure
typedef struct {
    char character;
    uint8_t keycode;
    bool needs_shift;
} char_keycode_map_t;

// US English keyboard layout mapping
static const char_keycode_map_t char_to_keycode_us[] = {
    // Lowercase letters (a-z)
    {'a', 0x04, false}, {'b', 0x05, false}, {'c', 0x06, false}, {'d', 0x07, false},
    {'e', 0x08, false}, {'f', 0x09, false}, {'g', 0x0A, false}, {'h', 0x0B, false},
    {'i', 0x0C, false}, {'j', 0x0D, false}, {'k', 0x0E, false}, {'l', 0x0F, false},
    {'m', 0x10, false}, {'n', 0x11, false}, {'o', 0x12, false}, {'p', 0x13, false},
    {'q', 0x14, false}, {'r', 0x15, false}, {'s', 0x16, false}, {'t', 0x17, false},
    {'u', 0x18, false}, {'v', 0x19, false}, {'w', 0x1A, false}, {'x', 0x1B, false},
    {'y', 0x1C, false}, {'z', 0x1D, false},
    
    // Uppercase letters (A-Z) - require shift
    {'A', 0x04, true}, {'B', 0x05, true}, {'C', 0x06, true}, {'D', 0x07, true},
    {'E', 0x08, true}, {'F', 0x09, true}, {'G', 0x0A, true}, {'H', 0x0B, true},
    {'I', 0x0C, true}, {'J', 0x0D, true}, {'K', 0x0E, true}, {'L', 0x0F, true},
    {'M', 0x10, true}, {'N', 0x11, true}, {'O', 0x12, true}, {'P', 0x13, true},
    {'Q', 0x14, true}, {'R', 0x15, true}, {'S', 0x16, true}, {'T', 0x17, true},
    {'U', 0x18, true}, {'V', 0x19, true}, {'W', 0x1A, true}, {'X', 0x1B, true},
    {'Y', 0x1C, true}, {'Z', 0x1D, true},
    
    // Numbers (0-9)
    {'1', 0x1E, false}, {'2', 0x1F, false}, {'3', 0x20, false}, {'4', 0x21, false},
    {'5', 0x22, false}, {'6', 0x23, false}, {'7', 0x24, false}, {'8', 0x25, false},
    {'9', 0x26, false}, {'0', 0x27, false},
    
    // Special characters that require shift (US layout)
    {'!', 0x1E, true},  // Shift + 1
    {'@', 0x1F, true},  // Shift + 2
    {'#', 0x20, true},  // Shift + 3
    {'$', 0x21, true},  // Shift + 4
    {'%', 0x22, true},  // Shift + 5
    {'^', 0x23, true},  // Shift + 6
    {'&', 0x24, true},  // Shift + 7
    {'*', 0x25, true},  // Shift + 8
    {'(', 0x26, true},  // Shift + 9
    {')', 0x27, true},  // Shift + 0
    
    // Common punctuation and symbols (US layout)
    {' ', 0x2C, false}, // Space
    {'\t', 0x2B, false}, // Tab
    {'\n', 0x28, false}, // Enter
    {'-', 0x2D, false}, // Hyphen
    {'=', 0x2E, false}, // Equal
    {'[', 0x2F, false}, // Left bracket
    {']', 0x30, false}, // Right bracket
    {'\\', 0x31, false}, // Backslash
    {';', 0x33, false}, // Semicolon
    {'\'', 0x34, false}, // Apostrophe
    {'`', 0x35, false}, // Grave accent
    {',', 0x36, false}, // Comma
    {'.', 0x37, false}, // Period
    {'/', 0x38, false}, // Slash
    
    // Shifted punctuation (US layout)
    {'_', 0x2D, true},  // Shift + Hyphen
    {'+', 0x2E, true},  // Shift + Equal
    {'{', 0x2F, true},  // Shift + Left bracket
    {'}', 0x30, true},  // Shift + Right bracket
    {'|', 0x31, true},  // Shift + Backslash
    {':', 0x33, true},  // Shift + Semicolon
    {'"', 0x34, true},  // Shift + Apostrophe
    {'~', 0x35, true},  // Shift + Grave accent
    {'<', 0x36, true},  // Shift + Comma
    {'>', 0x37, true},  // Shift + Period
    {'?', 0x38, true},  // Shift + Slash
};

// Japanese 109-key keyboard layout mapping
static const char_keycode_map_t char_to_keycode_ja[] = {
    // Lowercase letters (a-z) - same as US
    {'a', 0x04, false}, {'b', 0x05, false}, {'c', 0x06, false}, {'d', 0x07, false},
    {'e', 0x08, false}, {'f', 0x09, false}, {'g', 0x0A, false}, {'h', 0x0B, false},
    {'i', 0x0C, false}, {'j', 0x0D, false}, {'k', 0x0E, false}, {'l', 0x0F, false},
    {'m', 0x10, false}, {'n', 0x11, false}, {'o', 0x12, false}, {'p', 0x13, false},
    {'q', 0x14, false}, {'r', 0x15, false}, {'s', 0x16, false}, {'t', 0x17, false},
    {'u', 0x18, false}, {'v', 0x19, false}, {'w', 0x1A, false}, {'x', 0x1B, false},
    {'y', 0x1C, false}, {'z', 0x1D, false},
    
    // Uppercase letters (A-Z) - same as US
    {'A', 0x04, true}, {'B', 0x05, true}, {'C', 0x06, true}, {'D', 0x07, true},
    {'E', 0x08, true}, {'F', 0x09, true}, {'G', 0x0A, true}, {'H', 0x0B, true},
    {'I', 0x0C, true}, {'J', 0x0D, true}, {'K', 0x0E, true}, {'L', 0x0F, true},
    {'M', 0x10, true}, {'N', 0x11, true}, {'O', 0x12, true}, {'P', 0x13, true},
    {'Q', 0x14, true}, {'R', 0x15, true}, {'S', 0x16, true}, {'T', 0x17, true},
    {'U', 0x18, true}, {'V', 0x19, true}, {'W', 0x1A, true}, {'X', 0x1B, true},
    {'Y', 0x1C, true}, {'Z', 0x1D, true},
    
    // Numbers (0-9) - same as US
    {'1', 0x1E, false}, {'2', 0x1F, false}, {'3', 0x20, false}, {'4', 0x21, false},
    {'5', 0x22, false}, {'6', 0x23, false}, {'7', 0x24, false}, {'8', 0x25, false},
    {'9', 0x26, false}, {'0', 0x27, false},
    
    // Special characters for Japanese layout (different from US)
    {'!', 0x1E, true},  // Shift + 1
    {'"', 0x1F, true},  // Shift + 2 (Japanese: " instead of @)
    {'#', 0x20, true},  // Shift + 3
    {'$', 0x21, true},  // Shift + 4
    {'%', 0x22, true},  // Shift + 5
    {'&', 0x23, true},  // Shift + 6 (Japanese: & instead of ^)
    {'\'', 0x24, true}, // Shift + 7 (Japanese: ' instead of &)
    {'(', 0x25, true},  // Shift + 8 (Japanese: ( instead of *)
    {')', 0x26, true},  // Shift + 9 (Japanese: ) instead of ()
    {' ', 0x27, true},  // Shift + 0 (Japanese: space character)
    
    // Common punctuation and symbols (Japanese layout)
    {' ', 0x2C, false}, // Space
    {'\t', 0x2B, false}, // Tab
    {'\n', 0x28, false}, // Enter
    {'-', 0x2D, false}, // Hyphen (minus)
    {'^', 0x2E, false}, // Equal key position (Japanese: ^ instead of =)
    {'@', 0x2F, false}, // Left bracket key position (Japanese: @ instead of [)
    {'[', 0x30, false}, // Right bracket key position (Japanese: [ instead of ])
    {';', 0x31, false}, // Backslash key position (Japanese: ; instead of \)
    {'+', 0x33, false}, // Semicolon key position (Japanese: + instead of ;)
    {':', 0x34, false}, // Apostrophe key position (Japanese: : instead of ')
    {'*', 0x35, false}, // Grave accent key position (Japanese: * instead of `)
    {',', 0x36, false}, // Comma - same
    {'.', 0x37, false}, // Period - same
    {'/', 0x38, false}, // Slash - same
    
    // Shifted punctuation for Japanese layout
    {'=', 0x2D, true},  // Shift + Hyphen (Japanese: = instead of _)
    {'~', 0x2E, true},  // Shift + ^ key (Japanese: ~ instead of +)
    {'`', 0x2F, true},  // Shift + @ key (Japanese: ` instead of {)
    {'{', 0x30, true},  // Shift + [ key (Japanese: { instead of })
    {'*', 0x31, true},  // Shift + ; key (Japanese: * instead of |)
    {'*', 0x33, true},  // Shift + + key 
    {'*', 0x34, true},  // Shift + : key
    {'+', 0x35, true},  // Shift + * key (Japanese: + instead of ~)
    {'<', 0x36, true},  // Shift + Comma - same
    {'>', 0x37, true},  // Shift + Period - same  
    {'?', 0x38, true},  // Shift + Slash - same
    
    // Additional Japanese-specific keys
    {'\\', 0x89, false}, // Yen/Backslash key (0x89 = International3)
    {'|', 0x89, true},   // Shift + Yen key
    {']', 0x8A, false},  // Right bracket (0x8A = International1) 
    {'}', 0x8A, true},   // Shift + Right bracket
    {'_', 0x87, false},  // Underscore (0x87 = International4)
};

// Helper function to find keycode in current pressed keys
static int find_keycode_index(uint8_t keycode) {
    for (int i = 0; i < 6; i++) {
        if (lua_keyboard_keycodes[i] == keycode) {
            return i;
        }
    }
    return -1;
}

// Helper function to find empty slot in keycode array
static int find_empty_slot(void) {
    for (int i = 0; i < 6; i++) {
        if (lua_keyboard_keycodes[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Helper function to get current keyboard layout mapping
static const char_keycode_map_t* get_current_keymap(size_t* map_size) {
    if (current_keyboard_lang == KEYBOARD_LANG_JA) {
        *map_size = sizeof(char_to_keycode_ja) / sizeof(char_keycode_map_t);
        return char_to_keycode_ja;
    } else {
        *map_size = sizeof(char_to_keycode_us) / sizeof(char_keycode_map_t);
        return char_to_keycode_us;
    }
}

// Helper function to find keycode for a character
static bool find_char_keycode(char c, uint8_t* keycode, bool* needs_shift) {
    size_t map_size;
    const char_keycode_map_t* keymap = get_current_keymap(&map_size);
    
    for (size_t i = 0; i < map_size; i++) {
        if (keymap[i].character == c) {
            *keycode = keymap[i].keycode;
            *needs_shift = keymap[i].needs_shift;
            return true;
        }
    }
    return false; // Character not found
}

// Forward declarations
static void send_gamepad_report(void);

// Helper function to send current keyboard state
static void send_keyboard_report(void) {
    if (USB_output_switch == 0) {
        // USB output mode - send to USB device
        while (tud_hid_n_ready(0) == 0) { // Check if HID interface 0 (keyboard) is ready
            vTaskDelay(pdMS_TO_TICKS(1)); // 1ms wait time
        }
        if(tud_hid_n_ready(0)) {
            tud_hid_n_keyboard_report(0, 1, lua_keyboard_modifier, lua_keyboard_keycodes);
            lua_keyboard_dirty = false;
        } else {
            printf("Warning: HID interface not ready\n");
        }
    } else {
        // UART output mode - send to UART1
        // Create keyboard report structure (same as USBHostTask.c)
        typedef struct {
            uint8_t modifier;
            uint8_t reserved;
            uint8_t keycode[6];
        } __attribute__((packed)) hid_keyboard_report_t;
        
        hid_keyboard_report_t keyboard_report;
        keyboard_report.modifier = lua_keyboard_modifier;
        keyboard_report.reserved = 0;
        memcpy(keyboard_report.keycode, lua_keyboard_keycodes, 6);
        
        // Encode to base64 and send to UART1 (same format as USBHostTask.c)
        uint8_t base64_output[16];
        base64_output[0] = 'K';
        base64_output[1] = '0' + USB_output_switch;
        
        base64_encode((uint8_t*)&keyboard_report, sizeof(hid_keyboard_report_t), (char*)&(base64_output[2]), sizeof(base64_output) - 3);
        base64_output[14] = '\n';
        base64_output[15] = 0;
        
        uart_puts(uart1, (char*)base64_output);
        lua_keyboard_dirty = false;
    }
}

// Helper function to send current mouse state
static void send_mouse_report(void) {
    if (USB_output_switch == 0) {
        // USB output mode - send to USB device
        while (tud_hid_n_ready(1) == 0) { // Check if HID interface 1 (mouse) is ready
            vTaskDelay(pdMS_TO_TICKS(1)); // 1ms wait time
        }
        if(tud_hid_n_ready(1)) {
            tud_hid_n_mouse_report(1, 2, lua_mouse_buttons, lua_mouse_x, lua_mouse_y, lua_mouse_wheel, lua_mouse_pan);
            lua_mouse_dirty = false;
        } else {
            printf("Warning: Mouse HID interface not ready\n");
        }
    } else {
        // UART output mode - send to UART1 (same format as USBHostTask.c)
        typedef struct {
            uint8_t buttons;
            int8_t x;
            int8_t y;
            int8_t wheel;
            int8_t pan;
        } __attribute__((packed)) mouse_report_t;
        
        mouse_report_t mouse_report;
        mouse_report.buttons = lua_mouse_buttons;
        mouse_report.x = lua_mouse_x;
        mouse_report.y = lua_mouse_y;
        mouse_report.wheel = lua_mouse_wheel;
        mouse_report.pan = lua_mouse_pan;
        
        // Encode to base64 and send to UART1
        uint8_t base64_output[16];
        base64_output[0] = 'M';
        base64_output[1] = '0' + USB_output_switch;
        
        base64_encode((uint8_t*)&mouse_report, sizeof(mouse_report_t), (char*)&(base64_output[2]), sizeof(base64_output) - 3);
        base64_output[14] = '\n';
        base64_output[15] = 0;
        
        uart_puts(uart1, (char*)base64_output);
        lua_mouse_dirty = false;
    }
}

// Helper function to type a single character with timing
static void type_character(char c, int delay_ms) {
    uint8_t keycode;
    bool needs_shift;
    
    if (!find_char_keycode(c, &keycode, &needs_shift)) {
        // Character not found in mapping table
        printf("Warning: Unknown character '%c' (0x%02X) - skipping\n", c, (unsigned char)c);
        return;
    }
    
    // Find empty slot for the keycode
    int slot = find_empty_slot();
    if (slot < 0) {
        printf("Warning: Cannot type character - maximum simultaneous keys reached\n");
        return;
    }
    
    // Set up modifier and keycode simultaneously for proper shift handling
    uint8_t original_modifier = lua_keyboard_modifier;
    
    if (needs_shift) {
        lua_keyboard_modifier |= 0x02; // Left Shift (bit 1)
    }
    
    // Press the key (with shift if needed)
    lua_keyboard_keycodes[slot] = keycode;
    lua_keyboard_dirty = true;
    send_keyboard_report();
    
    // Add small delay to ensure the key press is registered
    vTaskDelay(pdMS_TO_TICKS(5)); // 10ms minimum press time
    
    // Hold for specified duration
    if (delay_ms > 5) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms - 5));
    }
    
    // Release the key first, then modifier
    lua_keyboard_keycodes[slot] = 0;
    lua_keyboard_dirty = true;
    send_keyboard_report();
    
    // Add small delay between key release and modifier release
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // Restore original modifier state
    lua_keyboard_modifier = original_modifier;
    send_keyboard_report();
    
    // Wait before next character
    if (delay_ms > 5) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms - 5));
    }
}

// Custom sleep function for Lua (sleep for specified milliseconds)
int lua_sleep(lua_State *L) {
    int ms = (int)luaL_checknumber(L, 1);  // Get milliseconds from Lua
    vTaskDelay(pdMS_TO_TICKS(ms));         // FreeRTOS delay
    return 0;  // No return values
}

// Custom switch function for Lua (switch USB output)
int lua_switch(lua_State *L) {
    USB_output_switch = (int)luaL_checknumber(L, 1);  // Get switching info from Lua
    return 0;  // No return values
}

// Lua function to press a keyboard key
int lua_keypress(lua_State *L) {
    int keycode = (int)luaL_checknumber(L, 1);  // Get keycode from Lua
    
    // Validate keycode range (HID keycodes are typically 0-255)
    if (keycode < 0 || keycode > 255) {
        luaL_error(L, "Invalid keycode: %d (must be 0-255)", keycode);
        return 0;
    }
    
    // Check if key is already pressed
    if (find_keycode_index((uint8_t)keycode) >= 0) {
        // Key already pressed, just return
        return 0;
    }
    
    // Find empty slot to add the key
    int slot = find_empty_slot();
    if (slot < 0) {
        luaL_error(L, "Cannot press more keys: maximum 6 simultaneous keys reached");
        return 0;
    }
    
    // Add keycode to pressed keys
    lua_keyboard_keycodes[slot] = (uint8_t)keycode;
    lua_keyboard_dirty = true;
    
    // Send the keyboard report
    send_keyboard_report();
    return 0;  // No return values
}

// Lua function to release a keyboard key
int lua_keyrelease(lua_State *L) {
    int keycode = (int)luaL_checknumber(L, 1);  // Get keycode from Lua
    
    // Validate keycode range
    if (keycode < 0 || keycode > 255) {
        luaL_error(L, "Invalid keycode: %d (must be 0-255)", keycode);
        return 0;
    }
    
    // Find the key in currently pressed keys
    int index = find_keycode_index((uint8_t)keycode);
    if (index < 0) {
        // Key not currently pressed, just return
        return 0;
    }
    
    // Remove keycode from pressed keys
    lua_keyboard_keycodes[index] = 0;
    lua_keyboard_dirty = true;
    
    // Send the keyboard report
    send_keyboard_report();
    return 0;  // No return values
}

// Lua function to set keyboard language
int lua_set_language(lua_State *L) {
    const char* lang = luaL_checkstring(L, 1);
    
    if (strcmp(lang, "us") == 0 || strcmp(lang, "en") == 0) {
        current_keyboard_lang = KEYBOARD_LANG_US;
        printf("Keyboard language set to US English (104-key)\n");
        
        if (tud_cdc_connected()) {
            tud_cdc_write_str("Keyboard language: US English (104-key)\r\n");
            tud_cdc_write_flush();
        }
    } else if (strcmp(lang, "ja") == 0 || strcmp(lang, "jp") == 0) {
        current_keyboard_lang = KEYBOARD_LANG_JA;
        printf("Keyboard language set to Japanese (109-key)\n");
        
        if (tud_cdc_connected()) {
            tud_cdc_write_str("Keyboard language: Japanese (109-key)\r\n");
            tud_cdc_write_flush();
        }
    } else {
        luaL_error(L, "Invalid language: %s (supported: us, en, ja, jp)", lang);
        return 0;
    }
    
    return 0; // No return values
}

// Lua function to get current keyboard language
int lua_get_language(lua_State *L) {
    const char* lang_str;
    
    if (current_keyboard_lang == KEYBOARD_LANG_JA) {
        lang_str = "ja";
    } else {
        lang_str = "us";
    }
    
    lua_pushstring(L, lang_str);
    return 1; // Return language string
}

// Lua function to type a string with timing
int lua_type_text(lua_State *L) {
    // Get string and delay from Lua
    const char* text = luaL_checkstring(L, 1);
    int delay_ms = (int)luaL_checknumber(L, 2);
    
    // Validate delay
    if (delay_ms < 0 || delay_ms > 10000) {
        luaL_error(L, "Invalid delay: %d (must be 0-10000 ms)", delay_ms);
        return 0;
    }
    
    // Type each character in the string
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        type_character(c, delay_ms);
        
        // Check for special escape sequences
        if (c == '\\' && i + 1 < len) {
            char next_char = text[i + 1];
            switch (next_char) {
                case 'n':
                    // Already handled by \n in mapping table
                    break;
                case 't':
                    // Already handled by \t in mapping table  
                    break;
                default:
                    // Unknown escape sequence, continue normally
                    break;
            }
        }
    }
    
    return 0;  // No return values
}

// Lua function to set various parameters
int lua_set(lua_State *L) {
    const char *param_name = luaL_checkstring(L, 1);  // Get parameter name from Lua
    int value = (int)luaL_checknumber(L, 2);          // Get value from Lua
    
    // Check parameter name and call appropriate function
    if (strcmp(param_name, "MetaKeyCode") == 0) {
        // Validate keycode range (HID keycodes are typically 0-255)
        if (value < 0 || value > 255) {
            luaL_error(L, "Invalid MetaKeyCode: %d (must be 0-255)", value);
            return 0;
        }
        setMetaKeyCode((uint8_t)value);
    } else {
        luaL_error(L, "Unknown parameter: %s", param_name);
        return 0;
    }
    
    return 0;  // No return values
}

// Lua function to move mouse
int lua_mouse_move(lua_State *L) {
    int x = (int)luaL_checknumber(L, 1);  // Get X movement from Lua
    int y = (int)luaL_checknumber(L, 2);  // Get Y movement from Lua
    
    // Validate movement range (-127 to 127)
    if (x < -127 || x > 127) {
        luaL_error(L, "Invalid X movement: %d (must be -127 to 127)", x);
        return 0;
    }
    if (y < -127 || y > 127) {
        luaL_error(L, "Invalid Y movement: %d (must be -127 to 127)", y);
        return 0;
    }
    
    lua_mouse_x = (int8_t)x;
    lua_mouse_y = (int8_t)y;
    lua_mouse_dirty = true;
    
    // Send the mouse report
    send_mouse_report();
    
    // Reset movement values after sending
    lua_mouse_x = 0;
    lua_mouse_y = 0;
    
    return 0;  // No return values
}

// Lua function to press mouse button
int lua_mouse_press(lua_State *L) {
    int button = (int)luaL_checknumber(L, 1);  // Get button number from Lua (1=left, 2=right, 3=middle)
    
    // Validate button number (1-8)
    if (button < 1 || button > 8) {
        luaL_error(L, "Invalid button number: %d (must be 1-8)", button);
        return 0;
    }
    
    // Set button bit
    uint8_t button_mask = 1 << (button - 1);
    lua_mouse_buttons |= button_mask;
    lua_mouse_dirty = true;
    
    // Send the mouse report
    send_mouse_report();
    return 0;  // No return values
}

// Lua function to release mouse button
int lua_mouse_release(lua_State *L) {
    int button = (int)luaL_checknumber(L, 1);  // Get button number from Lua
    
    // Validate button number (1-8)
    if (button < 1 || button > 8) {
        luaL_error(L, "Invalid button number: %d (must be 1-8)", button);
        return 0;
    }
    
    // Clear button bit
    uint8_t button_mask = 1 << (button - 1);
    lua_mouse_buttons &= ~button_mask;
    lua_mouse_dirty = true;
    
    // Send the mouse report
    send_mouse_report();
    return 0;  // No return values
}

// Lua function to scroll mouse wheel
int lua_mouse_scroll(lua_State *L) {
    int wheel = (int)luaL_checknumber(L, 1);  // Get wheel movement from Lua
    
    // Validate wheel range (-127 to 127)
    if (wheel < -127 || wheel > 127) {
        luaL_error(L, "Invalid wheel movement: %d (must be -127 to 127)", wheel);
        return 0;
    }
    
    lua_mouse_wheel = (int8_t)wheel;
    lua_mouse_dirty = true;
    
    // Send the mouse report
    send_mouse_report();
    
    // Reset wheel value after sending
    lua_mouse_wheel = 0;
    
    return 0;  // No return values
}

// Lua function to check if specific gamepad button is pressed
int lua_gamepad_get_button(lua_State *L) {
    const char *button_str = luaL_checkstring(L, 1);  // Get button string from Lua
    
    // Parse button number from string (e.g., "1", "2", etc.)
    int button_num = atoi(button_str);
    
    // Validate button number (1-16)
    if (button_num < 1 || button_num > 16) {
        luaL_error(L, "Invalid button number: %d (must be 1-16)", button_num);
        return 0;
    }
    
    // Check if button is pressed
    uint16_t button_mask = 1 << (button_num - 1);
    bool is_pressed = (current_gamepad_state.buttons & button_mask) != 0;
    
    lua_pushboolean(L, is_pressed);
    return 1;  // Return boolean result
}

// Lua function to get gamepad hat switch state
int lua_gamepad_get_hat(lua_State *L) {
    // Get hat value (0-15)
    uint8_t hat_value = current_gamepad_state.hat;
    
    lua_pushinteger(L, hat_value);
    return 1;  // Return hat value (0-15)
}

// Lua function to get gamepad analog stick X axis
int lua_gamepad_get_x(lua_State *L) {
    // Get X axis value (-127 to 127)
    int8_t x_value = current_gamepad_state.x;
    
    lua_pushinteger(L, x_value);
    return 1;  // Return X axis value
}

// Lua function to get gamepad analog stick Y axis
int lua_gamepad_get_y(lua_State *L) {
    // Get Y axis value (-127 to 127)
    int8_t y_value = current_gamepad_state.y;
    
    lua_pushinteger(L, y_value);
    return 1;  // Return Y axis value
}

// Lua function to get gamepad Z axis (right stick X or trigger)
int lua_gamepad_get_z(lua_State *L) {
    // Get Z axis value (-127 to 127)
    int8_t z_value = current_gamepad_state.z;
    
    lua_pushinteger(L, z_value);
    return 1;  // Return Z axis value
}

// Lua function to get gamepad RZ axis (right stick Y or trigger)
int lua_gamepad_get_rz(lua_State *L) {
    // Get RZ axis value (-127 to 127)
    int8_t rz_value = current_gamepad_state.rz;
    
    lua_pushinteger(L, rz_value);
    return 1;  // Return RZ axis value
}

// Lua function to get all gamepad analog values at once
int lua_gamepad_get_analog(lua_State *L) {
    // Return a table with all analog values
    lua_newtable(L);
    
    // Set X value
    lua_pushstring(L, "x");
    lua_pushinteger(L, current_gamepad_state.x);
    lua_settable(L, -3);
    
    // Set Y value
    lua_pushstring(L, "y");
    lua_pushinteger(L, current_gamepad_state.y);
    lua_settable(L, -3);
    
    // Set Z value
    lua_pushstring(L, "z");
    lua_pushinteger(L, current_gamepad_state.z);
    lua_settable(L, -3);
    
    // Set RZ value
    lua_pushstring(L, "rz");
    lua_pushinteger(L, current_gamepad_state.rz);
    lua_settable(L, -3);
    
    return 1;  // Return table with analog values
}

// Lua function to press a gamepad button
int lua_gamepad_press_button(lua_State *L) {
    int button_num = (int)luaL_checknumber(L, 1);  // Get button number from Lua
    
    // Validate button number (1-16)
    if (button_num < 1 || button_num > 16) {
        luaL_error(L, "Invalid button number: %d (must be 1-16)", button_num);
        return 0;
    }
    
    // Inherit analog values from real gamepad input
    lua_gamepad_x = current_gamepad_state.x;
    lua_gamepad_y = current_gamepad_state.y;
    lua_gamepad_z = current_gamepad_state.z;
    lua_gamepad_rz = current_gamepad_state.rz;
    
    // Set button bit (button 1 = bit 0, button 2 = bit 1, etc.)
    uint16_t button_mask = 1 << (button_num - 1);
    lua_gamepad_buttons |= button_mask;
    lua_gamepad_dirty = true;
    lua_gamepad_active = true;
    
    // Send gamepad report if UART mode
    send_gamepad_report();
    
    return 0;  // No return values
}

// Lua function to release a gamepad button
int lua_gamepad_release_button(lua_State *L) {
    int button_num = (int)luaL_checknumber(L, 1);  // Get button number from Lua
    
    // Validate button number (1-16)
    if (button_num < 1 || button_num > 16) {
        luaL_error(L, "Invalid button number: %d (must be 1-16)", button_num);
        return 0;
    }
    
    // Inherit analog values from real gamepad input
    lua_gamepad_x = current_gamepad_state.x;
    lua_gamepad_y = current_gamepad_state.y;
    lua_gamepad_z = current_gamepad_state.z;
    lua_gamepad_rz = current_gamepad_state.rz;
    
    // Clear button bit
    uint16_t button_mask = 1 << (button_num - 1);
    lua_gamepad_buttons &= ~button_mask;
    lua_gamepad_dirty = true;
    
    // Send gamepad report if UART mode
    send_gamepad_report();
    
    return 0;  // No return values
}

// Lua function to set gamepad analog stick values
int lua_gamepad_set_analog(lua_State *L) {
    const char* axis = luaL_checkstring(L, 1);  // Axis name
    int value = (int)luaL_checknumber(L, 2);    // Value (-127 to 127)
    
    // Validate value range
    if (value < -127 || value > 127) {
        luaL_error(L, "Invalid analog value: %d (must be -127 to 127)", value);
        return 0;
    }
    
    // Set the appropriate axis
    if (strcmp(axis, "x") == 0) {
        lua_gamepad_x = (int8_t)value;
    } else if (strcmp(axis, "y") == 0) {
        lua_gamepad_y = (int8_t)value;
    } else if (strcmp(axis, "z") == 0) {
        lua_gamepad_z = (int8_t)value;
    } else if (strcmp(axis, "rz") == 0) {
        lua_gamepad_rz = (int8_t)value;
    } else {
        luaL_error(L, "Invalid axis: %s (must be 'x', 'y', 'z', or 'rz')", axis);
        return 0;
    }
    
    lua_gamepad_dirty = true;
    lua_gamepad_active = true;
    
    // Send gamepad report if UART mode
    send_gamepad_report();
    
    return 0;  // No return values
}

// Lua function to set gamepad hat switch
int lua_gamepad_set_hat(lua_State *L) {
    int value = (int)luaL_checknumber(L, 1);  // Hat value (0-15)
    
    // Validate value range
    if (value < 0 || value > 15) {
        luaL_error(L, "Invalid hat value: %d (must be 0-15)", value);
        return 0;
    }
    
    lua_gamepad_hat = (uint8_t)value;
    lua_gamepad_dirty = true;
    lua_gamepad_active = true;
    
    // Send gamepad report if UART mode
    send_gamepad_report();
    
    return 0;  // No return values
}

// Internal function to reset gamepad state (same as lua_gamepad_reset but without Lua state)
static void internal_gamepad_reset(void) {
    // Inherit analog values from real gamepad input instead of zeroing them
    lua_gamepad_x = current_gamepad_state.x;
    lua_gamepad_y = current_gamepad_state.y;
    lua_gamepad_z = current_gamepad_state.z;
    lua_gamepad_rz = current_gamepad_state.rz;
    
    // Reset hat and buttons only
    lua_gamepad_hat = 0;
    lua_gamepad_buttons = 0;
    lua_gamepad_dirty = true;
    lua_gamepad_active = false;
    lua_gamepad_force_zero = true; // Force sending zero report to clear any stuck buttons/hat
}

// Lua function to reset gamepad state
int lua_gamepad_reset(lua_State *L) {
    internal_gamepad_reset();
    return 0;  // No return values
}

//--------------------------------------------------------------------+
// OLED Display Functions for Lua
//--------------------------------------------------------------------+

// Lua function to display text at specified coordinates
int lua_oled_text(lua_State *L) {
    int x = (int)luaL_checknumber(L, 1);        // X coordinate
    int y = (int)luaL_checknumber(L, 2);        // Y coordinate
    int scale = (int)luaL_checknumber(L, 3);    // Scale factor
    const char* text = luaL_checkstring(L, 4);  // Text to display
    
    // Validate coordinates (assuming 128x32 display)
    if (x < 0 || x > 127) {
        luaL_error(L, "Invalid X coordinate: %d (must be 0-127)", x);
        return 0;
    }
    if (y < 0 || y > 31) {
        luaL_error(L, "Invalid Y coordinate: %d (must be 0-31)", y);
        return 0;
    }
    
    // Validate scale factor
    if (scale < 1 || scale > 4) {
        luaL_error(L, "Invalid scale factor: %d (must be 1-4)", scale);
        return 0;
    }
    
    // Call OLED function
    oled_display_text((uint32_t)x, (uint32_t)y, (uint32_t)scale, text);
    
    return 0;  // No return values
}

// Lua function to display centered text
int lua_oled_centered(lua_State *L) {
    int y = (int)luaL_checknumber(L, 1);        // Y coordinate
    int scale = (int)luaL_checknumber(L, 2);    // Scale factor
    const char* text = luaL_checkstring(L, 3);  // Text to display
    
    // Validate Y coordinate
    if (y < 0 || y > 31) {
        luaL_error(L, "Invalid Y coordinate: %d (must be 0-31)", y);
        return 0;
    }
    
    // Validate scale factor
    if (scale < 1 || scale > 4) {
        luaL_error(L, "Invalid scale factor: %d (must be 1-4)", scale);
        return 0;
    }
    
    // Call OLED function
    oled_display_text_centered((uint32_t)y, (uint32_t)scale, text);
    
    return 0;  // No return values
}

// Lua function to clear OLED display
int lua_oled_clear(lua_State *L) {
    oled_clear_display();
    return 0;  // No return values
}

// Lua function to update OLED display
int lua_oled_update(lua_State *L) {
    oled_update_display();
    return 0;  // No return values
}

// Lua function to check if OLED is present
int lua_oled_present(lua_State *L) {
    bool present = isOLEDPresent();
    lua_pushboolean(L, present);
    return 1;  // Return boolean value
}

// Helper function to send current gamepad state
static void send_gamepad_report(void) {
    if (USB_output_switch != 0) {
        // UART output mode - send to UART1 (same format as GamepadReportParser.c)
        // Pack gamepad data into 8 bytes: x, y, z, rz, hat, buttons(2 bytes), reserved
        uint8_t gamepad_data[8];
        gamepad_data[0] = (uint8_t)lua_gamepad_x;     // X axis
        gamepad_data[1] = (uint8_t)lua_gamepad_y;     // Y axis
        gamepad_data[2] = (uint8_t)lua_gamepad_z;     // Z axis
        gamepad_data[3] = (uint8_t)lua_gamepad_rz;    // RZ axis
        gamepad_data[4] = lua_gamepad_hat;            // Hat switch
        gamepad_data[5] = (uint8_t)(lua_gamepad_buttons & 0xFF);        // Buttons low byte
        gamepad_data[6] = (uint8_t)((lua_gamepad_buttons >> 8) & 0xFF); // Buttons high byte
        gamepad_data[7] = 0;                          // Reserved/unused

        // Encode to base64
        char base64_output[16]; // 8 bytes -> 12 chars base64 + null terminator + padding
        int encoded_length = base64_encode(gamepad_data, sizeof(gamepad_data), base64_output, sizeof(base64_output));
        
        if (encoded_length > 0) {
            // Send gamepad report to UART1 in format: "G<switch_value><base64_data>\n"
            char uart_message[32];
            snprintf(uart_message, sizeof(uart_message), "G%d%s\n", USB_output_switch, base64_output);
            
            // Send to UART1
            uart_puts(uart1, uart_message);
        } else {
            printf("UART1: Failed to encode gamepad data to base64\n");
        }
    }
    // Note: USB output mode is handled by USBDeviceTask.c via get_lua_gamepad_state()
}

// Function to get current Lua gamepad state (called from USBDeviceTask.c)
void get_lua_gamepad_state(int8_t* x, int8_t* y, int8_t* z, int8_t* rz, 
                          uint8_t* hat, uint16_t* buttons, bool* active, bool* dirty) {
    *x = lua_gamepad_x;
    *y = lua_gamepad_y;
    *z = lua_gamepad_z;
    *rz = lua_gamepad_rz;
    *hat = lua_gamepad_hat;
    *buttons = lua_gamepad_buttons;
    *active = lua_gamepad_active || lua_gamepad_force_zero; // Force active if zero report needed
    *dirty = lua_gamepad_dirty || lua_gamepad_force_zero;   // Force dirty if zero report needed
    lua_gamepad_dirty = false; // Clear dirty flag after reading
    lua_gamepad_force_zero = false; // Clear force zero flag after reading
}

// Register custom functions with Lua
void register_lua_functions(lua_State *L) {
    lua_pushcfunction(L, lua_sleep);
    lua_setglobal(L, "sleep");  // Make function available as "sleep(ms)" in Lua
    
    lua_pushcfunction(L, lua_keypress);
    lua_setglobal(L, "keypress");  // Make function available as "keypress(keycode)" in Lua
    
    lua_pushcfunction(L, lua_keyrelease);
    lua_setglobal(L, "keyrelease");  // Make function available as "keyrelease(keycode)" in Lua
    
    lua_pushcfunction(L, lua_type_text);
    lua_setglobal(L, "type");  // Make function available as "type(text, delay_ms)" in Lua
    
    lua_pushcfunction(L, lua_set_language);
    lua_setglobal(L, "lang");  // Make function available as "lang(language)" in Lua
    
    lua_pushcfunction(L, lua_get_language);
    lua_setglobal(L, "getlang");  // Make function available as "getlang()" in Lua

    lua_pushcfunction(L, lua_switch);
    lua_setglobal(L, "switch");  // Make function available as "switch()" in Lua

    lua_pushcfunction(L, lua_set);
    lua_setglobal(L, "set");  // Make function available as "set(param_name, value)" in Lua
    
    // Mouse functions
    lua_pushcfunction(L, lua_mouse_move);
    lua_setglobal(L, "mouse_move");  // Make function available as "mouse_move(x, y)" in Lua
    
    lua_pushcfunction(L, lua_mouse_press);
    lua_setglobal(L, "mouse_press");  // Make function available as "mouse_press(button)" in Lua
    
    lua_pushcfunction(L, lua_mouse_release);
    lua_setglobal(L, "mouse_release");  // Make function available as "mouse_release(button)" in Lua
    
    lua_pushcfunction(L, lua_mouse_scroll);
    lua_setglobal(L, "mouse_scroll");  // Make function available as "mouse_scroll(wheel)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_button);
    lua_setglobal(L, "gamepad_get_button");  // Make function available as "gamepad_get_button(button)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_hat);
    lua_setglobal(L, "gamepad_get_hat");  // Make function available as "gamepad_get_hat()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_x);
    lua_setglobal(L, "gamepad_get_x");  // Make function available as "gamepad_get_x()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_y);
    lua_setglobal(L, "gamepad_get_y");  // Make function available as "gamepad_get_y()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_z);
    lua_setglobal(L, "gamepad_get_z");  // Make function available as "gamepad_get_z()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_rz);
    lua_setglobal(L, "gamepad_get_rz");  // Make function available as "gamepad_get_rz()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_get_analog);
    lua_setglobal(L, "gamepad_get_analog");  // Make function available as "gamepad_get_analog()" in Lua
    
    lua_pushcfunction(L, lua_gamepad_press_button);
    lua_setglobal(L, "gamepad_press_button");  // Make function available as "gamepad_press_button(button)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_release_button);
    lua_setglobal(L, "gamepad_release_button");  // Make function available as "gamepad_release_button(button)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_set_analog);
    lua_setglobal(L, "gamepad_set_analog");  // Make function available as "gamepad_set_analog(axis, value)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_set_hat);
    lua_setglobal(L, "gamepad_set_hat");  // Make function available as "gamepad_set_hat(value)" in Lua
    
    lua_pushcfunction(L, lua_gamepad_reset);
    lua_setglobal(L, "gamepad_reset");  // Make function available as "gamepad_reset()" in Lua
    
    // OLED display functions
    lua_pushcfunction(L, lua_oled_text);
    lua_setglobal(L, "oled_text");  // Make function available as "oled_text(x, y, scale, text)" in Lua
    
    lua_pushcfunction(L, lua_oled_centered);
    lua_setglobal(L, "oled_centered");  // Make function available as "oled_centered(y, scale, text)" in Lua
    
    lua_pushcfunction(L, lua_oled_clear);
    lua_setglobal(L, "oled_clear");  // Make function available as "oled_clear()" in Lua
    
    lua_pushcfunction(L, lua_oled_update);
    lua_setglobal(L, "oled_update");  // Make function available as "oled_update()" in Lua
    
    lua_pushcfunction(L, lua_oled_present);
    lua_setglobal(L, "oled_present");  // Make function available as "oled_present()" in Lua
}

//--------------------------------------------------------------------+
// FIFO Queue Processing
//--------------------------------------------------------------------+

// Macro execution tracking
#define MAX_MACRO_NAME_LENGTH 32
#define MAX_QUEUED_MACROS 16

static char currently_executing_macro[MAX_MACRO_NAME_LENGTH] = {0};
static char queued_macros[MAX_QUEUED_MACROS][MAX_MACRO_NAME_LENGTH];
static uint8_t queued_macro_count = 0;

// Helper function to extract macro name from command
static void extract_macro_name(const char* command, char* macro_name, size_t max_len) {
    macro_name[0] = '\0'; // Initialize as empty string
    
    if (strncmp(command, "FILE:", 5) == 0) {
        // Extract filename after "FILE:" prefix
        const char* filename = command + 5;
        strncpy(macro_name, filename, max_len - 1);
        macro_name[max_len - 1] = '\0';
    } else {
        // For direct Lua commands, use a hash or truncated version as identifier
        // For simplicity, we'll use the first 31 characters as identifier
        strncpy(macro_name, command, max_len - 1);
        macro_name[max_len - 1] = '\0';
    }
}

// Check if macro is currently being executed
static bool is_macro_executing(const char* macro_name) {
    return (currently_executing_macro[0] != '\0' && 
            strcmp(currently_executing_macro, macro_name) == 0);
}

// Check if macro is in the queue
static bool is_macro_queued(const char* macro_name) {
    for (uint8_t i = 0; i < queued_macro_count; i++) {
        if (strcmp(queued_macros[i], macro_name) == 0) {
            return true;
        }
    }
    return false;
}

// Add macro to queued list
static bool add_macro_to_queue_list(const char* macro_name) {
    if (queued_macro_count >= MAX_QUEUED_MACROS) {
        return false; // Queue list full
    }
    
    strncpy(queued_macros[queued_macro_count], macro_name, MAX_MACRO_NAME_LENGTH - 1);
    queued_macros[queued_macro_count][MAX_MACRO_NAME_LENGTH - 1] = '\0';
    queued_macro_count++;
    
    return true;
}

// Remove macro from queued list
static void remove_macro_from_queue_list(const char* macro_name) {
    for (uint8_t i = 0; i < queued_macro_count; i++) {
        if (strcmp(queued_macros[i], macro_name) == 0) {
            // Shift remaining items down
            for (uint8_t j = i; j < queued_macro_count - 1; j++) {
                strcpy(queued_macros[j], queued_macros[j + 1]);
            }
            queued_macro_count--;
            break;
        }
    }
}

// Set currently executing macro
static void set_executing_macro(const char* macro_name) {
    strncpy(currently_executing_macro, macro_name, MAX_MACRO_NAME_LENGTH - 1);
    currently_executing_macro[MAX_MACRO_NAME_LENGTH - 1] = '\0';
}

// Clear currently executing macro
static void clear_executing_macro(void) {
    currently_executing_macro[0] = '\0';
}

// Check if macro can be executed (not already running or queued)
static bool can_execute_macro(const char* macro_name) {
    if (macro_name[0] == '\0') {
        return true; // Allow empty names (direct commands)
    }
    
    return !is_macro_executing(macro_name) && !is_macro_queued(macro_name);
}

// Global Lua state for FIFO command execution
static lua_State *fifo_lua_state = NULL;

// Initialize Lua state for FIFO commands
void init_fifo_lua_state(void) {
    if (fifo_lua_state == NULL) {
        fifo_lua_state = luaL_newstate();
        if (fifo_lua_state != NULL) {
            luaL_openlibs(fifo_lua_state);
            register_lua_functions(fifo_lua_state);
            printf("FIFO Lua state initialized\n");
        } else {
            printf("Failed to create FIFO Lua state\n");
        }
    }
}

// Execute Lua script from FIFO command
void execute_lua_command(const char* lua_command) {
    if (fifo_lua_state == NULL) {
        printf("FIFO Lua state not initialized\n");
        return;
    }
    
    // Extract macro name for tracking
    char macro_name[MAX_MACRO_NAME_LENGTH];
    extract_macro_name(lua_command, macro_name, sizeof(macro_name));
    
    // Set as currently executing
    set_executing_macro(macro_name);
    
    printf("[Lua] Executing: %s\n", lua_command);
    
    // Execute the Lua command
    int result = luaL_dostring(fifo_lua_state, lua_command);
    
    if (result != LUA_OK) {
        // Handle Lua error
        const char* error_msg = lua_tostring(fifo_lua_state, -1);
        printf("[Lua Error] %s\n", error_msg);
        
        // Output error to CDC if connected
        if (tud_cdc_connected()) {
            char full_error[128];
            snprintf(full_error, sizeof(full_error), "[Lua Error] %s\r\n", error_msg);
            tud_cdc_write_str(full_error);
            tud_cdc_write_flush();
        }
        
        lua_pop(fifo_lua_state, 1);  // Remove error message from stack
    } else {
        // Check if there's a return value
        if (lua_gettop(fifo_lua_state) > 0) {
            if (lua_isnumber(fifo_lua_state, -1)) {
                double return_value = lua_tonumber(fifo_lua_state, -1);
                printf("[Lua] Returned: %f\n", return_value);
            } else if (lua_isstring(fifo_lua_state, -1)) {
                const char* return_str = lua_tostring(fifo_lua_state, -1);
                printf("[Lua] Returned: %s\n", return_str);
            }
            lua_pop(fifo_lua_state, 1);  // Remove return value from stack
        }
    }
    
    // Clear currently executing macro
    clear_executing_macro();
    
    // Automatically reset gamepad state after gamepad-related command execution
    // This ensures that user gamepad input will work again after any gamepad command
    if (strstr(lua_command, "gamepad_") != NULL) {
        internal_gamepad_reset();
    }
}

// Execute Lua script from LittleFS file
void execute_lua_file(const char* filename) {
    if (fifo_lua_state == NULL) {
        printf("FIFO Lua state not initialized\n");
        return;
    }
    
    // Set as currently executing
    set_executing_macro(filename);
    
    // Try to read the file from LittleFS
    char* file_buffer = malloc(8192); // 8KB buffer for Lua script files
    if (!file_buffer) {
        printf("[Lua File Error] Failed to allocate memory for file reading\n");
        if (tud_cdc_connected()) {
            tud_cdc_write_str("[Lua File Error] Failed to allocate memory for file reading\r\n");
            tud_cdc_write_flush();
        }
        clear_executing_macro();
        return;
    }
    
    int bytes_read = fstask_read_file(filename, file_buffer, 8191);
    if (bytes_read < 0) {
        printf("[Lua File Error] Failed to read file '%s' (error: %d)\n", filename, bytes_read);
        if (tud_cdc_connected()) {
            char error_msg[128];
            snprintf(error_msg, sizeof(error_msg), "[Lua File Error] Failed to read file '%s' (error: %d)\r\n", filename, bytes_read);
            tud_cdc_write_str(error_msg);
            tud_cdc_write_flush();
        }
        free(file_buffer);
        clear_executing_macro();
        return;
    }
    
    // Null terminate the file content
    file_buffer[bytes_read] = '\0';
    
    // printf("[Lua File] File loaded successfully (%d bytes)\n", bytes_read);
    
    // Execute the Lua script
    int result = luaL_dostring(fifo_lua_state, file_buffer);
    
    if (result != LUA_OK) {
        // Handle Lua error
        const char* error_msg = lua_tostring(fifo_lua_state, -1);
        printf("[Lua File Error] %s\n", error_msg);
        
        // Output error to CDC if connected
        if (tud_cdc_connected()) {
            char full_error[256];
            snprintf(full_error, sizeof(full_error), "[Lua File Error] %s\r\n", error_msg);
            tud_cdc_write_str(full_error);
            tud_cdc_write_flush();
        }
        
        lua_pop(fifo_lua_state, 1);  // Remove error message from stack
    } else {
        // Check if there's a return value
        if (lua_gettop(fifo_lua_state) > 0) {
            if (lua_isnumber(fifo_lua_state, -1)) {
                double return_value = lua_tonumber(fifo_lua_state, -1);
                printf("[Lua File] Returned: %f\n", return_value);
            } else if (lua_isstring(fifo_lua_state, -1)) {
                const char* return_str = lua_tostring(fifo_lua_state, -1);
                printf("[Lua File] Returned: %s\n", return_str);
            }
            lua_pop(fifo_lua_state, 1);  // Remove return value from stack
        }
    }
    
    // Check if this file contains gamepad commands before cleanup
    bool contains_gamepad_commands = (strstr(filename, "Pad-") != NULL || 
                                     strstr(filename, "Meta-") != NULL || 
                                     strstr(file_buffer, "gamepad_") != NULL);
    
    // Clean up
    free(file_buffer);
    clear_executing_macro();
    
    // Automatically reset gamepad state after macro execution for gamepad-related files
    // This ensures that user gamepad input will work again after any macro
    if (contains_gamepad_commands) {
        internal_gamepad_reset();
    }
}

// Check FIFO queue and execute commands as Lua scripts
void process_fifo_commands(void) {
    char command[64];  // Match MAX_COMMAND_LENGTH from USBtask.c
    
    while (!fifo_is_empty()) {
        if (fifo_pop(command, sizeof(command))) {
            // Extract macro name for duplicate checking
            char macro_name[MAX_MACRO_NAME_LENGTH];
            extract_macro_name(command, macro_name, sizeof(macro_name));
            
            // Remove from queued list (it was queued when added to FIFO)
            remove_macro_from_queue_list(macro_name);
            
            // Execute the command
            if (strncmp(command, "FILE:", 5) == 0) {
                // Extract filename after "FILE:" prefix
                const char* filename = command + 5;
                execute_lua_file(filename);
                
            } else {
                // Execute command as direct Lua script
                execute_lua_command(command);
                
            }
            
            // Add small delay to avoid overwhelming system
            vTaskDelay(pdMS_TO_TICKS(50)); // 50ms delay (reduced for better responsiveness)
        }
    }
}

// External interface functions for duplicate checking
bool can_execute_macro_external(const char* command) {
    char macro_name[MAX_MACRO_NAME_LENGTH];
    extract_macro_name(command, macro_name, sizeof(macro_name));
    return can_execute_macro(macro_name);
}

bool add_macro_to_queue_list_external(const char* command) {
    char macro_name[MAX_MACRO_NAME_LENGTH];
    extract_macro_name(command, macro_name, sizeof(macro_name));
    return add_macro_to_queue_list(macro_name);
}

// Enhanced FIFO push function with duplicate checking
bool fifo_push_with_duplicate_check(const char* command) {
    // Check if macro can be executed (not already running or queued)
    if (!can_execute_macro_external(command)) {
        char macro_name[MAX_MACRO_NAME_LENGTH];
        extract_macro_name(command, macro_name, sizeof(macro_name));
        
        printf("[Lua] Macro '%s' is already executing or queued - skipping\n", macro_name);
        
        if (tud_cdc_connected()) {
            char skip_msg[128];
            snprintf(skip_msg, sizeof(skip_msg), "[Lua] Macro '%s' is already executing or queued - skipping\r\n", macro_name);
            tud_cdc_write_str(skip_msg);
            tud_cdc_write_flush();
        }
        
        return false; // Command rejected due to duplicate
    }
    
    // Add to queue tracking list
    if (!add_macro_to_queue_list_external(command)) {
        printf("[Lua] Queue tracking list full - cannot track macro\n");
        if (tud_cdc_connected()) {
            tud_cdc_write_str("[Lua] Queue tracking list full - cannot track macro\r\n");
            tud_cdc_write_flush();
        }
        return false;
    }
    
    // Use the original fifo_push function
    return fifo_push(command);
}

//--------------------------------------------------------------------+
// Lua Task Implementation
//--------------------------------------------------------------------+

// Task 2 - Lua script execution task function  
void task_lua_function(void *pvParameters)
{
    (void)pvParameters; // Avoid unused parameter warning
    
    printf("Lua Task started\n");

    lua_State *L = luaL_newstate();
    if (L == NULL) {
        printf("Failed to create Lua state\n");
    }

    // Initialize Lua state

    // Load Lua libraries
    luaL_openlibs(L);
    // Register custom functions
    register_lua_functions(L);   
    // Clean up Lua state
    lua_close(L);
    
    // Initialize Lua state for FIFO command processing
    init_fifo_lua_state();
    
    printf("Lua Task initialization complete\n");
    // Wait a bit more for the file system to be fully ready
    
    // Check for autorun.lua and execute it if it exists
    lfs_ssize_t autorun_size = fstask_get_file_size("autorun.lua");
    if (autorun_size > 0) {
        printf("Executing autorun.lua (%d bytes)\n", (int)autorun_size);
        execute_lua_file("autorun.lua");
    } else {
        printf("autorun.lua not found or empty\n");
    }
    
    // Main task loop - monitor FIFO queue and process commands
    while (1)
    {
        // Check and process any commands in the FIFO queue
        process_fifo_commands();
        
        // Delay before next check (1ms for responsive command processing)
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}