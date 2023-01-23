#pragma once

/**
 * @brief Load and initialize a module.
 * This event will try to load and initialize a module.
 * Function signature for the event looks as follows:
 * LoadModule(char* moduleName, void (*loadCallback)(Module*, void*), void* callbackArg, char** argv)
 * @param moduleName Name of the module to load, alias decoding will be performed on.
 * @param loadCallback Called when the module was loaded successfully, can be used to perform further initialization.
 * @param callbackArg Argument to be passed to the load callback function.
 * @param argv Arguments to be passed to the init function. The array should contain a null entry to terminate it.
 */
#define EVENT_LOAD_MODULE   0x7e9b71526d87138f

#define EVENT_MOUNT_FS      0x5433905cb34219d9
