/* xeroskernel.h - disable, enable, halt, restore, isodd, min, max */

#ifndef XEROSKERNEL_H
#define XEROSKERNEL_H

/* Symbolic constants used throughout gutted Xinu */

typedef	char    Bool;        /* Boolean type                  */
typedef unsigned int size_t; /* Something that can hold the value of
                              * theoretical maximum number of bytes 
                              * addressable in this architecture.
                              */

typedef unsigned int PID_t;  // What a process ID is defined to be

#define	FALSE   0       /* Boolean constants             */
#define	TRUE    1
#define	EMPTY   (-1)    /* an illegal gpq                */
#define	NULL    0       /* Null pointer for linked lists */
#define	NULLCH '\0'     /* The null character            */

#define CREATE_FAILURE -1  /* Process creation failed     */


/* Universal return constants */

#define	OK            1         /* system call ok               */
#define	SYSERR       -1         /* system call failed           */
#define	EOF          -2         /* End-of-file (usu. from read)	*/
#define	TIMEOUT      -3         /* time out  (usu. recvtim)     */
#define	INTRMSG      -4         /* keyboard "intr" key pressed	*/
                                /*  (usu. defined as ^B)        */
#define	BLOCKERR     -5         /* non-blocking op would block  */
#define LASTCONST    -5

/* Functions defined by startup code */


void           bzero(void *base, int cnt);
void           bcopy(const void *src, void *dest, unsigned int n);
void           disable(void);
unsigned short getCS(void);
unsigned char  inb(unsigned int);
void           init8259(void);
int            kprintf(char * fmt, ...);
void           lidt(void);
void           outb(unsigned int, unsigned char);


/* Some constants involved with process creation and managment */
 
   /* Maximum number of processes */      
#define MAX_PROC        64           
   /* Kernel trap number          */
#define KERNEL_INT      84
   /* Interrupt number for the timer */
#define TIMER_INT      (TIMER_IRQ + 32)
   /* Minimum size of a stack when a process is created */
#define PROC_STACK      (4096 * 4)    

#define KB_INT          (TIMER_IRQ + 33)            
   /* Number of milliseconds in a tick */
#define MILLISECONDS_TICK 10        

#define MAX_SIGNALS  32

#define MAX_DEVICES  5

#define MAX_FD       4

/* Constants to track states that a process is in */
#define STATE_STOPPED   0
#define STATE_READY     1
#define STATE_SLEEP     22
#define STATE_RUNNING   23
#define STATE_WAITING   24
#define STATE_READING   25
#define STATE_WRITING   26

/* System call identifiers */
#define SYS_STOP        20
#define SYS_YIELD       21
#define SYS_CREATE      22

#define SYS_TIMER       233
#define SYS_GETPID      244
#define SYS_PUTS        255
#define SYS_SLEEP       266
#define SYS_KILL        277
#define SYS_CPUTIMES    278
#define SYS_P           300
#define SYS_V           301
#define SYS_SIGNAL      302
#define SYS_SIGRETURN   303        
#define SYS_SIGKILL     304  
#define SYS_SIGWAIT     305
#define SYS_OPEN        306
#define SYS_CLOSE       307
#define SYS_WRITE       308
#define SYS_READ        309
#define SYS_IOCTL       310

#define CONSOLE   0
#define SERIAL0   1
#define SERIAL1   2
#define KBMON     3
#define TTY0      4

#define CMD_EOF         18
#define CMD_ECHO_OFF    73
#define CMD_ECHO_ON     74

#define DEVICE_BLOCK    1
#define DEVICE_DONE     0
#define DEVICE_ERROR    -1

// Devices
typedef struct struct_devsw devsw;
struct struct_devsw {
   int dvnum;
   char *dvname;
   int (*dvinit)();
   int (*dvopen)();
   int (*dvclose)();
   int (*dvread)();
   int (*dvwrite)();
   int (*dvseek)();
   int (*dvgetc)();
   int (*dvputc)();
   int (*dvcntl)();
   void *dvcsr;
   void *dvivec;
   void *dvovec;
   int (*dviint)();
   int (*dvoint)();
   void *dvioblk;
};


devsw devtab[MAX_DEVICES];

extern int ttyinit(void);
extern int ttyopen(void);
extern int ionull(void);
extern int ttyread(void);
extern int ttywrite(void);

// File descriptor
typedef struct struct_fd_t fd_t;
struct struct_fd_t {
   int major; 
   char* dvname;
	devsw* dev_ptr;
	void *data;
	int status;
   
};

/* Structure to track the information associated with a single process */
typedef void (*sighandler_t)(void *);

