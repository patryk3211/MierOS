#undef TARGET_MIEROS
#define TARGET_MIEROS 1

#undef LIB_SPEC
#define LIB_SPEC "-lc"

#undef STARTFILE_SPEC
#define STARTFILE_SPEC "crt0.o%s crti.o%s crtbegin.o%s"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend.o%s crtn.o%s"

#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()            \
    do {                                    \
        builtin_define("__mieros__");       \
        builtin_define("__unix__");         \
        builtin_define("system=mieros");    \
        builtin_define("system=unix");      \
        builtin_define("system=posix");     \
    } while(0);
