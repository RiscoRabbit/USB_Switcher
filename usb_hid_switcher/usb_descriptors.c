#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug. */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID   0x000a

#define USB_VID   0xCa07
#define USB_BCD   0x0200

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_keyboard_report[] =
{
  TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(1) )
};

uint8_t const desc_hid_mouse_report[] =
{
  TUD_HID_REPORT_DESC_MOUSE( HID_REPORT_ID(2) )
};

uint8_t const desc_hid_gamepad_report[] =
{
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      ),
  HID_USAGE      ( HID_USAGE_DESKTOP_GAMEPAD   ),  // GAMEPADに戻す
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  ),
    // Report ID
    HID_REPORT_ID( 3 )
    
    // Left analog stick X and Y (8-bit signed)
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP    ),
    HID_USAGE       ( HID_USAGE_DESKTOP_X       ),
    HID_USAGE       ( HID_USAGE_DESKTOP_Y       ),
    HID_LOGICAL_MIN ( 0x81                      ), // -127
    HID_LOGICAL_MAX ( 0x7F                      ), // 127  
    HID_REPORT_COUNT( 2                         ),
    HID_REPORT_SIZE ( 8                         ),
    HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    
    // Right analog stick (Z and RZ)
    HID_USAGE       ( HID_USAGE_DESKTOP_Z       ),
    HID_USAGE       ( HID_USAGE_DESKTOP_RZ      ),
    HID_LOGICAL_MIN ( 0x81                      ), // -127
    HID_LOGICAL_MAX ( 0x7F                      ), // 127
    HID_REPORT_COUNT( 2                         ),
    HID_REPORT_SIZE ( 8                         ),
    HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    
    // Hat Switch (8-bit)
    HID_USAGE       ( HID_USAGE_DESKTOP_HAT_SWITCH ),
    HID_LOGICAL_MIN ( 1                         ),
    HID_LOGICAL_MAX ( 8                         ),
    HID_PHYSICAL_MIN( 0                         ),
    HID_PHYSICAL_MAX_N ( 315, 2                 ),
    HID_REPORT_COUNT( 1                         ),
    HID_REPORT_SIZE ( 8                         ),
    HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    
    // Buttons (16 buttons = 2 bytes, no padding needed)
    HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON     ),
    HID_USAGE_MIN   ( 1                         ),
    HID_USAGE_MAX   ( 16                        ),
    HID_LOGICAL_MIN ( 0                         ),
    HID_LOGICAL_MAX ( 1                         ),
    HID_REPORT_COUNT( 16                        ),
    HID_REPORT_SIZE ( 1                         ),
    HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
    
  HID_COLLECTION_END
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
  switch(instance)
  {
    case 0:
      return desc_hid_keyboard_report;
    case 1:
      return desc_hid_mouse_report;
    case 2:
      return desc_hid_gamepad_report;
    default:
      return NULL;
  }
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
  ITF_NUM_KEYBOARD = 0,
  ITF_NUM_MOUSE,
  ITF_NUM_GAMEPAD,
  ITF_NUM_CDC,
  ITF_NUM_CDC_DATA,
  ITF_NUM_TOTAL
};

#define EPNUM_KEYBOARD  0x81
#define EPNUM_MOUSE     0x82
#define EPNUM_GAMEPAD   0x83

#define EPNUM_CDC_NOTIF 0x84
#define EPNUM_CDC_OUT   0x04
#define EPNUM_CDC_IN    0x85

#define  CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN * 3)

uint8_t const desc_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

  // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
  TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_keyboard_report), EPNUM_KEYBOARD, CFG_TUD_HID_EP_BUFSIZE, 1),
  TUD_HID_DESCRIPTOR(ITF_NUM_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(desc_hid_mouse_report), EPNUM_MOUSE, CFG_TUD_HID_EP_BUFSIZE, 1),
  TUD_HID_DESCRIPTOR(ITF_NUM_GAMEPAD, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_gamepad_report), EPNUM_GAMEPAD, CFG_TUD_HID_EP_BUFSIZE, 1),

  // CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size.
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "TinyUSB2",                     // 1: Manufacturer
  "TinyUSB Device2",              // 2: Product
  "123456789012",                // 3: Serials, should use chip ID
  "TinyUSB CDC",                 // 4: CDC Interface
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}