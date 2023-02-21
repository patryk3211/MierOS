#pragma once

/**
 * @brief Null event
 */
#define EVENT_NULL              0

/**
 * @brief Load and initialize a module.
 *
 * This event will try to load and initialize a module.
 * Function signature for the event looks as follows:
 * LoadModule(char* moduleName, void (*loadCallback)(Module*, void*), void* callbackArg, char** argv)
 *
 * @param moduleName Name of the module to load, alias decoding will be performed on.
 * @param loadCallback Called when the module was loaded successfully, can be used to perform further initialization.
 * @param callbackArg Argument to be passed to the load callback function. It has to be a void*
 * @param argv Arguments to be passed to the init function. The array should contain a null entry to terminate it. It is an array of char*
 */
#define EVENT_LOAD_MODULE       0x7e9b71526d87138f

/**
 * @brief Synchronize event.
 *
 * This event will fire the given callback when all events before it
 * are processed. It will also make sure that no events after it are
 * processed before it is.
 *
 * @param callback Callback to call
 * @param callbackArg An argument to be passed to the callback
 */
#define EVENT_SYNC              0x5ce90a4b4133c36a

/**
 * @brief Event queue empty event.
 *
 * This event is fired by the event manager when it has processed all
 * events on it's queue. It does not provide any arguments.
 */
#define EVENT_QUEUE_EMPTY       0x2801da0cf10a2d25

/**
 * @brief Event processed event.
 *
 * This event is fired by the event manager when it finishes processing an event.
 * It provides 2 arguments, both of them are an unsigned 64 bit integer (u64_t).
 *
 * @param eventIdentifier The first argument is the unique event id that has been processed.
 * @param localEventId The second argument is an event id that was assigned during the event raise call.
 */
#define EVENT_PROCESSED_EVENT   0x9bbe5456af663f0f

/**
 * @brief Userspace event identifier.
 *
 * Used mostly for signalling the completion of a userspace event.
 */
#define EVENT_USERSPACE_EVENT   0x97ed4ff89bc5cc08

