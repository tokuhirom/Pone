#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

void pone_init() {
#if defined(_WIN32) || defined(_WIN64)
    setmode(fileno(stdout), O_BINARY);
#endif
}
