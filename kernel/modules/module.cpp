/**
 * @brief Module only things
 *
 * This file contains functions and variables which need
 * to be compiled into a module for it to work.
 */

#include <types.h>
#include <defines.h>

extern "C" void atexit(void (*)()) { }

SECTION(".rodata") u16_t major = 0;
