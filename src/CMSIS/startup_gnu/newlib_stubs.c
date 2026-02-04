// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Kris Kwiatkowski <kris@amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

/* Variables */
//Uncomment following if needed - commented out for now as not used and causes warning
//extern int errno;
register char *stack_ptr asm("sp");

/* Functions */

/**
 _sbrk
 Increase program data space. Malloc and related functions depend on this
**/
char *_sbrk(int incr) {
    extern char  end asm("end");
    static char *heap_end;
    char        *prev_heap_end;

    if (heap_end == 0)
        heap_end = &end;

    prev_heap_end = heap_end;
    if (heap_end + incr > stack_ptr) {
        errno = ENOMEM;
        return (char *)-1;
    }

    heap_end += incr;

    return (char *)prev_heap_end;
}

int _write(int fd, const void *buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

int _read(int fd, void *buf, size_t count) {
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

int _close(int fd) {
    (void)fd;
    errno = ENOSYS;
    return -1;
}
int _lseek(int fd, int ptr, int dir) {
    (void)fd;
    (void)ptr;
    (void)dir;
    errno = ENOSYS;
    return -1;
}

int _fstat(int fd, struct stat *st) {
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}
int _isatty(int fd) {
    (void)fd;
    return 1;
}