
#include <di_calls.h>
#include <xeroskernel.h>
#include <stdarg.h>

// Implement the Device Indepedent code in this file.


void devinit(void) {
  devtab[CONSOLE].dvnum = CONSOLE;
  devtab[CONSOLE].dvname = "zero_device";
  devtab[CONSOLE].dvopen = &zero_open;
  devtab[CONSOLE].dvclose = &zero_close;
  devtab[CONSOLE].dvread = &zero_read;
  devtab[CONSOLE].dvwrite = &zero_write;
  devtab[CONSOLE].dvcntl = &zero_ioctl;

  devtab[SERIAL0].dvnum = SERIAL0;
  devtab[SERIAL0].dvname = "rand_device";
  devtab[SERIAL0].dvopen = &rand_open;
  devtab[SERIAL0].dvclose = &rand_close;
  devtab[SERIAL0].dvread = &rand_read;
  devtab[SERIAL0].dvwrite = &rand_write;
  devtab[SERIAL0].dvcntl = &rand_ioctl;

  devtab[KBMON].dvnum = KBMON;
  devtab[KBMON].dvname = "keyboard_device";
  devtab[KBMON].dvopen = &kbd_open;
  devtab[KBMON].dvclose = &kbd_close;
  devtab[KBMON].dvread = &kbd_read;
  devtab[KBMON].dvwrite = &kbd_write;
  devtab[KBMON].dvcntl = &kbd_ioctl;
}
/*
- allocate fdt entry, lookup device name in fs,
- get major and minor numbers from fs, locate device block
returns: index of selected fdt to process
*/
int di_open(pcb* p, int device_no) {
    if (device_no < 0 || device_no >= MAX_DEVICES) {
        return DEVICE_ERROR;
    }
    int fd = -1;
    for (int i = 0; i < MAX_FD; i++) {
        if (p->fd_table[i].status == 0) {
            fd = i;
        }
    }
    if (fd == -1) {
        return DEVICE_ERROR;
    }
	p->fd_table[fd].major = devtab[device_no].dvnum;
	p->fd_table[fd].dvname = devtab[device_no].dvname; 	
	p->fd_table[fd].dev_ptr = &devtab[device_no]; 
	p->fd_table[fd].data = NULL;
	p->fd_table[fd].status = 1;
    // kprintf("calling device open\n");
	return (p->fd_table[fd].dev_ptr->dvopen)(p, fd);
}

int di_close(pcb* p, int fd) {
    if(fd > MAX_FD || fd < 0) {
        return DEVICE_ERROR;
    }
    if(p->fd_table[fd].status == 0 ) {
        return DEVICE_ERROR;
    }
    p->fd_table[fd].status = 0;
    // kprintf("calling device close\n");
    return (p->fd_table[fd].dev_ptr->dvclose)(p, fd);
}

/* 
- verify fd is valid and opened
- determine index of device in device table'
- call device function
*/
int di_write(pcb* p, int fd, void* buff, int len) {
    if(fd > MAX_FD || fd < 0) {
        return DEVICE_ERROR;
    }
    if(p->fd_table[fd].status == 0 ) {
        return DEVICE_ERROR;
    }
    int res =  (p->fd_table[fd].dev_ptr->dvwrite)(p, buff, len);
    if(res ==DEVICE_BLOCK ) {
        enqueue(&device_block, p);
        p->state = STATE_WRITING;
        return DEVICE_BLOCK;
    } else {
        // finished or error, return number of bytes written
        ready(p);
        return res;
    }
}

int di_read(pcb* p, int fd, void* buff, int len) {
    if(fd > MAX_FD || fd < 0) {
        return DEVICE_ERROR;
    }
    if(p->fd_table[fd].status == 0 ) {
        return DEVICE_ERROR;
    }
    int res =  (p->fd_table[fd].dev_ptr->dvread)(p, buff, len);
    if(res == DEVICE_BLOCK ) {
        // kprintf("Process: %d put on kb blocked queue\n", p->pid);
        enqueue(&device_block, p);
        p->kbd_buf = buff;
        p->kbd_buf_len = len;
        p->kbd_fd = fd;
        p->state = STATE_READING;
        return res;
    } else {
        // finished or error, return number of bytes written
        ready(p);
        return res;
    }
}

int di_ioctl(pcb* p, int fd, unsigned long command, va_list ap) {
    if(fd > MAX_FD || fd < 0) {
        return DEVICE_ERROR;
    }
    if(p->fd_table[fd].status == 0 ) {
        return DEVICE_ERROR;
    }
    int res =  (p->fd_table[fd].dev_ptr->dvcntl)(p, command, ap);
    // finished or error, return number of bytes written
    return res;
}
