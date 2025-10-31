#ifndef CONFIGREAD_H
#define CONFIGREAD_H

#include <stdint.h>
#include "lfs.h"  // For lfs_size_t type

// External reference to the global variable that will be set by the config
extern uint8_t default_hid_protocol;
extern int device_id;

/**
 * Function to read configuration file from littlefs and parse settings
 * @return 0: 成功, 負の値: エラー
 */
int read_config_file(void);

/**
 * Function to scan and read all device definition files at startup
 * @return 0: 成功, 負の値: エラー
 */
int scan_and_read_device_definitions(void);

/**
 * Function to find a device parser by VID and PID
 * @param vid Vendor ID
 * @param pid Product ID
 * @return Pointer to interface_report_parser_info_t or NULL if not found
 */
void* find_device_parser(uint16_t vid, uint16_t pid);

/**
 * Function to parse mouse definition from content string
 * @param content The content string containing mouse definition
 * @param vid Vendor ID
 * @param pid Product ID
 * @return 0: 成功, 負の値: エラー
 */
int parse_mouse_definition(const char* content, uint16_t vid, uint16_t pid);

/**
 * Callback function for directory listing during device definition scanning
 * @param name File name
 * @param type File type (LFS_TYPE_REG for regular file, LFS_TYPE_DIR for directory)
 * @param size File size
 * @param user_data User data pointer (unused)
 */
void device_definition_callback(const char* name, int type, lfs_size_t size, void* user_data);

#endif // CONFIGREAD_H