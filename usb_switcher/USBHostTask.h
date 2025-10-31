#ifndef USBHOSTTASK_H
#define USBHOSTTASK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"

// HID report structures
#include "class/hid/hid.h"
#include "MouseReportParser.h"

#define VERSION_STRING "USB HID Switcher v1.0.1"

// External variables that USB Host functions need access to
extern uint8_t const keycode2ascii[128][2];

// Keyboard report buffering for retry functionality
#define KEYBOARD_REPORT_BUFFER_SIZE 8
#define MOUSE_REPORT_BUFFER_SIZE 8

typedef struct {
    hid_keyboard_report_t report;
    uint32_t timestamp;
    bool valid;
} buffered_keyboard_report_t;

typedef struct {
    mouse_report_t report;
    uint32_t timestamp;
    bool valid;
} buffered_mouse_report_t;

// Function declarations for USB Host functionality
void usb_host_task(void);
void try_send_buffered_keyboard_reports(void);
void try_send_buffered_mouse_reports(void);
void send_keyboard_led_state(uint8_t led_state);

// HID processing functions
void process_kbd_report(uint8_t dev_addr, hid_keyboard_report_t const* report, uint16_t len);
void process_mouse_report(uint8_t instance, uint8_t const* report, uint16_t len);

// TinyUSB Host HID Callbacks (these must be global)
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len);
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance);
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);

#define USE_DEVICE_PROTOCOL HID_PROTOCOL_BOOT
//#define USE_DEVICE_PROTOCOL HID_PROTOCOL_REPORT

#endif // USBHOSTTASK_H