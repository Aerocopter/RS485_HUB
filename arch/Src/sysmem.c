#include <errno.h>
#include <sys/stat.h>

extern int errno;
extern char _end; /* 由链接脚本定义 */
static char *heap_end = &_end;

caddr_t _sbrk(int incr) {
    char *prev_heap_end = heap_end;
    heap_end += incr;
    return (caddr_t) prev_heap_end;
}
