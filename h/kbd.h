#include <xeroskernel.h>
#include <stdarg.h>

#define KERNEL_BUF_SIZE 4
#define EOT 0x04
#define ENTER 0x0A
unsigned int kernel_buf[KERNEL_BUF_SIZE];

// from scancodesToAscii
static  int     state; /* the state of the keyboard */
#define KEY_UP   0x80            /* If this bit is on then it is a key   */
                                 /* up event instead of a key down event */

/* Control code */
#define LSHIFT  0x2a
#define RSHIFT  0x36
#define LMETA   0x38

#define LCTL    0x1d
#define CAPSL   0x3a

/* scan state flags */
#define INCTL           0x01    /* control key is down          */
#define INSHIFT         0x02    /* shift key is down            */
#define CAPSLOCK        0x04    /* caps lock mode               */
#define INMETA          0x08    /* meta (alt) key is down       */
#define EXTENDED        0x10    /* in extended character mode   */

#define EXTESC          0xe0    /* extended character escape    */
#define NOCHAR  256

int kbd_open(pcb* p, int fd);
int kbd_close(pcb* p, int fd);
int kbd_read(pcb* p, void* buff, int len);
int kbd_write(pcb* p, void* buff, int len);
int kbd_ioctl(pcb * p, void* command, va_list ap);

