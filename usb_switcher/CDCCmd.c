#include "CDCCmd.h"
#include "fstask.h"
#include "base64.h"
#include "tusb.h"
#include "bsp/board.h"
#include "pico/stdlib.h"
#include "LuaTask.h"  // For duplicate checking functions
#include "ReportParser.h"  // For interface_report_parser_info
#include "class/hid/hid.h"  // For HID_ITF_PROTOCOL_* constants
#include "host/usbh.h"  // For TinyUSB host functions
#include <stdlib.h>
#include <string.h>

// Version information
#define VERSION_STRING "USB HID Switcher v1.0.1"

// CDC command interface variables
static char cdc_command_buffer[128] = {0};
static uint8_t cdc_buffer_index = 0;
static bool cdc_connected_before = false;

// CDC receive command state variables
typedef enum {
    RECEIVE_STATE_IDLE,
    RECEIVE_STATE_READING_BASE64,
    RECEIVE_STATE_READING_PLAINTEXT
} receive_state_t;

static receive_state_t receive_state = RECEIVE_STATE_IDLE;
static char receive_filename[64] = {0};
static char* receive_base64_buffer = NULL;
static size_t receive_base64_buffer_size = 0;
static size_t receive_base64_index = 0;

// Program mode variables for plain text input
static char prog_filename[64] = {0};
static char* prog_text_buffer = NULL;
static size_t prog_text_buffer_size = 0;
static size_t prog_text_index = 0;

// FIFO buffer for run commands
#define FIFO_BUFFER_SIZE 16
#define MAX_COMMAND_LENGTH 64

typedef struct {
    char commands[FIFO_BUFFER_SIZE][MAX_COMMAND_LENGTH];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} fifo_buffer_t;

static fifo_buffer_t run_command_fifo = {0};



//--------------------------------------------------------------------+
// File Storage Functions  
//--------------------------------------------------------------------+

/**
 * Save binary data to littlefs with specified filename
 * @param filename Target filename
 * @param data Data to save
 * @param data_size Data size in bytes
 * @return 0: Success, negative value: Error
 */
