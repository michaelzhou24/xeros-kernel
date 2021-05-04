
#include <kbd.h>

// From scancodesToAscii
/*  Normal table to translate scan code  */
unsigned char   kbcode[] = { 0,
          27,  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',
         '0',  '-',  '=', '\b', '\t',  'q',  'w',  'e',  'r',  't',
         'y',  'u',  'i',  'o',  'p',  '[',  ']', '\n',    0,  'a',
         's',  'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';', '\'',
         '`',    0, '\\',  'z',  'x',  'c',  'v',  'b',  'n',  'm',
         ',',  '.',  '/',    0,    0,    0,  ' ' };

/* captialized ascii code table to tranlate scan code */
unsigned char   kbshift[] = { 0,
           0,  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',
         ')',  '_',  '+', '\b', '\t',  'Q',  'W',  'E',  'R',  'T',
         'Y',  'U',  'I',  'O',  'P',  '{',  '}', '\n',    0,  'A',
         'S',  'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  '"',
         '~',    0,  '|',  'Z',  'X',  'C',  'V',  'B',  'N',  'M',
         '<',  '>',  '?',    0,    0,    0,  ' ' };
/* extended ascii code table to translate scan code */
unsigned char   kbctl[] = { 0,
           0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
           0,   31,    0, '\b', '\t',   17,   23,    5,   18,   20,
          25,   21,    9,   15,   16,   27,   29, '\n',    0,    1,
          19,    4,    6,    7,    8,   10,   11,   12,    0,    0,
           0,    0,   28,   26,   24,    3,   22,    2,   14,   13 };

static int extchar(unsigned char code);
unsigned int kbtoa( unsigned char code );


// Implement the upper and lower half of the keyboard device driver in this file.
static unsigned char KB_VAL; 
int kernel_buf_head = -2;
int kernel_buf_tail = -1;
unsigned int ascii;

int echo_flag;
int EOF_flag;

pcb* process;
char* process_buf = NULL;
int process_buf_len = 0; 

unsigned char EOF_MASK = 0x04; 

int kbd_open(pcb* p, int fd) {
    enable_irq(1, 0);
    return fd;
}

int kbd_close(pcb* p, int fd) {
    enable_irq(1,1);
    ascii = 0;
    KB_VAL = 0;
    echo_flag = 0;
    EOF_flag = 0;
    kernel_buf_head = -2;
    kernel_buf_tail = -1;
    process = 0;
    process_buf = 0;
    process_buf_len = 0;
}

int kbd_read(pcb* p, void* buff, int len) {
    process = p;
    if (!process_buf) {
        process_buf = buff;
        process_buf_len = len;
        memset(process_buf, 0, len);
    }
    if (EOF_flag) {
        return 0;
    }

	for (int i = 0; i < len - p->kbd_read_so_far; i++){
		if (kernel_buf_head != -2 && kernel_buf_tail != -1) {
			process_buf[p->kbd_read_so_far] = kernel_buf[kernel_buf_head];
			p->kbd_read_so_far++;
			kernel_buf_head++;
			kernel_buf_head = kernel_buf_head%KERNEL_BUF_SIZE;
		}
		if (kernel_buf_head == kernel_buf_tail) {
			kernel_buf_head = -2;
			kernel_buf_tail = -1;
		}
	}

    if (p->kbd_read_so_far == len) {
        p->kbd_read_so_far = 0;
        return len;
    } else if (p->kbd_read_so_far < len) {
        return DEVICE_BLOCK;
    } else {
        return -1;
    }
}

int kbd_write(pcb* p, void* buff, int len) {
    return -1;
}

int kbd_ioctl(pcb * p, void* command, va_list ap) {
    int cmd  = (int) command;
    unsigned long *r;
    switch (cmd) {
        case CMD_EOF:
            r = (unsigned long*) va_arg(ap, int);
            EOF_MASK = (char *) r;
            break;
        case CMD_ECHO_OFF:
          echo_flag = 0;
          break;
        case CMD_ECHO_ON:
          echo_flag = 1;
          break;
        default:
          return -1;
          break;
    }
    return 0;
}

