#include <stdarg.h>
#include <xeroskernel.h>
int zero_open(pcb* p, int fd);
int zero_close(pcb* p, int fd);
int zero_read(pcb* p, void* buff, int len);
int zero_write(pcb* p, void* buff, int len);
int zero_ioctl(pcb * p, void* command, va_list ap);

int rand_open(pcb* p, int fd);
int rand_close(pcb* p, int fd);
int rand_read(pcb* p, void* buff, int len);
int rand_write(pcb* p, void* buff, int len);
int rand_ioctl(pcb * p, void* command, va_list ap);