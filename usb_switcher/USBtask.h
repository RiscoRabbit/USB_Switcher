#ifndef USBTASK_H
#define USBTASK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "bsp/board.h"
#include "tusb.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "pico/multicore.h"
#include "pio_usb.h"
#include "lfs.h"

// USB Status enumeration
enum {
    USB_NOT_MOUNTED,
    USB_MOUNTED = 1000,
    USB_SUSPENDED = 2500,
};

// Standard gamepad report structure with 16 buttons
typedef struct TU_ATTR_PACKED
{
    int8_t x;            // Left stick X axis (-127 to 127)
    int8_t y;            // Left stick Y axis (-127 to 127)
    int8_t z;            // Right stick X axis (-127 to 127)
    int8_t rz;           // Right stick Y axis (-127 to 127)
    uint8_t hat;         // Hat switch (1-8, 0=center)
    uint16_t buttons;    // 16 buttons (bits 0-15)
} hid_standard_gamepad_report_t;

// Vibration output report structure
typedef struct TU_ATTR_PACKED
{
    uint8_t enable;      // Enable actuators
    uint8_t left_motor;  // Left motor intensity (0-255)
    uint8_t right_motor; // Right motor intensity (0-255)
} hid_vibration_report_t;

// Function prototypes
void task1_function(void);
void led_status_task(void);
void hid_task(void);
void vibration_control_task(void);


// USB Host functions
void usb_host_task(void);

// TinyUSB Callback functions (must be available globally)
// USB Device callbacks
void tud_mount_cb(void);
void tud_umount_cb(void);
void tud_suspend_cb(bool remote_wakeup_en);
void tud_resume_cb(void);
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len);
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol);
uint8_t tud_hid_get_protocol_cb(uint8_t instance);
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);
void tud_cdc_rx_cb(uint8_t itf);
void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char);
// USB Host callbacks
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);
void show_protocol_settings(void);

// Global variable accessors
extern uint32_t device_usb_status;
extern hid_vibration_report_t vibration_state;
extern uint8_t const keycode2ascii[128][2];
extern bool host_initialized;
extern bool has_gamepad_key;
extern uint8_t hid_protocol[3];

extern uint8_t metaKeyCode;
extern uint8_t default_hid_protocol;
extern int device_id;

void setMetaKeyCode(uint8_t code);

#endif // USBTASK_H