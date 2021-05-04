/* signal.c - support for signal handling
   This file is not used until Assignment 3
 */

#include <xeroskernel.h>
#include <xeroslib.h>

/* Your code goes here */
void sigtramp(void (*handler)(void *), void *cntxPtr) {
  handler(cntxPtr);
	syssigreturn(cntxPtr);
}

int signal(int pid, int signal){
  pcb *p = findPCB(pid);
	if(!p){
		return -1;
	}
	else if (signal < 0 || signal >= MAX_SIGNALS){
		return -1;
	}else {
  
    if (p->allowed_sig & (1 << signal)) {
      p->pending_sig |= (1 << signal);
    }

    if (p->state == STATE_WAITING) {
      p->ret = -777;
      pcb* waiting_on = findPCB(p->waiting_on_pid);
      dequeue(&(waiting_on->waiters), p->pid);
      ready(p);
    } else if(p->state == STATE_SLEEP) {
      removeFromSleep(p);
      ready(p);
    }

	}	
  return 1000;
}

/* PID - process to wait on to terminate
   caller - process that is going to wait
*/
int sigwait(PID_t PID, pcb *caller) {
  pcb* toWaitOnPcb = findPCB(PID);
  if(!toWaitOnPcb) {
    return -1; // Doesnt exist
  } else if(toWaitOnPcb->state != STATE_READY) {
    ready(toWaitOnPcb);
    return -777;
  }
  // Add process to waiting Q; maybe refactor to enqueue
    // if (toWaitOn)
    // pcb* curr = toWaitOnPcb->waiters;
    // while(curr->next != NULL) {
    //   curr = curr->next;
    // }
    // curr->next = caller;
    enqueue(&(toWaitOnPcb->waiters), caller);
    caller->waiting_on_pid = PID;
    caller->state = STATE_WAITING;
    return 0;
}

/* 
  Deliver signal to process p
*/
void deliverSignal(pcb* p) {
  // Get highst priority signal
  // Check validity, then handle that (signal_helper)
  // Reset pending signals to allow that same signal 
  
  context_frame *new_context;
  signal_frame *sig_frame;
  int sigNo = get_highest_signal(p);
 
  if (sigNo == -1) {
    return; // No sig to process
  }
  p->pending_sig &= ~(1 << sigNo);
  void (*handler)(void*) = p->sig_handlers[sigNo];
  new_context = (context_frame*) (p->esp - sizeof(signal_frame));

  new_context->edi = 0;
	new_context->esi = 0;
	new_context->ebp = 0;
	new_context->esp = 0;
	new_context->ebx = 0;
	new_context->edx = 0;
	new_context->ecx = 0;
	new_context->eax = 0;
  new_context->iret_eip = (unsigned int) sigtramp;
  new_context->iret_cs = (unsigned int) getCS();
  new_context->eflags = 0x00003200;

  sig_frame = (signal_frame*) new_context;
  sig_frame->ret_addr = (unsigned int) 0;
  sig_frame->handler= handler;
  sig_frame->cntx = (unsigned int) p->esp;
  sig_frame->old_sp = (unsigned int) p->esp;
  
  p->esp = new_context;
}

int get_highest_signal(pcb* p) {
  unsigned int pending = p->pending_sig;
  if (pending == 0) {
    return -1;
  }
  int bitpos = -1;
  while (pending != 0) {
    bitpos++;
    pending = pending >> 1;
  }
  return bitpos;
}


int getSignalNumber(int pendingSig) {
  return -1;
}