static int save_file_to_littlefs(const char* filename, const uint8_t* data, size_t data_size)
{
    if (!filename || !data || data_size == 0) {
        return -1;
    }
    
    int result = fstask_write_file(filename, data, data_size);
    if (result < 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Save text file for program mode
 * @param filename Target filename
 * @param text_data Text data to save
 * @param text_size Text size in bytes
 * @return 0: Success, negative value: Error
 */
static int save_prog_text_to_littlefs(const char* filename, const char* text_data, size_t text_size)
{
    if (!filename || !text_data || text_size == 0) {
        return -1;
    }
    
    int result = fstask_write_file(filename, (const uint8_t*)text_data, text_size);
    if (result < 0) {
        return -1;
    }
    
    return 0;
}

//--------------------------------------------------------------------+
// FIFO Buffer Functions
//--------------------------------------------------------------------+

// Add command to FIFO buffer
bool fifo_push(const char* command) 
{
    if (run_command_fifo.count >= FIFO_BUFFER_SIZE) {
        return false; // Buffer full
    }
    
    // 安全な文字列コピー（null終端を保証）
    size_t len = strlen(command);
    size_t copy_len = (len < MAX_COMMAND_LENGTH - 1) ? len : MAX_COMMAND_LENGTH - 1;
    memcpy(run_command_fifo.commands[run_command_fifo.tail], command, copy_len);
    run_command_fifo.commands[run_command_fifo.tail][copy_len] = '\0';
    
    run_command_fifo.tail = (run_command_fifo.tail + 1) % FIFO_BUFFER_SIZE;
    run_command_fifo.count++;
    
    return true;
}

// Remove command from FIFO buffer
bool fifo_pop(char* command, size_t max_length) 
{
    if (run_command_fifo.count == 0) {
        return false; // Buffer empty
    }
    
    strncpy(command, run_command_fifo.commands[run_command_fifo.head], max_length - 1);
    command[max_length - 1] = '\0';
    
    run_command_fifo.head = (run_command_fifo.head + 1) % FIFO_BUFFER_SIZE;
    run_command_fifo.count--;
    
    return true;
}

// Check if FIFO buffer is empty
bool fifo_is_empty(void) 
{
    return run_command_fifo.count == 0;
}

// Get current FIFO buffer count
uint8_t fifo_get_count(void) 
{
    return run_command_fifo.count;
}

//--------------------------------------------------------------------+
// CDC Command Functions
//--------------------------------------------------------------------+

// Callback function for directory listing (used by ls command)
void cdc_file_list_callback(const char* name, int type, lfs_size_t size, void* user_data) {
    (void)user_data; // Unused parameter
    
    char line_buffer[128];
    const char* type_str = (type == LFS_TYPE_REG) ? "file" : 
                          (type == LFS_TYPE_DIR) ? " dir" : "unkn";
    
    if (type == LFS_TYPE_REG) {
        // Regular file - show size
        snprintf(line_buffer, sizeof(line_buffer), "%s  %8lu bytes  %s\r\n", type_str, (unsigned long)size, name);
    } else {
        // Directory - no size
        snprintf(line_buffer, sizeof(line_buffer), "%s      --       %s\r\n", type_str, name);
    }
    
    tud_cdc_write_str(line_buffer);
}

// Process the complete base64 data
static void process_received_base64_data(void)
{
    if (!receive_base64_buffer || receive_base64_index == 0) {
        tud_cdc_write_str("Error: No base64 data received\r\n");
        receive_state = RECEIVE_STATE_IDLE;
        return;
    }
    
    // Null terminate the base64 string
    receive_base64_buffer[receive_base64_index] = '\0';
    
    // Calculate estimated decoded size (base64 decoded size is about 3/4 of encoded size)
    size_t estimated_decoded_size = (receive_base64_index * 3 / 4) + 1024;
    
    // Allocate buffer for decoded data
    uint8_t* decoded_buffer = malloc(estimated_decoded_size);
    if (!decoded_buffer) {
        tud_cdc_write_str("Error: Failed to allocate memory for decoded data\r\n");
        free(receive_base64_buffer);
        receive_base64_buffer = NULL;
        receive_state = RECEIVE_STATE_IDLE;
        return;
    }
    
    // Decode base64 data
    int decoded_len = base64_decode(receive_base64_buffer, receive_base64_index, decoded_buffer, estimated_decoded_size);
    
    if (decoded_len < 0) {
        tud_cdc_write_str("Error: Failed to decode base64 data (");
        char error_str[32];  // バッファサイズを拡大
        snprintf(error_str, sizeof(error_str), "code: %d", decoded_len);
        tud_cdc_write_str(error_str);
        tud_cdc_write_str(")\r\n");
        free(decoded_buffer);
        free(receive_base64_buffer);
        receive_base64_buffer = NULL;
        receive_state = RECEIVE_STATE_IDLE;
        return;
    }
    
    // Save to filesystem
    int result = save_file_to_littlefs(receive_filename, decoded_buffer, (size_t)decoded_len);
    
    if (result == 0) {
        tud_cdc_write_str("Success: File '");
        tud_cdc_write_str(receive_filename);
        tud_cdc_write_str("' saved successfully (");
        char size_str[16];
        snprintf(size_str, sizeof(size_str), "%d", decoded_len);
        tud_cdc_write_str(size_str);
        tud_cdc_write_str(" bytes)\r\n");
    } else {
        tud_cdc_write_str("Error: Failed to save file to filesystem\r\n");
    }
    
    // Cleanup
    free(decoded_buffer);
    free(receive_base64_buffer);
    receive_base64_buffer = NULL;
    receive_base64_buffer_size = 0;
    receive_base64_index = 0;
    receive_state = RECEIVE_STATE_IDLE;
}

// Process the complete program mode text data
static void process_prog_text_data(void)
{
    if (!prog_text_buffer || prog_text_index == 0) {
        tud_cdc_write_str("Error: No text data received\r\n");
        receive_state = RECEIVE_STATE_IDLE;
        return;
    }
    
    // Save plain text to filesystem
    int result = save_prog_text_to_littlefs(prog_filename, prog_text_buffer, prog_text_index);
    
    if (result == 0) {
        tud_cdc_write_str("Success: File '");
        tud_cdc_write_str(prog_filename);
        tud_cdc_write_str("' saved successfully (");
        char size_str[16];
        snprintf(size_str, sizeof(size_str), "%zu", prog_text_index);
        tud_cdc_write_str(size_str);
        tud_cdc_write_str(" bytes)\r\n");
    } else {
        tud_cdc_write_str("Error: Failed to save file to filesystem\r\n");
    }
    
    // Cleanup
    free(prog_text_buffer);
    prog_text_buffer = NULL;
    prog_text_buffer_size = 0;
    prog_text_index = 0;
    receive_state = RECEIVE_STATE_IDLE;
}

// Process receive mode data (base64 only)
static void process_receive_mode_data(const char* line)
{
    if (receive_state == RECEIVE_STATE_READING_BASE64) {
        // Check for empty line (single newline ends transfer)
        if (strlen(line) == 0) {
            // Single empty line - end of base64 data
            process_received_base64_data();
            return;
        } else {
            // Non-empty line - continue processing
            
            // Accumulate base64 data
            size_t line_len = strlen(line);
            
            // Check if we need to expand the buffer
            if (receive_base64_index + line_len + 1 >= receive_base64_buffer_size) {
                size_t new_size = receive_base64_buffer_size * 2;
                char* new_buffer = realloc(receive_base64_buffer, new_size);
                if (!new_buffer) {
                    tud_cdc_write_str("Error: Failed to expand receive buffer\r\n");
                    receive_state = RECEIVE_STATE_IDLE;
                    if (receive_base64_buffer) {
                        free(receive_base64_buffer);
                        receive_base64_buffer = NULL;
                    }
                    return;
                }
                receive_base64_buffer = new_buffer;
                receive_base64_buffer_size = new_size;
            }
            
            strcpy(receive_base64_buffer + receive_base64_index, line);
            receive_base64_index += line_len;
        }
    }
}

// Process program mode data (plain text)
static void process_prog_mode_data(const char* line)
{
    if (receive_state == RECEIVE_STATE_READING_PLAINTEXT) {
        // Accumulate plain text data
        size_t line_len = strlen(line);
        
        // Check if we need to expand the buffer
        if (prog_text_index + line_len + 2 >= prog_text_buffer_size) { // +2 for \r\n
            size_t new_size = prog_text_buffer_size * 2;
            char* new_buffer = realloc(prog_text_buffer, new_size);
            if (!new_buffer) {
                tud_cdc_write_str("Error: Failed to expand program buffer\r\n");
                receive_state = RECEIVE_STATE_IDLE;
                if (prog_text_buffer) {
                    free(prog_text_buffer);
                    prog_text_buffer = NULL;
                }
                return;
            }
            prog_text_buffer = new_buffer;
            prog_text_buffer_size = new_size;
        }
        
        // Add line to buffer
        if (line_len > 0) {
            strcpy(prog_text_buffer + prog_text_index, line);
            prog_text_index += line_len;
        }
        
        // Add newline (use \n for simplicity)
        prog_text_buffer[prog_text_index] = '\n';
        prog_text_index++;
    }
}

// CDC command processing function
void process_cdc_command(const char* command)
{
    if (strcmp(command, "version") == 0) {
        tud_cdc_write_str(VERSION_STRING);
        tud_cdc_write_str("\r\n");
    } else if (strncmp(command, "run ", 4) == 0) {
        // Extract filename after "run "
        const char* filename = command + 4;
        
        // Skip leading spaces
        while (*filename == ' ') filename++;
        
        if (strlen(filename) > 0) {
            // Create a special command to indicate file execution
            char file_command[MAX_COMMAND_LENGTH];
            snprintf(file_command, sizeof(file_command), "FILE:%s", filename);
            
            // Use enhanced fifo_push with duplicate checking
            if (fifo_push_with_duplicate_check(file_command)) {
                tud_cdc_write_str("Lua script file '");
                tud_cdc_write_str(filename);
                tud_cdc_write_str("' added to queue (");
                
                // Show current queue count
                char count_str[8];
                snprintf(count_str, sizeof(count_str), "%d", fifo_get_count());
                tud_cdc_write_str(count_str);
                tud_cdc_write_str("/");
                snprintf(count_str, sizeof(count_str), "%d", FIFO_BUFFER_SIZE);
                tud_cdc_write_str(count_str);
                tud_cdc_write_str(")\r\n");
            } else {
                // Check if it was rejected due to duplicate or queue full
                if (fifo_get_count() >= FIFO_BUFFER_SIZE) {
                    tud_cdc_write_str("Error: Command queue is full\r\n");
                } else {
                    tud_cdc_write_str("Script '");
                    tud_cdc_write_str(filename);
                    tud_cdc_write_str("' is already executing or queued\r\n");
                }
            }
        } else {
            tud_cdc_write_str("Error: run command requires a filename\r\n");
            tud_cdc_write_str("Usage: run <filename>\r\n");
        }
    } else if (strcmp(command, "queue") == 0) {
        // Show current queue status
        char count_str[8];
        snprintf(count_str, sizeof(count_str), "%d", fifo_get_count());
        tud_cdc_write_str("Queue status: ");
        tud_cdc_write_str(count_str);
        tud_cdc_write_str("/");
        snprintf(count_str, sizeof(count_str), "%d", FIFO_BUFFER_SIZE);
        tud_cdc_write_str(count_str);
        tud_cdc_write_str(" commands\r\n");
    } else if (strcmp(command, "ls") == 0) {
        // List filesystem contents
        tud_cdc_write_str("Directory listing:\r\n");
        tud_cdc_write_str("Type     Size       Name\r\n");
        tud_cdc_write_str("------------------------\r\n");
        
        int result = fstask_list_directory(cdc_file_list_callback, NULL);
        if (result < 0) {
            tud_cdc_write_str("Error: Failed to list directory contents\r\n");
        } else {
            tud_cdc_write_str("------------------------\r\n");
        }
    } else if (strncmp(command, "rm ", 3) == 0) {
        // Remove file command
        const char* filename = command + 3;
        
        // Skip leading spaces
        while (*filename == ' ') filename++;
        
        if (strlen(filename) > 0) {
            int result = fstask_remove_file(filename);
            if (result == 0) {
                tud_cdc_write_str("File '");
                tud_cdc_write_str(filename);
                tud_cdc_write_str("' removed successfully\r\n");
            } else {
                tud_cdc_write_str("Error: Failed to remove file '");
                tud_cdc_write_str(filename);
                tud_cdc_write_str("'\r\n");
                
                // Provide more specific error messages
                if (result == -2) { // LFS_ERR_NOENT
                    tud_cdc_write_str("Reason: File not found\r\n");
                } else if (result == -1) {
                    tud_cdc_write_str("Reason: Invalid file or filesystem error\r\n");
                } else {
                    char error_str[32];
                    snprintf(error_str, sizeof(error_str), "Reason: LFS error code %d\r\n", result);
                    tud_cdc_write_str(error_str);
                }
            }
        } else {
            tud_cdc_write_str("Error: rm command requires a filename\r\n");
            tud_cdc_write_str("Usage: rm <filename>\r\n");
        }
    } else if (strncmp(command, "cat ", 4) == 0) {
        // Display file contents command
        const char* filename = command + 4;
        
        // Skip leading spaces
        while (*filename == ' ') filename++;
        
        if (strlen(filename) > 0) {
            // Try to read the file
            char* file_buffer = malloc(4096); // 4KB buffer for file reading
            if (!file_buffer) {
                tud_cdc_write_str("Error: Failed to allocate memory for file reading\r\n");
            } else {
                int bytes_read = fstask_read_file(filename, file_buffer, 4095);
                if (bytes_read >= 0) {
                    file_buffer[bytes_read] = '\0'; // Null terminate
                    
                    tud_cdc_write_str("--- Content of '");
                    tud_cdc_write_str(filename);
                    tud_cdc_write_str("' ---\r\n");
                    
                    // Output file content, handling binary data safely
                    for (int i = 0; i < bytes_read; i++) {
                        char ch = file_buffer[i];
                        if (ch >= 0x20 && ch <= 0x7E) {
                            // Printable ASCII character
                            tud_cdc_write(&ch, 1);
                        } else if (ch == '\r') {
                            tud_cdc_write_str("\r");
                        } else if (ch == '\n') {
                            tud_cdc_write_str("\n");
                        } else if (ch == '\t') {
                            tud_cdc_write_str("    "); // Replace tab with 4 spaces
                        } else {
                            // Non-printable character - show as hex
                            char hex_str[8];
                            snprintf(hex_str, sizeof(hex_str), "\\x%02X", (uint8_t)ch);
                            tud_cdc_write_str(hex_str);
                        }
                    }
                    
                    tud_cdc_write_str("\r\n--- End of file (");
                    char size_str[16];
                    snprintf(size_str, sizeof(size_str), "%d", bytes_read);
                    tud_cdc_write_str(size_str);
                    tud_cdc_write_str(" bytes) ---\r\n");
                } else {
                    tud_cdc_write_str("Error: Failed to read file '");
                    tud_cdc_write_str(filename);
                    tud_cdc_write_str("'\r\n");
                    
                    // Provide more specific error messages
                    if (bytes_read == -2) { // LFS_ERR_NOENT
                        tud_cdc_write_str("Reason: File not found\r\n");
                    } else if (bytes_read == -1) {
                        tud_cdc_write_str("Reason: Filesystem not mounted or invalid file\r\n");
                    } else {
                        char error_str[64];  // バッファサイズを拡大
                        snprintf(error_str, sizeof(error_str), "Reason: LFS error code %d\r\n", (int)bytes_read);
                        tud_cdc_write_str(error_str);
                    }
                }
                free(file_buffer);
            }
        } else {
            tud_cdc_write_str("Error: cat command requires a filename\r\n");
            tud_cdc_write_str("Usage: cat <filename>\r\n");
        }
    } else if (strncmp(command, "receive ", 8) == 0 || strncmp(command, "rcv ", 4) == 0) {
        // Start receive mode with filename (receive or rcv command)
        const char* filename;
        const char* cmd_name;
        
        if (strncmp(command, "receive ", 8) == 0) {
            filename = command + 8;
            cmd_name = "receive";
        } else {
            filename = command + 4;
            cmd_name = "rcv";
        }
        
        // Skip leading spaces
        while (*filename == ' ') filename++;
        
        if (strlen(filename) == 0) {
            tud_cdc_write_str("Error: ");
            tud_cdc_write_str(cmd_name);
            tud_cdc_write_str(" command requires a filename\r\n");
            tud_cdc_write_str("Usage: ");
            tud_cdc_write_str(cmd_name);
            tud_cdc_write_str(" <filename>\r\n");
            return;
        }
        
        if (receive_state != RECEIVE_STATE_IDLE) {
            tud_cdc_write_str("Error: Already in receive mode\r\n");
            return;
        }
        
        // Set filename and start receive mode
        strncpy(receive_filename, filename, sizeof(receive_filename) - 1);
        receive_filename[sizeof(receive_filename) - 1] = '\0';
        
        receive_state = RECEIVE_STATE_READING_BASE64;
        receive_base64_index = 0;
        
        // Free previous buffer if exists
        if (receive_base64_buffer) {
            free(receive_base64_buffer);
            receive_base64_buffer = NULL;
            receive_base64_buffer_size = 0;
        }
        
        // Allocate buffer for base64 data (start with 8KB, can grow)
        receive_base64_buffer_size = 8192;
        receive_base64_buffer = malloc(receive_base64_buffer_size);
        if (!receive_base64_buffer) {
            tud_cdc_write_str("Error: Failed to allocate memory for receive buffer\r\n");
            receive_state = RECEIVE_STATE_IDLE;
            return;
        }
        
        tud_cdc_write_str("Ready to receive file '");
        tud_cdc_write_str(receive_filename);
        tud_cdc_write_str("'.\r\n");
        tud_cdc_write_str("Send base64 data. Send empty line to finish.\r\n");
        return; // Don't show prompt
    } else if (strncmp(command, "lang ", 5) == 0) {
        // Set keyboard language command
        const char* lang = command + 5;
        
        // Skip leading spaces
        while (*lang == ' ') lang++;
        
        if (strlen(lang) > 0) {
            // Create a Lua command to set the language
            char lua_command[MAX_COMMAND_LENGTH];
            snprintf(lua_command, sizeof(lua_command), "lang('%s')", lang);
            
            if (fifo_push(lua_command)) {
                tud_cdc_write_str("Language setting '");
                tud_cdc_write_str(lang);
                tud_cdc_write_str("' added to queue\r\n");
            } else {
                tud_cdc_write_str("Error: Command queue is full\r\n");
            }
        } else {
            tud_cdc_write_str("Error: lang command requires a language\r\n");
            tud_cdc_write_str("Usage: lang <language>\r\n");
            tud_cdc_write_str("Supported: us (US English), ja (Japanese)\r\n");
        }
    } else if (strcmp(command, "lang") == 0) {
        // Show current language - add to Lua queue for execution
        if (fifo_push("print('Current language: ' .. getlang())")) {
            // Command queued successfully
        } else {
            tud_cdc_write_str("Error: Command queue is full\r\n");
        }
    } else if (strcmp(command, "list") == 0) {
        // List connected USB Host devices
        tud_cdc_write_str("Connected USB Host devices:\r\n");
        tud_cdc_write_str("Addr Inst  VID:PID   Type       Status\r\n");
        tud_cdc_write_str("-----------------------------------\r\n");
        
        bool found_devices = false;
        
        // Iterate through all possible device slots
        for (uint8_t i = 0; i < CFG_TUH_HID; i++) {
            if (interface_report_parser_info[i].is_valid) {
                found_devices = true;
                
                // Format device information
                char device_line[128];
                const char* device_type = "Unknown";
                
                // Determine device type based on HID protocol
                if (tuh_hid_mounted(interface_report_parser_info[i].dev_addr, interface_report_parser_info[i].instance)) {
                    uint8_t itf_protocol = tuh_hid_interface_protocol(interface_report_parser_info[i].dev_addr, interface_report_parser_info[i].instance);
                    switch (itf_protocol) {
                        case HID_ITF_PROTOCOL_KEYBOARD:
                            device_type = "Keyboard";
                            break;
                        case HID_ITF_PROTOCOL_MOUSE:
                            device_type = "Mouse";
                            break;
                        case HID_ITF_PROTOCOL_NONE:
                        default:
                            device_type = "Gamepad/HID";
                            break;
                    }
                }
                
                snprintf(device_line, sizeof(device_line), " %2u   %2u   %04X:%04X %-10s %s\r\n",
                    interface_report_parser_info[i].dev_addr,
                    interface_report_parser_info[i].instance,
                    interface_report_parser_info[i].vid,
                    interface_report_parser_info[i].pid,
                    device_type,
                    tuh_hid_mounted(interface_report_parser_info[i].dev_addr, interface_report_parser_info[i].instance) ? "Connected" : "Disconnected"
                );
                
                tud_cdc_write_str(device_line);
            }
        }
        
        if (!found_devices) {
            tud_cdc_write_str("No USB Host devices detected\r\n");
        }
        
        tud_cdc_write_str("-----------------------------------\r\n");
    } else if (strncmp(command, "prog ", 5) == 0) {
        // Start program mode with filename (plain text input)
        const char* filename = command + 5;
        
        // Skip leading spaces
        while (*filename == ' ') filename++;
        
        if (strlen(filename) == 0) {
            tud_cdc_write_str("Error: prog command requires a filename\r\n");
            tud_cdc_write_str("Usage: prog <filename>\r\n");
            return;
        }
        
        if (receive_state != RECEIVE_STATE_IDLE) {
            tud_cdc_write_str("Error: Already in receive mode\r\n");
            return;
        }
        
        // Set filename and start program mode
        strncpy(prog_filename, filename, sizeof(prog_filename) - 1);
        prog_filename[sizeof(prog_filename) - 1] = '\0';
        
        receive_state = RECEIVE_STATE_READING_PLAINTEXT;
        prog_text_index = 0;
        
        // Free previous buffer if exists
        if (prog_text_buffer) {
            free(prog_text_buffer);
            prog_text_buffer = NULL;
            prog_text_buffer_size = 0;
        }
        
        // Allocate buffer for plain text data (start with 4KB, can grow)
        prog_text_buffer_size = 4096;
        prog_text_buffer = malloc(prog_text_buffer_size);
        if (!prog_text_buffer) {
            tud_cdc_write_str("Error: Failed to allocate memory for program buffer\r\n");
            receive_state = RECEIVE_STATE_IDLE;
            return;
        }
        
        tud_cdc_write_str("Ready to receive program file '");
        tud_cdc_write_str(prog_filename);
        tud_cdc_write_str("'.\r\n");
        tud_cdc_write_str("Enter plain text. Press Ctrl-D to save and finish.\r\n");
        return; // Don't show prompt
    } else if (strlen(command) > 0) {
        tud_cdc_write_str("Unknown command: ");
        tud_cdc_write_str(command);
        tud_cdc_write_str("\r\n");
        tud_cdc_write_str("Available commands: version, run <filename>, queue, ls, rm <filename>, cat <filename>, receive <filename>, rcv <filename>, prog <filename>, list\r\n");
    }
    
    // Show prompt
    tud_cdc_write_str("> ");
    tud_cdc_write_flush();
}

//--------------------------------------------------------------------+
// Public Functions
//--------------------------------------------------------------------+

void cdc_cmd_task(void)
{
    // Check if CDC is connected
    if ( tud_cdc_connected() )
    {
        // Send welcome message only once when first connected
        if (!cdc_connected_before) {
            cdc_connected_before = true;
            
            // Create complete welcome message in buffer to avoid CDC overflow
            static const char welcome_msg[] = 
                "\r\n"
                "=== USB HID Switcher Command Interface ===\r\n"
                VERSION_STRING
                "Available commands:\r\n"
                "  version        - Show version information\r\n"
                "  run <filename> - Execute Lua script file from LittleFS\r\n"
                "  queue          - Show queue status\r\n"
                "  ls             - List filesystem contents\r\n"
                "  rm <filename>  - Remove specified file\r\n"
                "  cat <filename> - Display file contents\r\n"
                "  receive <filename> - File receive mode (Base64)\r\n"
                "  rcv <filename>     - Alias for receive command\r\n"
                "  prog <filename>    - Program mode (plain text input, Ctrl-D to save)\r\n"
                "  list           - Show connected USB Host devices\r\n"
                "> ";

            tud_cdc_write(welcome_msg, sizeof(welcome_msg));
            tud_cdc_write_flush();
        }
        
        // Process incoming data if available
        if ( tud_cdc_available() )
        {
            // Read character by character
            char ch;
            uint32_t count = tud_cdc_read(&ch, 1);
            
            if (count > 0) {
                if (ch == 0x04) {
                    // Ctrl-D - End program mode if active
                    if (receive_state == RECEIVE_STATE_READING_PLAINTEXT) {
                        tud_cdc_write_str("^D\r\n");
                        process_prog_text_data();
                        tud_cdc_write_str("> ");
                        tud_cdc_write_flush();
                        cdc_buffer_index = 0;
                        memset(cdc_command_buffer, 0, sizeof(cdc_command_buffer));
                    }
                    // Ignore Ctrl-D in other modes
                } else if (ch == '\r' || ch == '\n') {
                    // End of line - process it
                    cdc_command_buffer[cdc_buffer_index] = '\0';
                    tud_cdc_write_str("\r\n");
                    tud_cdc_write_flush();
                    
                    if (receive_state == RECEIVE_STATE_READING_BASE64) {
                        // In base64 receive mode - process data differently
                        process_receive_mode_data(cdc_command_buffer);
                        
                        // Show prompt only when back to idle state
                        if (receive_state == RECEIVE_STATE_IDLE) {
                            tud_cdc_write_str("> ");
                            tud_cdc_write_flush();
                        }
                        // No prompt in base64 reading mode
                    } else if (receive_state == RECEIVE_STATE_READING_PLAINTEXT) {
                        // In program mode - process plain text
                        process_prog_mode_data(cdc_command_buffer);
                        // No prompt in program mode - continue reading until Ctrl-D
                    } else if (cdc_buffer_index > 0) {
                        // Normal command processing
                        process_cdc_command(cdc_command_buffer);
                    } else {
                        // Empty command - just show prompt
                        tud_cdc_write_str("> ");
                        tud_cdc_write_flush();
                    }
                    
                    cdc_buffer_index = 0;
                    memset(cdc_command_buffer, 0, sizeof(cdc_command_buffer));
                } else if (ch == '\b' || ch == 0x7F) {
                    // Backspace - remove last character
                    if (cdc_buffer_index > 0) {
                        cdc_buffer_index--;
                        cdc_command_buffer[cdc_buffer_index] = '\0';
                        tud_cdc_write_str("\b \b");  // Backspace, space, backspace
                        tud_cdc_write_flush();
                    }
                } else if (ch >= 0x20 && ch <= 0x7E) {
                    // Printable character
                    if (cdc_buffer_index < sizeof(cdc_command_buffer) - 1) {
                        cdc_command_buffer[cdc_buffer_index] = ch;
                        cdc_buffer_index++;
                        tud_cdc_write(&ch, 1);  // Echo character
                        tud_cdc_write_flush();
                    }
                }
                // Ignore other control characters
            }
        }
    } else {
        // Reset connection state when disconnected
        if (cdc_connected_before) {
            cdc_connected_before = false;
            cdc_buffer_index = 0;
            memset(cdc_command_buffer, 0, sizeof(cdc_command_buffer));
            
            // Reset receive state on disconnect
            receive_state = RECEIVE_STATE_IDLE;
            if (receive_base64_buffer) {
                free(receive_base64_buffer);
                receive_base64_buffer = NULL;
                receive_base64_buffer_size = 0;
            }
            if (prog_text_buffer) {
                free(prog_text_buffer);
                prog_text_buffer = NULL;
                prog_text_buffer_size = 0;
            }
        }
    }
}