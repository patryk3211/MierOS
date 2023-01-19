#pragma once

/**
 * @brief Load and initialize a module.
 * This event will try to load and initialize a module.
 * @arg The first arg passed to this event should be the name of the module to load
 * @arg The second arg is the argument count (u8_t) passed to the module_init function.
 * @arg All arguments past this are interpreted as argc amount of char* args passed
 *      to the module_init
 */
#define EVENT_LOAD_MODULE   0x7e9b71526d87138f
#define EVENT_MOUNT_FS      0x5433905cb34219d9
