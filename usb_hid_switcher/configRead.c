#include "configRead.h"
#include "fstask.h"
#include "USBHostTask.h" // For USE_DEVICE_PROTOCOL definition
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tusb.h" // For HID_PROTOCOL_BOOT and HID_PROTOCOL_REPORT constants
#include "ReportParser.h"
#include "MouseReportParser.h"

// Global counter for defined_report_parser_info array
static int defined_parser_count = 0;

// External declaration for defined_report_parser_info
extern defined_mouse_report_parser_info_t defined_report_parser_info[99];

/**
 * Function to read configuration file from littlefs and parse settings
 * @return 0: 成功, 負の値: エラー
 */
int read_config_file(void) {
    char config_buffer[256];
    int bytes_read;
    
    printf("Reading configuration file...\n");
    
    // Read the config file
    bytes_read = fstask_read_file("config", config_buffer, sizeof(config_buffer) - 1);
    
    if (bytes_read < 0) {
        printf("Failed to read config file: %d\n", bytes_read);
        printf("Using default settings\n");
        return bytes_read;
    }
    
    // Null terminate the buffer
    config_buffer[bytes_read] = '\0';
        
    // Parse the PROTOCOL and DEVICEID settings
    char *line = strtok(config_buffer, "\n\r");
    while (line != NULL) {
        // Skip empty lines and comments
        if (line[0] != '\0' && line[0] != '#') {
            // Look for PROTOCOL= setting
            if (strncmp(line, "PROTOCOL=", 9) == 0) {
                char *value = line + 9; // Skip "PROTOCOL="
                
                // Remove any trailing whitespace
                char *end = value + strlen(value) - 1;
                while (end > value && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
                    end--;
                }
                *(end + 1) = '\0';
                
                if (strcmp(value, "BOOT") == 0) {
                    printf("Protocol setting: BOOT mode\n");
                    default_hid_protocol = HID_PROTOCOL_BOOT;
                } else if (strcmp(value, "REPORT") == 0) {
                    printf("Protocol setting: REPORT mode\n");
                    default_hid_protocol = HID_PROTOCOL_REPORT;
                } else {
                    printf("Unknown protocol setting: %s (using default REPORT mode)\n", value);
                }
            }
            // Look for DEVICEID= setting
            else if (strncmp(line, "DEVICEID=", 9) == 0) {
                char *value = line + 9; // Skip "DEVICEID="
                
                // Remove any trailing whitespace
                char *end = value + strlen(value) - 1;
                while (end > value && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
                    end--;
                }
                *(end + 1) = '\0';
                
                // Parse the device ID as integer
                int parsed_id = atoi(value);
                if (parsed_id != 0 || strcmp(value, "0") == 0) {
                    device_id = parsed_id;
                    printf("Device ID setting: %d\n", device_id);
                } else {
                    printf("Invalid device ID setting: %s (keeping default: %d)\n", value, device_id);
                }
            }
        }
        line = strtok(NULL, "\n\r");
    }
    
    return 0;
}

/**
 * Parse mouse device definition file and create mouse_report_parser_info_t
 */
