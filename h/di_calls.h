#include <stdarg.h>
#include <xeroskernel.h>
#include <zerorand.h>
#include <kbd.h>
int di_open(pcb* p, int device_no);

int di_close(pcb* p, int fd);

int di_write(pcb* p, int fd, void* buff, int len);

int di_read(pcb* p, int fd, void* buff, int len);

int di_ioctl(pcb* p, int fd, unsigned long command, va_list ap);