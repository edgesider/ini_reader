#include <stdlib.h>
#include <errno.h>

#define errExit(str) do \
        {\
            perror(str);\
            exit(EXIT_FAILURE);\
        } while (0)

#define fatal(args...) do \
        { \
            dprintf(2, args); \
            exit(EXIT_FAILURE); \
        } while (0)

#define myassert(expr, str) do \
        { \
            if (!(expr)) { \
                dprintf(2, "(FILE: %s, LINE: %d) assert failed %s: %s", \
                __FILE__, __LINE__, #expr, str); \
                exit(EXIT_FAILURE); \
            } \
        } \
        while (0)