unsigned char read_char(void){
	__asm __volatile( " \
		pusha  \n\
		in  $0x60, %%al  \n\
		movb %%al, KB_VAL  \n\
		popa  \n\
			"
		:
		: 
		: "%eax"
		);	
	ascii = kbtoa(KB_VAL);	
	if(ascii == EOT || ascii == EOF_MASK){
		EOF_flag = 1;
	}
	return ascii;
}


int kbd_int_handler(){
	unsigned char in_value = read_char();
	if(!EOF_flag){
		if(in_value && in_value != NOCHAR){
			if(echo_flag){
				kprintf("%c",in_value);
			}

			if(!process_buf){		
       // no porcesses waiting
				if(kernel_buf_head == -2 && kernel_buf_tail == -1){
          kernel_buf_head = 0;
          kernel_buf_tail = 0;
          kernel_buf[kernel_buf_tail]=in_value;
          kernel_buf_tail++;
        }
        else if(kernel_buf_head != kernel_buf_tail){
          kernel_buf[kernel_buf_tail] = in_value;
          kernel_buf_tail++;
          kernel_buf_tail = (kernel_buf_tail) % KERNEL_BUF_SIZE;
        }
        return NULL;
						
			} else {
        // process waiting to read
        process_buf[process->kbd_read_so_far] = (char)in_value;
				process->kbd_read_so_far++;
				// read complete
				if (process->kbd_read_so_far == process_buf_len){
          // kprintf("Read complete for  process %d\n", process->pid);	
					process->kbd_read_so_far = 0;
					process_buf = NULL;
          process->ret = process_buf_len-1;
          process_buf_len = 0;
          return process;
				} 
        // read interrupted by enter
        else if (in_value == ENTER){
          // kprintf("Enter detected!\n");
          process_buf[process->kbd_read_so_far] = (char)"\n";
      		process->kbd_read_so_far++; 
					process->ret = process->kbd_read_so_far-1;
					process->kbd_read_so_far = 0;
					process_buf = NULL;
          process_buf_len = 0;
          return process;
				}		
			}
		}
	} else {
      // EOF key, return 0
      process->ret = 0;
      process_buf = NULL;
      return process;	
  }
  return NULL;
}

// from scancodesToAscii
// only changed printf to kprintf
static int extchar(unsigned char   code)
{
    state &= ~EXTENDED;
}

unsigned int kbtoa( unsigned char code )
{
  unsigned int    ch;
  
  if (state & EXTENDED)
    return extchar(code);
  if (code & KEY_UP) {
    switch (code & 0x7f) {
    case LSHIFT:
    case RSHIFT:
      state &= ~INSHIFT;
      break;
    case CAPSL:
      // kprintf("Capslock off detected\n");
      state &= ~CAPSLOCK;
      break;
    case LCTL:
      state &= ~INCTL;
      break;
    case LMETA:
      state &= ~INMETA;
      break;
    }
    
    return NOCHAR;
  }
  
  
  /* check for special keys */
  switch (code) {
    case LSHIFT:
    case RSHIFT:
        state |= INSHIFT;
        // kprintf("shift detected!\n");
        return NOCHAR;
    case CAPSL:
        state |= CAPSLOCK;
        // kprintf("Capslock ON detected!\n");
        return NOCHAR;
    case LCTL:
        state |= INCTL;
        return NOCHAR;
    case LMETA:
        state |= INMETA;
        return NOCHAR;
    case EXTESC:
        state |= EXTENDED;
        return NOCHAR;
  }
  
  ch = NOCHAR;
  
  if (code < sizeof(kbcode)){
    if ( state & CAPSLOCK )
      ch = kbshift[code];
	  else
	    ch = kbcode[code];
  }
  if (state & INSHIFT) {
    if (code >= sizeof(kbshift))
      return NOCHAR;
    if ( state & CAPSLOCK )
      ch = kbcode[code];
    else
      ch = kbshift[code];
  }
  if (state & INCTL) {
    if (code >= sizeof(kbctl))
      return NOCHAR;
    ch = kbctl[code];
  }
  if (state & INMETA)
    ch += 0x80;
  return ch;
}