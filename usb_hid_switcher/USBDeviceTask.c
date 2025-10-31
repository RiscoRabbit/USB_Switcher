#include "USBDeviceTask.h"
#include "GamepadReportParser.h"
#include "LuaTask.h"  // For Lua gamepad control functions
#include "USBHostTask.h"  // For send_keyboard_led_state function
#include <stdlib.h>
#include <string.h>

// External variables that need to be accessed from USBDeviceTask.c
extern bool has_gamepad_key;
extern uint8_t hid_protocol[3];

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
    // Poll every 10ms
    const uint32_t interval_ms = 10;
    static uint32_t start_ms = 0;

    if ( board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    // Remote wakeup
    if ( tud_suspended() )
    {
        // Wake up host if we are in suspend mode
        // and REMOTE_WAKEUP feature is enabled by host
        tud_remote_wakeup();
        return;
    }

    /*------------- Keyboard -------------*/
    if ( tud_hid_n_ready(0) )
    {
        // Keyboard interface is ready - USBHostTask will send reports directly
        // This section ensures the interface stays responsive
    }

    /*------------- Mouse -------------*/
    if ( tud_hid_n_ready(1) )
    {
        // Mouse functionality placeholder
    }

    /*------------- Gamepad -------------*/
    if ( tud_hid_n_ready(2) )
    {
        // use to avoid send multiple consecutive zero report
        static bool has_gamepad_key_last = false;

        hid_standard_gamepad_report_t report =
        {
            .x = 0, .y = 0, .z = 0, .rz = 0,
            .hat = 0, .buttons = 0
        };

        bool send_report = false;

        // Check for Lua gamepad control first
        int8_t lua_x, lua_y, lua_z, lua_rz;
        uint8_t lua_hat;
        uint16_t lua_buttons;
        bool lua_active, lua_dirty;
        
        get_lua_gamepad_state(&lua_x, &lua_y, &lua_z, &lua_rz, 
                             &lua_hat, &lua_buttons, &lua_active, &lua_dirty);
        
        if (lua_active) {
            // Use Lua-controlled gamepad state with real gamepad analog values
            // Analog values: use real gamepad input if available, otherwise use Lua values
            if (has_gamepad_key && !gamepad_state_updated) {
                // Use real gamepad analog values when real gamepad is present
                report.x = current_gamepad_state.x;
                report.y = current_gamepad_state.y;
                report.z = current_gamepad_state.z;
                report.rz = current_gamepad_state.rz;
            } else {
                // Use Lua analog values when no real gamepad or during gamepad updates
                report.x = lua_x;
                report.y = lua_y;
                report.z = lua_z;
                report.rz = lua_rz;
            }
            
            // Hat and buttons: use Lua-controlled values
            report.hat = lua_hat;
            report.buttons = lua_buttons;
            
            send_report = lua_dirty || has_gamepad_key_last != lua_active;
            has_gamepad_key = true;
        }
        // Check if we have new gamepad data from USB host
        else if (gamepad_state_updated) {
            // Copy the received gamepad data to the report
            report.x = current_gamepad_state.x;
            report.y = current_gamepad_state.y;
            report.z = current_gamepad_state.z;
            report.rz = current_gamepad_state.rz;
            report.hat = current_gamepad_state.hat;
            report.buttons = current_gamepad_state.buttons;
            
            send_report = true;
            gamepad_state_updated = false; // Mark as processed
            has_gamepad_key = true;
        }
        // Send zero report if gamepad was disconnected or no data
        else if (has_gamepad_key_last && !has_gamepad_key) {
            // Send zero report to clear previous state
            send_report = true;
        }

        if ( send_report )
        {
            // Send gamepad report to USB device interface
            tud_hid_n_report(2, 3, &report, sizeof(report));
            has_gamepad_key_last = has_gamepad_key;
        }
    }
}

