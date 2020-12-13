#include <stdio.h>
#include <stdlib.h>

#define STRINGIFY_impl(x) #x
#define STRINGIFY(x) STRINGIFY_impl(x)

#define ASSERT(expr)                                                           \
    ((!!(expr))                                                                \
         ? (void)0                                                             \
         : (fprintf(stderr,                                                    \
                    "\u001b[31;1m%S:" STRINGIFY(__LINE__) ": " #expr           \
                                                          "\u001b[0m\n",       \
                    [](auto x) {                                               \
                        return x ? x + 1 : __FILEW__;                          \
                    }(wcsrchr(__FILEW__, '\\'))),                              \
            exit(1)))