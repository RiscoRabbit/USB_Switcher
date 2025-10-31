#ifndef USBDEVICETASK_H_
#define USBDEVICETASK_H_

#include "USBtask.h"

// Function declarations for USB Device functionality
void hid_task(void);
void vibration_control_task(void);

// TinyUSB Device HID Callbacks
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len);
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen);
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol);
uint8_t tud_hid_get_protocol_cb(uint8_t instance);
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize);

// TinyUSB Device CDC Callbacks
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts);
void tud_cdc_rx_cb(uint8_t itf);
void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char);

#endif /* USBDEVICETASK_H_ */