//--------------------------------------------------------------------+
// USB Device Callbacks (These need to be global and available from main)
//--------------------------------------------------------------------+

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
    (void) len;
    (void) report;
    
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
    // TODO not Implemented
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;

    return 0;
}

// Invoked when received SET_PROTOCOL control request
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol)
{
    printf("HID Set Protocol: instance=%d, protocol=%d (%s)\n", 
           instance, protocol, protocol ? "Report" : "Boot");
    
    // Store protocol state for this instance
    if (instance < 3) {
        hid_protocol[instance] = protocol;
        
        // Handle protocol-specific behavior
        switch (instance) {
            case 0: // Keyboard
                if (protocol == 0) {
                    printf("Keyboard switched to Boot mode (6-key rollover)\n");
                } else {
                    printf("Keyboard switched to Report mode (NKRO capable)\n");
                }
                break;
                
            case 1: // Mouse
                if (protocol == 0) {
                    printf("Mouse switched to Boot mode (3-button + wheel)\n");
                } else {
                    printf("Mouse switched to Report mode (extended features)\n");
                }
                break;
                
            case 2: // Gamepad
                printf("Gamepad protocol change (custom device)\n");
                break;
                
            default:
                break;
        }
    }
}

// Invoked when received GET_PROTOCOL control request
uint8_t tud_hid_get_protocol_cb(uint8_t instance)
{
    uint8_t protocol = (instance < 3) ? hid_protocol[instance] : 1;
    printf("HID Get Protocol: instance=%d, returning protocol=%d (%s)\n", 
           instance, protocol, protocol ? "Report" : "Boot");
    return protocol;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // Keyboard SET_REPORT handling
    if (instance == 0) // Keyboard instance
    {
        // Handle keyboard LED output reports (typically report_type == OUTPUT)
        if (report_type == HID_REPORT_TYPE_OUTPUT && bufsize >= 1)
        {
            uint8_t led_state = buffer[0];
            // Forward the LED state to USB Host keyboards
            send_keyboard_led_state(led_state);
        }
    }
    // Gamepad vibration handling
    else if (instance == 2 && report_id == 3 && report_type == HID_REPORT_TYPE_OUTPUT)
    {
        if (bufsize >= sizeof(hid_vibration_report_t))
        {
            // Copy vibration data
            memcpy(&vibration_state, buffer, sizeof(hid_vibration_report_t));
            
            printf("Vibration received - Enable: %d, Left: %d, Right: %d\n", 
                   vibration_state.enable, vibration_state.left_motor, vibration_state.right_motor);
            
            // ここで実際の振動制御を実装
            // 例：PWMでモーターを制御、GPIOでバイブレーターを制御など
            if (vibration_state.enable)
            {
                // 左モーター制御（強い振動）
                if (vibration_state.left_motor > 0)
                {
                    printf("Left motor at %d%% intensity\n", (vibration_state.left_motor * 100) / 255);
                    // TODO: 実際の左モーター制御をここに実装
                }
                
                // 右モーター制御（弱い振動）
                if (vibration_state.right_motor > 0)
                {
                    printf("Right motor at %d%% intensity\n", (vibration_state.right_motor * 100) / 255);
                    // TODO: 実際の右モーター制御をここに実装
                }
            }
            else
            {
                printf("Vibration disabled\n");
                // TODO: 振動停止をここに実装
            }
        }
    }
}

//--------------------------------------------------------------------+
// CDC Callbacks
//--------------------------------------------------------------------+

// Invoked when cdc when line state changed e.g connected/disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void) itf;
    (void) rts;

    // TODO set some indicator
    if ( dtr )
    {
        // Terminal connected
        printf("CDC Terminal connected\n");
    }
    else
    {
        // Terminal disconnected
        printf("CDC Terminal disconnected\n");
    }
}

// Invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void) itf;
}

// Invoked when received 'wanted' char
void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char)
{
    (void) itf;
    (void) wanted_char;
    
    tud_cdc_read_flush();    // flush read fifo
}