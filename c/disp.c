/* disp.c : dispatcher
 */

#include <xeroskernel.h>
#include <i386.h>
#include <xeroslib.h>
#include <stdarg.h>
#include <di_calls.h>

int getCPUtimes(pcb *p, processStatuses *ps);
void close_fds(pcb *p);
void print_queue(pcb* p) {
  while (p) {
    kprintf("pid->%d ; ",p->pid);
    p = p->next;
  }
  kprintf("\n");
}
static int  kill(pcb *currP, int pid);
void unblock_waiters(pcb* p);
extern long freemem;
static pcb      *head = NULL;
static pcb      *tail = NULL;

void     dispatch( void ) {
/********************************/

    pcb         *p;
    int         r;
    funcptr     fp;
    int         stack;
    va_list     ap;
    char        *str;
    int         len;
    signal_frame *sig_frame;
    for( p = next(); p; ) {
      //      kprintf("Process %x selected stck %x\n", p, p->esp);
      deliverSignal(p);
      r = contextswitch( p ); 
      switch( r ) {
      case( SYS_CREATE ):
        ap = (va_list)p->args;
        fp = (funcptr)(va_arg( ap, int ) );
        stack = va_arg( ap, int );
	      p->ret = create( fp, stack );
        break;
      case( SYS_YIELD ):
        ready( p );
        p = next();
        break;
      case( SYS_STOP ):
        p->state = STATE_STOPPED;
        unblock_waiters(p);
        close_fds(p);
        p = next();
        break;
      case ( SYS_KILL ):
        ap = (va_list)p->args;
	      p->ret = kill(p, va_arg( ap, int ) );
	      break;
      case (SYS_CPUTIMES):
	      ap = (va_list) p->args;
	      p->ret = getCPUtimes(p, va_arg(ap, processStatuses *));
	      break;
      case( SYS_PUTS ):
        ap = (va_list)p->args;
        str = va_arg( ap, char * );
        kprintf( "%s", str );
        p->ret = 0;
        break;
      case( SYS_GETPID ):
        p->ret = p->pid;
        break;
      case( SYS_SLEEP ):
        ap = (va_list)p->args;
        len = va_arg( ap, int );
        sleep( p, len );
        p = next();
        break;
      case( SYS_TIMER ):
        tick();
        //kprintf("T");
        p->cpuTime++;
        ready( p );
        p = next();
        end_of_intr();
	      break;
      case (SYS_SIGNAL): {
        // kprintf("DISPATCH: SYS_SIGNAL! \n");
        ap = (va_list)p->args;
        int signal_no = va_arg(ap, int);
        sighandler_t handler = (sighandler_t) va_arg(ap, int);
        set_signal_handler(p, signal_no, handler);
        break;
      }
      case (SYS_SIGRETURN):
        ap = (va_list)p->args;
        p->esp = va_arg(ap, int);
        ready(p);
        p = next();
        break;
      case (SYS_SIGKILL): {
        //  kprintf("DISPATCH: SYS_SIGKILL! \n");
        ap = (va_list)p->args;
        int pid = va_arg(ap, int);
        int signal_no = va_arg(ap, int);
        signal(pid, signal_no);
      //  sigkill(p, pid , signal);
        break;
      }
      case (SYS_SIGWAIT): {
        ap = (va_list)p->args;
        int pid = va_arg(ap, int);
        p->ret = sigwait(pid, p);
        p = next();
        break;
      }
      case (SYS_OPEN): {
        ap = (va_list)p->args;
        int device_no = va_arg(ap, int);
        p->ret = di_open(p, device_no);
        ready(p);
        p = next();
        break;
      }
      case (SYS_CLOSE): {
        ap = (va_list)p->args;
        int fd = va_arg(ap, int);
        p->ret = di_close(p, fd);
        ready(p);
        p = next();
        break;
      }
      case (SYS_READ): {
        ap = (va_list)p->args;
        int fd = va_arg(ap, int);
        int buff = (void *)va_arg(ap, int);
        int len = va_arg(ap,int);
        p->ret = di_read(p, fd, buff, len);
        // kprintf("printing the ready queue\n");
        // print_queue(head);
        // kprintf("printing the blocked queue\n");
        // print_queue(device_block);
        p = next();
        break;
      }
      case (SYS_WRITE): {
        ap = (va_list)p->args;
        int fd = va_arg(ap, int);
        int buff = (void *)va_arg(ap, int);
        int len = va_arg(ap,int);
        int ret =  di_write(p, fd, buff, len);
        p->ret = ret;
        if(ret == DEVICE_ERROR) {
          ready(p); // Doesn't get blocked and queued; 
        }
        // ready happens in write or when process is done writing
        p = next();
        break;
      }
      case (SYS_IOCTL): {
        ap = (va_list)p->args;
        int fd = va_arg(ap, int);
        unsigned long cmd = (unsigned long) va_arg(ap, long);
        va_list ap_arg = va_arg(ap, va_list);
        p->ret = di_ioctl(p, fd, cmd, ap_arg);
        ready(p);
        p = next();
        break;
      }
      case (KB_INT): {
        pcb* finished_p = kbd_int_handler();
        if (finished_p) {
          // unblock.
          // kprintf("Dequing process...");
          dequeue(&device_block, finished_p->pid);
          ready(finished_p);
          if (device_block) {
            // read again
            dequeue(&device_block, device_block->pid);
            di_read(device_block, device_block->kbd_fd, device_block->kbd_buf, device_block->kbd_buf_len);
          }
        } 
        end_of_intr();
        break;
      }
      default:
        kprintf( "Bad Sys request %d, pid = %d\n", r, p->pid );
      }
    }

    kprintf( "Out of processes: dying\n" );
    
    for( ;; );
}

