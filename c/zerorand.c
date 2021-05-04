
#include <zerorand.h>
#include <xeroslib.h>
#include <stdarg.h>
// Implement the upper and lower half of the device driver code for the zero device here
// The zero device is an infinite stream of 0s. Any read of the device for any number of bytes
// alwasy returns the requested number of bytes, but each byte is 0 (i.e. the bits are all 0s)

int zero_open(pcb* p, int fd){
    return fd;
}
int zero_ioctl(pcb * p, void* command, va_list ap){
    return -1;
}
int zero_close(pcb* p, int fd){
    return fd;
}
int zero_read(pcb* p, void* buff, int len){
    memset(buff, 0,len);
    return len;
}
int zero_write(pcb* p, void* buff, int len){
    return len;
}


// Implement the upper and lower half of the device driver code for the random device here 
// The random device is an infinite stream of random bytes. Any read of the device for any number of bytes 
// alwasy returns the requested number of bytes, but each byte is a random value. Use the rand() and srand()
// functions from the xeroslib. The srand() function can be used to set a seed value for the rand() function

int rand_open(pcb* p, int fd){
    return fd;
}
int rand_ioctl(pcb * p, void* command, va_list ap){
    if((int) command == 23){
        unsigned int seed = va_arg(ap, unsigned int);
        srand(seed);
        return 0;
    }
    return -1;
}
int rand_close(pcb* p, int fd){
    return fd;
}
int rand_read(pcb* p, void* buff, int len){

   int remainder = len%4;
   int lowestClosest = 4 - remainder;
   int ret = 0;
   for (int i = 0; i < len/4; i++) {
       ret += rand() << (i*32);
   }
   ret += (rand() >> lowestClosest) << ((len/4*32) + (remainder*32));
   memset(buff, ret, len);
   return ret;
}

int rand_write(pcb* p, void* buff, int len){
    return -1;
}