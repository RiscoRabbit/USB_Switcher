
cmake_minimum_required(VERSION 3.13)

# Use local modified TinyUSB to fix Interface 1+ issues - MUST be set before pico_sdk_init()
set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/tinyusb)

include (pico_sdk_import.cmake)
project(pio_usb)

pico_sdk_init()

# add pico-pio-usb library for examples
set(PICO_PIO_USB_DIR "${CMAKE_CURRENT_LIST_DIR}/pico_pio_usb/")
# the second argument to add_subdirectory is needed here because this isn't
# a subdirectory, it's out of tree.
# add_subdirectory(${PICO_PIO_USB_DIR} pico_pio_usb)

add_subdirectory(pico_pio_usb)

add_subdirectory(freertos)
add_subdirectory(lua)
add_subdirectory(littlefs)


add_subdirectory(usb_hid_switcher)
