set(target_name usb_hid_switcher)

add_executable(${target_name})

pico_enable_stdio_usb(${target_name} 0)  # Disable default stdio USB (conflict with TinyUSB host)
pico_enable_stdio_uart(${target_name} 1) # Enable UART stdio as fallback 

# Set local TinyUSB path
set(LOCAL_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/../tinyusb)

target_sources(${target_name} PRIVATE
  usb_hid_switcher.c
  usb_descriptors.c
  ws2812.c
  LEDtask.c
  OLEDtask.c
  ssd1306.c
  USBtask.c
  USBHostTask.c
  USBDeviceTask.c
  UARTtask.c
  LuaTask.c
  fstask.c
  CDCCmd.c
  configRead.c
  base64.c
  MouseReportParser.c
  ReportParser.c
  GamepadReportParser.c
  # Add PIO USB Host Controller Driver for local TinyUSB
  ${CMAKE_CURRENT_LIST_DIR}/../tinyusb/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

# Generate PIO header from ws2812.pio
pico_generate_pio_header(usb_hid_switcher ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)


target_include_directories(${target_name} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${LOCAL_TINYUSB_PATH}/src
        ${LOCAL_TINYUSB_PATH}/src/common
        ${LOCAL_TINYUSB_PATH}/src/device
        ${LOCAL_TINYUSB_PATH}/src/host
        ${LOCAL_TINYUSB_PATH}/src/class/hid
        ${LOCAL_TINYUSB_PATH}/src/class/cdc
        ${LOCAL_TINYUSB_PATH}/src/portable/raspberrypi/rp2040
        )

# print memory usage, enable all warnings
target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_options(${target_name} PRIVATE -Wall -Wextra)

# use tinyusb implementation for PIO USB
target_compile_definitions(${target_name} PRIVATE PIO_USB_USE_TINYUSB)

# Use local TinyUSB configuration
target_compile_definitions(${target_name} PRIVATE CFG_TUSB_MCU=OPT_MCU_RP2040)

# Define PIO USB pin configuration
target_compile_definitions(${target_name} PRIVATE PICO_DEFAULT_PIO_USB_DP_PIN=4)

# Define board configuration for TinyUSB
target_compile_definitions(${target_name} PRIVATE BOARD_TUD_RHPORT=0)
target_compile_definitions(${target_name} PRIVATE BOARD_TUH_RHPORT=1)

# TinyUSB Debug Configuration (0=Off, 1=Error, 2=Warning, 3=Info, 4=Debug)
target_compile_definitions(${target_name} PRIVATE CFG_TUSB_DEBUG=0)
target_compile_definitions(${target_name} PRIVATE CFG_TUH_LOG_LEVEL=0)
target_compile_definitions(${target_name} PRIVATE CFG_TUD_LOG_LEVEL=0)

# Disable stdio USB to prevent conflicts with USB Host
target_compile_definitions(${target_name} PRIVATE PICO_STDIO_USB=1)

target_link_libraries(${target_name} PRIVATE 
    pico_stdlib 
    pico_multicore
    pico_pio_usb
    hardware_uart
    hardware_i2c
    tinyusb_device
    tinyusb_host
    tinyusb_board
    freertos
    lua
    littlefs
)

pico_add_extra_outputs(${target_name})