void close_fds(pcb* p) {
  for (int i = 0; i < MAX_FD; i++) {
    di_close(p, i);
  }
}

void set_signal_handler(pcb* p, int signal_no, sighandler_t handler) {
  if (signal_no < 0 || signal_no >= (MAX_SIGNALS-1)) {
    p->ret = 0;
  } else if ((unsigned int) handler >= HOLESTART && (unsigned int) handler <= HOLEEND) {
    p->ret = 0;
  } else {
    p->sig_handlers[signal_no] = handler;
    p->ret = 1;
    if (handler == NULL) {
      p->allowed_sig &= ~(1 << signal_no);
    } else {
      p->allowed_sig |= 1 << signal_no;
    }
  }
}

void unblock_waiters(pcb* p) {
  while (p->waiters) {
    kprintf("unblocking process pid = %d\n", p->waiters->pid);
    ready(p->waiters);
    p->waiters->state = STATE_READY;
    p->waiters = p->waiters->next;
  }
}

extern void dispatchinit( void ) {
/********************************/

  //bzero( proctab, sizeof( pcb ) * MAX_PROC );
  memset(proctab, 0, sizeof( pcb ) * MAX_PROC);
}



extern void     ready( pcb *p ) {
/*******************************/

    p->next = NULL;
    p->state = STATE_READY;

    if( tail ) {
        tail->next = p;
    } else {
        head = p;
    }

    tail = p;
}

extern pcb      *next( void ) {
/*****************************/

    pcb *p;

    p = head;

    if( p ) {
        head = p->next;
        if( !head ) {
            tail = NULL;
        }
    } else { // Nothing on the ready Q and there should at least be the idle proc.
        kprintf( "BAD\n" );
        for(;;);
    }

    p->next = NULL;
    p->prev = NULL;
    return( p );
}


extern pcb *findPCB( int pid ) {
/******************************/

    int	i;

    for( i = 0; i < MAX_PROC; i++ ) {
        if( proctab[i].pid == pid ) {
            return( &proctab[i] );
        }
    }

    return( NULL );
}


// This function takes a pointer to the pcbtab entry of the currently active process. 
// The functions purpose is to remove the process being pointed to from the ready Q
// A similar function exists for the management of the sleep Q. Things should be re-factored to 
// eliminate the duplication of code if possible. There are some challenges to that because
// the sleepQ is a delta list and something more than just removing an element in a list
// is being preformed. 