int parse_mouse_definition(const char* content, uint16_t vid, uint16_t pid) {
    static mouse_report_parser_info_t mouse_parser_storage[99]; // Static storage for parsed info
    static int storage_count = 0;
    
    if (defined_parser_count >= 99 || storage_count >= 99) {
        printf("Error: Too many mouse parsers defined\n");
        return -1;
    }
    
    defined_mouse_report_parser_info_t* parser_entry = &defined_report_parser_info[defined_parser_count];
    mouse_report_parser_info_t* parser = &mouse_parser_storage[storage_count];
    
    // Initialize parser entry
    parser_entry->is_valid = true;
    parser_entry->vid = vid;
    parser_entry->pid = pid;
    parser_entry->protocol = PROTOCOL; // Default to REPORT protocol
    parser_entry->dev_addr = 0; // Will be set when device connects
    parser_entry->instance = 0; // Will be set when device connects
    parser_entry->parser_info = parser; // Set pointer to storage
    
    // Initialize parser with default values
    parser->ReportID = 0xffff;
    parser->buttons_index = 0xffff;
    parser->buttons_bitpos = 0;
    parser->buttons_size = 0;
    parser->x_index = 0xffff;
    parser->x_bitpos = 0;
    parser->x_size = 0;
    parser->y_index = 0xffff;
    parser->y_bitpos = 0;
    parser->y_size = 0;
    parser->wheel_index = 0xffff;
    parser->wheel_bitpos = 0;
    parser->wheel_size = 0;
    parser->pan_index = 0xffff;
    parser->pan_bitpos = 0;
    parser->pan_size = 0;
    
    printf("Parsing mouse definition for %04x:%04x\n", vid, pid);
    
    // Parse line by line
    char* content_copy = malloc(strlen(content) + 1);
    if (content_copy == NULL) {
        printf("Failed to allocate memory for content parsing\n");
        return -1;
    }
    strcpy(content_copy, content);
    
    char* line = strtok(content_copy, "\n\r");
    while (line != NULL) {
        // Skip empty lines and comments
        if (line[0] == '#') {
        } else if (line[0] != '\0') {
            if (strncmp(line, "PROTOCOL ", 9) == 0) {
                char* protocol_str = line + 9;
                // Remove any trailing whitespace
                char* end = protocol_str + strlen(protocol_str) - 1;
                while (end > protocol_str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
                    end--;
                }
                *(end + 1) = '\0';
                
                if (strcmp(protocol_str, "BOOT") == 0) {
                    parser_entry->protocol = BOOT;
                } else if (strcmp(protocol_str, "REPORT") == 0) {
                    parser_entry->protocol = PROTOCOL;
                } else {
                    parser_entry->protocol = PROTOCOL;
                }
            }
            else if (strncmp(line, "ReportID", 8) == 0) {
                // Find the first non-space character after ReportID
                char* data_start = line + 8;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                
                // Check for special case FFFF (no ReportID)
                if (strncmp(data_start, "FFFF", 4) == 0 || strncmp(data_start, "ffff", 4) == 0) {
                    parser->ReportID = 0xffff;
                } else {
                    int report_id;
                    if (sscanf(data_start, "%x", &report_id) == 1) {
                        parser->ReportID = (uint16_t)report_id;
                    } else {
                    }
                }
            }
            else if (strncmp(line, "BUTTON", 6) == 0) {
                int index, bitpos, size;
                // Find the first digit after BUTTON (skip any spaces)
                char* data_start = line + 6;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                if (sscanf(data_start, "%d %d %d", &index, &bitpos, &size) == 3) {
                    parser->buttons_index = (uint16_t)index;
                    parser->buttons_bitpos = (uint8_t)bitpos;
                    parser->buttons_size = (uint8_t)size;
                }
            }
            else if (strncmp(line, "X", 1) == 0 && (line[1] == ' ' || line[1] == '\t')) {
                int index, bitpos, size;
                // Find the first digit after X (skip any spaces)
                char* data_start = line + 1;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                if (sscanf(data_start, "%d %d %d", &index, &bitpos, &size) == 3) {
                    parser->x_index = (uint16_t)index;
                    parser->x_bitpos = (uint8_t)bitpos;
                    parser->x_size = (uint8_t)size;
                }
            }
            else if (strncmp(line, "Y", 1) == 0 && (line[1] == ' ' || line[1] == '\t')) {
                int index, bitpos, size;
                // Find the first digit after Y (skip any spaces)
                char* data_start = line + 1;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                if (sscanf(data_start, "%d %d %d", &index, &bitpos, &size) == 3) {
                    parser->y_index = (uint16_t)index;
                    parser->y_bitpos = (uint8_t)bitpos;
                    parser->y_size = (uint8_t)size;
                }
            }
            else if (strncmp(line, "WHEEL", 5) == 0) {
                int index, bitpos, size;
                // Find the first digit after WHEEL (skip any spaces)
                char* data_start = line + 5;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                if (sscanf(data_start, "%d %d %d", &index, &bitpos, &size) == 3) {
                    parser->wheel_index = (uint16_t)index;
                    parser->wheel_bitpos = (uint8_t)bitpos;
                    parser->wheel_size = (uint8_t)size;
                }
            }
            else if (strncmp(line, "PAN", 3) == 0) {
                int index, bitpos, size;
                // Find the first digit after PAN (skip any spaces)
                char* data_start = line + 3;
                while (*data_start == ' ' || *data_start == '\t') data_start++;
                if (sscanf(data_start, "%d %d %d", &index, &bitpos, &size) == 3) {
                    parser->pan_index = (uint16_t)index;
                    parser->pan_bitpos = (uint8_t)bitpos;
                    parser->pan_size = (uint8_t)size;
                }
            }
        }
        line = strtok(NULL, "\n\r");
    }
    
    free(content_copy);
    storage_count++;
    defined_parser_count++;
    
    return defined_parser_count - 1; // Return the index of the created parser
}

