#ifndef LUATASK_H
#define LUATASK_H

#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

//--------------------------------------------------------------------+
// CDC Output Functions for Lua
//--------------------------------------------------------------------+

// CDC output functions
void cdc_write_char(char c);
void cdc_write_string(const char* str, size_t len);
void cdc_write_line(void);

//--------------------------------------------------------------------+
// Lua Task Functions
//--------------------------------------------------------------------+

// Custom sleep function for Lua
int lua_sleep(lua_State *L);

// Keyboard control functions for Lua
int lua_keypress(lua_State *L);
int lua_keyrelease(lua_State *L);
int lua_type_text(lua_State *L);

// Keyboard language functions for Lua  
int lua_set_language(lua_State *L);
int lua_get_language(lua_State *L);

// Gamepad control functions for Lua
int lua_gamepad_get_button(lua_State *L);
int lua_gamepad_get_hat(lua_State *L);
int lua_gamepad_get_x(lua_State *L);
int lua_gamepad_get_y(lua_State *L);
int lua_gamepad_get_z(lua_State *L);
int lua_gamepad_get_rz(lua_State *L);
int lua_gamepad_get_analog(lua_State *L);

// Gamepad output control functions for Lua
int lua_gamepad_press_button(lua_State *L);
int lua_gamepad_release_button(lua_State *L);
int lua_gamepad_set_analog(lua_State *L);
int lua_gamepad_set_hat(lua_State *L);
int lua_gamepad_reset(lua_State *L);

// Function to get Lua gamepad state (for USBDeviceTask.c)
void get_lua_gamepad_state(int8_t* x, int8_t* y, int8_t* z, int8_t* rz, 
                          uint8_t* hat, uint16_t* buttons, bool* active, bool* dirty);

// Register custom functions with Lua
void register_lua_functions(lua_State *L);

// FIFO Lua state management functions
void init_fifo_lua_state(void);
void execute_lua_command(const char* lua_command);
void execute_lua_file(const char* filename);

// FIFO queue processing function
void process_fifo_commands(void);

// Macro duplicate checking functions
bool can_execute_macro_external(const char* command);
bool add_macro_to_queue_list_external(const char* command);

// Main Lua task function
void task_lua_function(void *pvParameters);

extern uint8_t USB_output_switch; // USB出力切替 0:USB1, 1:UART経由

#endif // LUATASK_H