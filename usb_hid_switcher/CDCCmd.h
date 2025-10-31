#ifndef CDCCMD_H
#define CDCCMD_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * CDC command interface task function
 * This function handles CDC command interface including:
 * - Command parsing and execution
 * - File receive functionality (base64 decode)
 * - Terminal interface with command history
 * 
 * Should be called regularly from main loop
 */
void cdc_cmd_task(void);

// FIFO buffer functions (accessed from LuaTask)
bool fifo_push(const char* command);
bool fifo_pop(char* command, size_t max_length);
bool fifo_is_empty(void);
uint8_t fifo_get_count(void);

// Enhanced FIFO functions with duplicate checking
bool fifo_push_with_duplicate_check(const char* command);
bool is_macro_in_queue_or_executing(const char* macro_name);

#endif // CDCCMD_H