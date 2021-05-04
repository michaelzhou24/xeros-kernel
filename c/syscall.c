/* syscall.c : syscalls
 */

#include <xeroskernel.h>
#include <stdarg.h>


int syscall( int req, ... ) {
/**********************************/

    va_list     ap;
    int         rc;

    va_start( ap, req );

    __asm __volatile( " \
        movl %1, %%eax \n\
        movl %2, %%edx \n\
        int  %3 \n\
        movl %%eax, %0 \n\
        "
        : "=g" (rc)
        : "g" (req), "g" (ap), "i" (KERNEL_INT)
        : "%eax" 
    );
 
    va_end( ap );

    return( rc );
}

unsigned int syscreate( funcptr fp, size_t stack ) {
/*********************************************/

    return( syscall( SYS_CREATE, fp, stack ) );
}

void sysyield( void ) {
/***************************/
  syscall( SYS_YIELD );
}

 void sysstop( void ) {
/**************************/

   syscall( SYS_STOP );
}

PID_t sysgetpid( void ) {
/****************************/

    return( syscall( SYS_GETPID ) );
}

void sysputs( char *str ) {
/********************************/

    syscall( SYS_PUTS, str );
}

unsigned int syssleep( unsigned int t ) {
/*****************************/

    return syscall( SYS_SLEEP, t );
}

int sysgetcputimes(processStatuses *ps) {
  return syscall(SYS_CPUTIMES, ps);
}

sighandler_t syssignal(int signum, sighandler_t handler) {
  return syscall(SYS_SIGNAL, signum, handler);
}

void syssigreturn(void *cntxPtr)  {
  return syscall(SYS_SIGRETURN, cntxPtr);
}

int syskill(PID_t PID, int signalNumber) {
  return syscall(SYS_SIGKILL, PID, signalNumber);
}

int syswait(PID_t PID) {
  return syscall(SYS_SIGWAIT, PID);
}

extern int sysopen(int device_no){
   return syscall(SYS_OPEN, device_no);
}

extern int sysclose(int fd){
   return syscall(SYS_CLOSE, fd);
}

extern int syswrite(int fd, void *buff, int bufflen){
   return syscall(SYS_WRITE, fd, buff, bufflen);
}

extern int sysread(int fd, void *buff, int bufflen){
   return syscall(SYS_READ, fd, buff, bufflen);
}

extern int sysioctl(int fd, unsigned long command, ...) {
  va_list ap;
  int res;
  va_start(ap, command);
  res = syscall(SYS_IOCTL, fd, command, ap);
  va_end(ap);
  return res;
}