extern struct memHeader {
    unsigned long size;
    struct memHeader *prev;
    struct memHeader *next;
    char* sanityCheck;
    unsigned char dataStart[0];
};

typedef struct struct_pcb pcb;
struct struct_pcb {
   void        *esp;    /* Pointer to top of saved stack           */
   pcb         *next;   /* Next process in the list, if applicable */
   pcb         *prev;   /* Previous proccess in list, if applicable*/
   int          state;  /* State the process is in, see above      */
   PID_t        pid;    /* The process's ID                        */
   int          ret;    /* Return value of system call             */
                        /* if process interrupted because of system*/
                        /* call                                    */
   long         args;   
   unsigned int otherpid;
   void        *buffer;
   int          bufferlen;
   int          sleepdiff;
   long         cpuTime;  /* CPU time consumed                     */
   /* signal handling */
   void (*sig_handlers[MAX_SIGNALS])(void*);
   unsigned int pending_sig;
   unsigned int allowed_sig;
   int waiting_on_pid;
   pcb         *waiters; // processes waiting for this process to terminate
   /* fd, device */
   fd_t fd_table[MAX_FD];
   int kbd_read_so_far;
   void* kbd_buf;
   int kbd_buf_len;
   int kbd_fd;
};

// processes blocked by devices
pcb* device_block;

typedef struct struct_ps processStatuses;
struct struct_ps {
  int     entries;            // Last entry used in the table
  PID_t   pid[MAX_PROC];      // The process ID
  int     status[MAX_PROC];   // The process status
  long    cpuTime[MAX_PROC]; // CPU time used in milliseconds
};


/* The actual space is set aside in create.c */
extern pcb     proctab[MAX_PROC];


#pragma pack(1)

/* What the set of pushed registers looks like on the stack */
typedef struct context_frame {
  unsigned long        edi;
  unsigned long        esi;
  unsigned long        ebp;
  unsigned long        esp;
  unsigned long        ebx;
  unsigned long        edx;
  unsigned long        ecx;
  unsigned long        eax;
  unsigned long        iret_eip;
  unsigned long        iret_cs;
  unsigned long        eflags;
  unsigned long        stackSlots[];
} context_frame;

/* Signal frame */
typedef struct signal_context_frame {
  context_frame new_cntx;
  unsigned int ret_addr;
  unsigned int handler;
  unsigned int cntx;
  unsigned int old_sp;
} signal_frame;
/* Memory mangement system functions, it is OK for user level   */
/* processes to call these.                                     */

int      kfree(void *ptr);
void     kmeminit( void );
void     *kmalloc( size_t );


/* A typedef for the signature of the function passed to syscreate */
typedef void    (*funcptr)(void);


/* Internal functions for the kernel, applications must never  */
/* call these.  You can modify the interfaces if needed.       */
void     dispatch( void );
void     dispatchinit( void );
void     ready( pcb *p );
pcb      *next( void );
void     contextinit( void );
int      contextswitch( pcb *p );
int      create( funcptr fp, size_t stack );
void     set_evec(unsigned int xnum, unsigned long handler);
void     printCF (void * stack);  /* print the call frame */
int      syscall(int call, ...);  /* Used in the system call stub */
void     sleep(pcb *, unsigned int);
void     tick( void );
void removeFromSleep(pcb * p);

void devinit(void) ;

/* Function prototypes for system calls as called by the application */

unsigned int syscreate( funcptr fp, size_t stack );
void         sysyield( void );
void         sysstop( void );
PID_t        sysgetpid( void );
PID_t        syssleep(unsigned int);
void         sysputs(char *str);
int          sysgetcputimes(processStatuses *ps);
int          sysV(int);
int          sysP(int);      

/* The initial process that the system creates and schedules */
void     root( void );

void           set_evec(unsigned int xnum, unsigned long handler);

sighandler_t syssignal(int signum, sighandler_t handler);
void syssigreturn(void *cntxPtr);
int syskill(PID_t PID, int signalNumber);
int syswait(PID_t PID);

extern int sysopen(int device_no);
extern int sysclose(int fd);
extern int syswrite(int fd, void *buff, int bufflen);
extern int sysread(int fd, void *buff, int bufflen);
extern int sysioctl(int fd, unsigned long command, ...);

void sigtramp(void (*handler)(void *), void *cntxPtr);
int signal(int pid, int signal);
int sigwait(PID_t PID, pcb *caller);

extern pcb *findPCB( int pid );
pcb* dequeue(pcb** queue, PID_t pid);
void enqueue(pcb** queue, pcb* proc);
void set_signal_handler(pcb* p, int signal_no, sighandler_t handler);
/* Anything you add must be between the #define and this comment */
#endif