void removeFromReady(pcb * p) {

  if (!head) {
    kprintf("Ready queue corrupt, empty when it shouldn't be\n");
    return;
  }

  if (head == p) { // At front of list
    // kprintf("Pid %d is at front of list\n", p->pid);
    head = p->next;

    // If the implementation has idle on the ready list this next statement
    // isn't needed. However, it is left just in case someone decides to 
    // change things so that the idle process is kept separate. 

    if (head == NULL) { // If the implementation has idle process  on the 
      tail = head;      // ready list this should never happen
      kprintf("Kernel bug: Where is the idle process\n");
    }
  } else {  // Not at front, find the process.
    pcb * prev = head;
    pcb * curr;
    
    for (curr = head->next; curr!=NULL; curr = curr->next) {
      if (curr == p) { // Found process so remove it
	// kprintf("Found %d in list, removing\n", curr->pid);
	prev->next = p->next;
	if (tail == p) { //last element in list
	    tail = prev;
	    // kprintf("Last element\n");
	}
	p->next = NULL; // just to clean things up
	break;
      }
      prev = curr;
    }
    if (curr == NULL) {
      kprintf("Kernel bug: Ready queue corrupt, process %d claimed on queue and not found\n",
	      p->pid);
      
    }
  }
}

// This function takes 2 paramenters:
//  currP  - a pointer into the pcbtab that identifies the currently running process
//  pid    - the proces ID of the process to be killed.
//
// Note: this function needs to be augmented so that it delivers a kill signal to a 
//       a particular process. The main functionality of the this routine will remain the 
//       same except that when the process is located it needs to be put onto the readyq
//       and a signal needs to be marked for delivery. 
//

static int  kill(pcb *currP, int pid) {
  pcb * targetPCB;
  
  kprintf("Current pid %d Killing %d\n", currP->pid, pid);
  
  if (pid == currP->pid) {   // Trying to kill self
    return -2;
  }

  // Don't let it kill the idle process, which from the user side
  // of things isn't a real process
  // IDLE process had PID 0

  if (pid == 0) {
    return -1;
  }
    
  if (!(targetPCB = findPCB( pid ))) {
    // kprintf("Target pid not found\n");
    return -1;
  }

  if (targetPCB->state == STATE_STOPPED) {
    kprintf("Target pid was stopped\n");
    return  -1;
  }
  
  // PCB has been found,  and the proces is either sleepign or running.
  // based on that information remove the process from 
  // the appropriate queue/list.

  if (targetPCB->state == STATE_SLEEP) {
    // kprintf("Target pid %d sleeping\n", targetPCB->pid);
    removeFromSleep(targetPCB);
  }

  if (targetPCB->state == STATE_READY) {
    // remove from ready queue
    // kprintf("Target pid %d is ready\n", targetPCB->pid);
    removeFromReady(targetPCB);
  }

  // Check other states and do state specific cleanup before stopping
  // the process 
  // In the new version the process will not be marked as stopped but be 
  // put onto the readyq and a signal marked for delivery. 

  targetPCB->state = STATE_STOPPED;
  return 0;
}
  

// This function is the system side of the sysgetcputimes call.
// It places into a the structure being pointed to information about
// each currently active process. 
//  p - a pointer into the pcbtab of the currently active process
//  ps  - a pointer to a processStatuses structure that is 
//        filled with information about all the processes currently in the system
//

extern char * maxaddr;
  
int getCPUtimes(pcb *p, processStatuses *ps) {
  
  int i, currentSlot;
  currentSlot = -1;

  // Check if address is in the hole
  if (((unsigned long) ps) >= HOLESTART && ((unsigned long) ps <= HOLEEND)) 
    return -1;

  //Check if address of the data structure is beyone the end of main memory
  if ((((char * ) ps) + sizeof(processStatuses)) > maxaddr)  
    return -2;

  // There are probably other address checks that can be done, but this is OK for now


  for (i=0; i < MAX_PROC; i++) {
    if (proctab[i].state != STATE_STOPPED) {
      // fill in the table entry
      currentSlot++;
      ps->pid[currentSlot] = proctab[i].pid;
      ps->status[currentSlot] = p->pid == proctab[i].pid ? STATE_RUNNING: proctab[i].state;
      ps->cpuTime[currentSlot] = proctab[i].cpuTime * MILLISECONDS_TICK;
    }
  }

  return currentSlot;
}

void enqueue(pcb** queue, pcb* proc) {
  if (*queue == NULL) {
    *queue = proc;
    proc->prev = NULL;
  } else {
    pcb* tmp = *queue;
    while (tmp->next != NULL) {
      tmp = tmp->next;
    }
    tmp->next = proc;
    proc->prev = tmp;
  }
  proc->next = NULL;
}

pcb* dequeue(pcb** queue, PID_t pid) {
  if(*queue == NULL) {
    return NULL;
  }
  pcb* head = *queue;
  *queue = (*queue)->next;
  (*queue)->prev = NULL;
  return head;
}