// Callback function for directory listing
void device_definition_callback(const char* name, int type, lfs_size_t size, void* user_data) {
    // Log all files found for debugging
    
    // Check if this is a file (not directory) and matches the pattern
    if (type == LFS_TYPE_REG) {
        // Check for MOUSE-xxxx:yyyy pattern
        if (strncmp(name, "MOUSE-", 6) == 0) {
            // Look for colon separator in the expected position
            char* colon_pos = strchr(name + 6, ':');
            if (colon_pos != NULL) {
                // Calculate VID and PID lengths
                int vid_len = colon_pos - (name + 6);
                int pid_len = strlen(colon_pos + 1);
                
                // VID and PID should both be 4 hex digits
                if (vid_len == 4 && pid_len == 4) {
                    
                    // Extract VID and PID from filename
                    uint16_t vid, pid;
                    char vid_str[5] = {0};
                    char pid_str[5] = {0};
                    strncpy(vid_str, name + 6, 4);
                    strncpy(pid_str, colon_pos + 1, 4);
                    
                    if (sscanf(vid_str, "%hx", &vid) == 1 && sscanf(pid_str, "%hx", &pid) == 1) {
                        
                        // Allocate buffer and read the file
                        char* buffer = malloc(size + 1);
                        if (buffer != NULL) {
                            int bytes_read = fstask_read_file(name, buffer, size);
                            if (bytes_read > 0) {
                                buffer[bytes_read] = '\0';
                                // Parse the mouse definition and create parser
                                int parser_index = parse_mouse_definition(buffer, vid, pid);
                                if (parser_index >= 0) {
                                } else {
                                    printf("Failed to create mouse parser\n");
                                }
                            } else {
                                printf("Failed to read device definition file '%s': %d\n", name, bytes_read);
                            }
                            free(buffer);
                        } else {
                            printf("Failed to allocate memory for device definition file '%s'\n", name);
                        }
                    } else {
                        printf("Failed to parse VID/PID from filename '%s'\n", name);
                    }
                }
            }
        }
        // Check for KEYBOARD-xxxx:yyyy pattern  
        else if (strncmp(name, "KEYBOARD-", 9) == 0) {
            // Look for colon separator in the expected position
            char* colon_pos = strchr(name + 9, ':');
            if (colon_pos != NULL) {
                // Calculate VID and PID lengths
                int vid_len = colon_pos - (name + 9);
                int pid_len = strlen(colon_pos + 1);
                printf("Checking KEYBOARD pattern: VID length=%d, PID length=%d\n", vid_len, pid_len);
                
                // VID and PID should both be 4 hex digits
                if (vid_len == 4 && pid_len == 4) {
                    printf("Found keyboard device definition: %s (size: %d bytes)\n", name, (int)size);
                    
                    // Allocate buffer and read the file
                    char* buffer = malloc(size + 1);
                    if (buffer != NULL) {
                        int bytes_read = fstask_read_file(name, buffer, size);
                        if (bytes_read > 0) {
                            buffer[bytes_read] = '\0';
                            printf("Device definition file '%s' contents:\n%s\n", name, buffer);
                        } else {
                            printf("Failed to read device definition file '%s': %d\n", name, bytes_read);
                        }
                        free(buffer);
                    } else {
                        printf("Failed to allocate memory for device definition file '%s'\n", name);
                    }
                }
            }
        }
    }
}

/**
 * Function to scan and read all device definition files at startup
 * @return 0: 成功, 負の値: エラー
 */
int scan_and_read_device_definitions(void) {
    printf("Scanning for device definition files...\n");
    
    int result = fstask_list_directory(device_definition_callback, NULL);
    
    if (result < 0) {
        printf("Failed to list directory: %d\n", result);
        return result;
    }
    printf("Total device parsers loaded: %d\n", defined_parser_count);
    return 0;
}

/**
 * Function to find a device parser by VID and PID
 * @param vid Vendor ID
 * @param pid Product ID
 * @return Pointer to parser_info or NULL if not found
 */
void* find_device_parser(uint16_t vid, uint16_t pid) {
    for (int i = 0; i < defined_parser_count && i < 99; i++) {
        if (defined_report_parser_info[i].is_valid && 
            defined_report_parser_info[i].vid == vid && 
            defined_report_parser_info[i].pid == pid) {
            return defined_report_parser_info[i].parser_info;
        }
    }
    
    return NULL;
}