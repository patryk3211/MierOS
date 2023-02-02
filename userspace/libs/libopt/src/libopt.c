#include <libopt.h>

#include <stdlib.h>
#include <string.h>

struct argument_parsing_context* start_arg_parse(const struct option* options, int argc, char* argv[]) {
    struct argument_parsing_context* ctx = (struct argument_parsing_context*)malloc(sizeof(struct argument_parsing_context));

    ctx->argc = argc;
    ctx->argv = argv;
    ctx->options = options;

    ctx->currentArg = 1;
    ctx->currentShortNameParse = 0;

    ctx->currentOptArgPtr = 0;

    return ctx;
}

char get_next_option(struct argument_parsing_context* ctx) {
    if(ctx->currentArg >= ctx->argc)
        return -1;
   
shortOptParse:
    if(ctx->currentShortNameParse) {
        // Get next character
        char c = *ctx->currentShortNameParse++;
        if(c) {
            // Go through the options
            for(const struct option* opt = ctx->options; opt->short_opt != 0; ++opt) {
                if(opt->short_opt == c) {
                    // If option has args set the currentOptArgPtr and advance the currentArg
                    if(opt->arg_count > 0 && ctx->currentArg < ctx->argc && ctx->currentArg + opt->arg_count <= ctx->argc) {
                        ctx->currentOptArgPtr = ctx->argv + ctx->currentArg;
                        ctx->currentArg += opt->arg_count;
                    } else {
                        ctx->currentOptArgPtr = 0;
                    }
                    return c;
                }
            }

            return -2;
        }
        
        ctx->currentShortNameParse = 0;
    }

    char* currentArg = ctx->argv[ctx->currentArg++];

    if(currentArg[0] == '-') {
        if(currentArg[1] == '-') {
            // Full name option
            for(const struct option* opt = ctx->options; opt->short_opt != 0; ++opt) {
                if(!strcmp(currentArg + 2, opt->long_opt)) {
                    // If option has args set the currentOptArgPtr and advance the currentArg
                    if(opt->arg_count > 0 && ctx->currentArg < ctx->argc && ctx->currentArg + opt->arg_count <= ctx->argc) {
                        ctx->currentOptArgPtr = ctx->argv + ctx->currentArg;
                        ctx->currentArg += opt->arg_count;
                    } else {
                        ctx->currentOptArgPtr = 0;
                    }

                    return opt->short_opt;
                }
            }

            return -2;
        } else {
            // Short name option
            ctx->currentShortNameParse = currentArg + 1;
            goto shortOptParse;
        }
    }

    ctx->currentOptArgPtr = ctx->argv + ctx->currentArg - 1;
    return 0;
}

char** get_option_arguments(struct argument_parsing_context* ctx) {
    return ctx->currentOptArgPtr;
}

void finish_arg_parse(struct argument_parsing_context* ctx) {
    free(ctx);
}

