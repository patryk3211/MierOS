#ifndef _LIBOPT_LIBOPT_H
#define _LIBOPT_LIBOPT_H

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct option {
    char short_opt;
    const char* long_opt;
    int arg_count;
};

struct argument_parsing_context {
    const struct option* options;
    int argc;
    char** argv;

    int currentArg;
    char* currentShortNameParse;

    char** currentOptArgPtr;
};

/**
 * @brief Create argument parsing context
 *
 * @param options Option array
 * @param argc Argument count
 * @param argv Argument values
 * @return Parsing context
 */
extern struct argument_parsing_context* start_arg_parse(const struct option* options, int argc, char* argv[]);

/**
 * @brief Get next option
 *
 * @param ctx Context returned from start_arg_parse
 * @return short_opt for any valid option, 0 for normal arguments, -1 on end, -2 for unknown arguments
 */
extern char get_next_option(struct argument_parsing_context* ctx);

/**
 * @brief Get option argument
 *
 * @param ctx Parsing context
 * @return Pointer to current option's arguments
 */
extern char** get_option_arguments(struct argument_parsing_context* ctx);

/**
 * @brief Finish argument parsing
 *
 * @param ctx Parsing context
 */
extern void finish_arg_parse(struct argument_parsing_context* ctx);


#if defined(__cplusplus)
}
#endif

#endif // _LIBOPT_LIBOPT_H

