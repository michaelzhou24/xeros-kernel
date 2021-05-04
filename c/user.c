/* user.c : User processes
 */

#include <xeroskernel.h>
#include <xeroslib.h>

int sleepMS =-1;
PID_t shellPID = -1;
int amp_flag = 0;
void test_2(void);
void test_1(void);
void test_zero(void);
void test_sys_read(void);
void test_sys_ioctl(void);
void test_sys_write(void);
void test_sys_open(void);
void test_sys_wait(void);
void test_sys_kill(void) ;
void test_sig_prio(void);
void kill_handler();
/*
void busy( void ) {
  int myPid;
  char buff[100];
  int i;
  int count = 0;

  myPid = sysgetpid();
  
  for (i = 0; i < 10; i++) {
    sprintf(buff, "My pid is %d\n", myPid);
    sysputs(buff);
    if (myPid == 2 && count == 1) syskill(3);
    count++;
    sysyield();
  }
}

void sleep1( void ) {
  int myPid;
  char buff[100];

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 1000 is %d\n", myPid);
  sysputs(buff);
  syssleep(1000);
  sprintf(buff, "Awoke 1000 from my nap %d\n", myPid);
  sysputs(buff);
}

void sleep2( void ) {
  int myPid;
  char buff[100];

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 2000 pid is %d\n", myPid);
  sysputs(buff);
  syssleep(2000);
  sprintf(buff, "Awoke 2000 from my nap %d\n", myPid);
  sysputs(buff);
}

void sleep3( void ) {
  int myPid;
  char buff[100];

  myPid = sysgetpid();
  sprintf(buff, "Sleeping 3000 pid is %d\n", myPid);
  sysputs(buff);
  syssleep(3000);
  sprintf(buff, "Awoke 3000 from my nap %d\n", myPid);
  sysputs(buff);
}

void producer( void ) {

    int         i;
    char        buff[100];


    // Sping to get some cpu time
    for(i = 0; i < 100000; i++);

    syssleep(3000);
    for( i = 0; i < 20; i++ ) {
      
      sprintf(buff, "Producer %x and in hex %x %d\n", i+1, i, i+1);
      sysputs(buff);
      syssleep(1500);

    }
    for (i = 0; i < 15; i++) {
      sysputs("P");
      syssleep(1500);
    }
    sprintf(buff, "Producer finished\n");
    sysputs( buff );
    sysstop();
}

void consumer( void ) {

    int         i;
    char        buff[100];

    for(i = 0; i < 50000; i++);
    syssleep(3000);
    for( i = 0; i < 10; i++ ) {
      sprintf(buff, "Consumer %d\n", i);
      sysputs( buff );
      syssleep(1500);
      sysyield();
    }

    for (i = 0; i < 40; i++) {
      sysputs("C");
      syssleep(700);
    }

    sprintf(buff, "Consumer finished\n");
    sysputs( buff );
    sysstop();
}
*/

void handler() {
  kprintf("Handler! \n");
}
void deez() {
  syssignal(10, &kill_handler); // Register kill_handler to sig 10
  sysyield();
  sysputs("Failure!\n"); 
}

void child(){
  PID_t deez_pid = syscreate(&deez, 1024);
  syswait(deez_pid);
  sysputs("I am awake: child \n");
}


void kill_handler() {
  sysstop();
}

void shell_ps(void) {
    processStatuses ps;
    char str[15];
    int procs = sysgetcputimes(&ps);

    kprintf("%s\t%s\t\t%s\t\n", "PID", "State", "Time");
    for (int i = 0; i <= procs; i++) {
        char* state;
        if (ps.status[i] == 1) {
          state = "READY";
        } else if (ps.status[i] == 22) {
          state = "SLEEP";
        } else if (ps.status[i] == 23) {
          state = "RUNNING";
        } else if (ps.status[i] == 24) {
          state = "WAIT";
        } else if (ps.status[i] == 25) {
          state = "READ";
        } else if (ps.status[i] == 26) {
          state = "WRITE";
        }
        kprintf("%d\t%s\t%l\t\n",  ps.pid[i], state, ps.cpuTime[i]);
    }
}

void handler_a(){
  sysputs("SIG21 SIG21\n");
}

void alarm() {
  syssleep(sleepMS);
  syskill(shellPID, 21);
}

void shell_t() {
  for(;;) {
  sysputs("T\n");
  syssleep(10000);
  }
  
}

// Sets ampersand flag and argument from shell input
void parse_input(char* str, char* arg, int* amp_flag) {
    int index = 0;

    while (*str != '\0' && *str != '&' && *str != ' ') {
        str++;
        index++;
    }
    while ((*str != '\0') && (*str == '&' || *str == ' ')) {
        str++;
        index++;
        if (*str == '&') {
            *amp_flag = 1;
        }
    }
    while (*str != '\0' && *str != '&' && *str != ' ') {
        *arg = *str;
        arg++;
        str++;
        index++;
    }
    *arg = '\0';
    while(*str != '\0') {
        str++;
        index++;
    }
    index--;
    str--; 
    while((index > 0) && (*str == '&' || *str == ' ')) {
        if (*str == '&') {
            *amp_flag = 1;
        }
        if (*str != ' ') {
            break;
        }
        str--;
        index--;
    }
}

void shell_a() {
  sleepMS = 0;
  PID_t alarmPID  = syscreate(&alarm, 4096);
  syssignal(21, &handler_a);
  if (!amp_flag) {
    syswait(alarmPID);
  }
  syssignal(21, NULL);
  amp_flag = 0;
}

void shell() {
  // TODO: Handle CTRL-D
  char buff[100];
  char* cmd_p = "p";
  char* cmd_x = "x";
  char* cmd_k = "k";
  char* cmd_a = "a";
  char* cmd_t = "t";
  char* arg = "";

  int kb = sysopen (KBMON);
  int ioctl_res = sysioctl(kb, CMD_ECHO_ON);
  
  sysputs("Welcome to the shell.\n");
  shell_start: 
  sysputs("\n>");
  int length = sysread(kb, buff, 100);
  if (length < 0) {
    sysputs("Error regarding read!\n");
  }
  if(length == 0) { //EOF
    sysputs("\n");
    sysstop();
  }
  parse_input(buff, arg, &amp_flag);
  // kprintf("input arg = %d, amp_flag = %d\n", atoi(arg), amp_flag);
  if (!strncmp(buff, cmd_p, 1)) {
    shell_ps();
  } else if (!strncmp(buff, cmd_x, 1)) {
    sysputs("\n");
    sysstop();
  } else if (!strncmp(buff, cmd_k, 1)) {
    syskill(atoi(arg), 31);
  } else if (!strncmp(buff, cmd_a, 1)) {
    shell_a();
  } else if (!strncmp(buff, cmd_t, 1)) {
    PID_t t_pid  = syscreate(&shell_t, 4096);
    sysyield();
  }
  goto shell_start;
}

void     root( void ) {

//TESTING:
  // PID_t testPID = syscreate(&test_sig_prio, 4096);
  // sysyield();
  // for(;;);

// SHELL:
/****************************/
    char buff[100];
    char buff2[100];
    syssignal(9, &kill_handler);
    
    int kb = sysopen (KBMON);
    beginning:
    sysputs("Welcome to Xeros 415 - A not very secure Kernel\n");
    int ret = sysioctl(kb, CMD_ECHO_ON);
    // kprintf("ret from ioctl is %d\n", ret);
    if(ret == -1) {
      sysputs("Error regarding ioctl! \n");
    }
    sysputs("Username: ");
    int ret1Buff = sysread(kb,buff, 100);
    sysputs("Password: ");
    ret = sysread(kb,buff2,100);
    if(ret == -1){
      sysputs("Error regarding read! \n");
    }
    char* user = "cs415";
    char* pw = "pw";
    // kprintf("user = %.*s pass: %.*s\n", ret1Buff, buff, ret, buff2);
    if (strncmp(buff, user, 5) != 0 || strncmp(buff2, pw, 2) != 0) {
        // flag = 1;
        sysputs("Wrong user or password!\n");
        goto beginning;
    } else {
      sysputs("Credentials correct :)\n");
    }
    sysclose(kb);
    
    shellPID = syscreate(&shell, 4096);
   
    sysyield();
    for(;;);
}
void testHanlder(){
  sysputs("In handler! \n");
}

void deez2() {
  sysputs("In deez2()\n");
}
void test_prio_a() {
    syssignal(10, &deez2);  
    syssignal(20, &testHanlder);
    sysyield();
    kprintf("I should be after handlers\n");
}
void test_sig_prio(void) {
    
    PID_t a = syscreate(&test_prio_a, 4096);
    sysyield();
    syskill(a, 10);
    syskill(a, 20);
    sysyield();
  sysputs("done\n");
}

void test_sig_handler(void) {
  syssignal(5, &deez);
  syskill(sysgetpid(), 5);
}

void test_sys_kill(void) {
  PID_t killed = syscreate(&deez, 4096);
  sysyield();
  // syssignal(21, &kill_handler);
  syskill(killed, 10);  // Deliver signal to killed
  sysyield();
  shell_ps();
}
void basic(){
  kprintf("in basic \n");
}
void test_sys_wait(void) {
  PID_t proc = syscreate(&basic, 4096);
  syswait(proc);
  sysputs("done waiting\n");
}

void test_sys_open(void) {
  int zero_fd = sysopen(-1);
  if (zero_fd == DEVICE_ERROR) {
    sysputs("testpassed!\n");
  }
  int f = sysopen(5);
  if (f == DEVICE_ERROR) {
    sysputs("testpassed!\n");
  }
  sysclose(zero_fd);
}

void test_sys_write(void) {
  char buff[30];
  int length = syswrite(-1, buff, 30);
  if (length == DEVICE_ERROR) {
    sysputs("test passed!\n");
  }
}

void test_sys_ioctl(void) {
  char buff[30];
  int kb = sysopen (KBMON);
  int ret = sysioctl(kb, 33);
  if(ret == -1) {
    sysputs("Test success: ioctl command returned -1\n");
  }
  sysclose(kb);
}

void test_sys_read(void) {
  // char buff[30];
  // int rand_fd = sysopen(1);
  // int length = sysread(rand_fd, buff, 30);
  // kprintf("%s", 30, buff);
  // sysclose(rand_fd);
  char buff[10];
  int kb = sysopen(KBMON);
  int ioctl_res = sysioctl(kb, CMD_ECHO_ON);
  // Write some stuff to fill kb buffer
  syssleep(5000); // Sleep for 3 s
  int res = sysread(kb, buff, 2);
  kprintf("Read returned: %.*s\n", res, buff);
  sysclose(kb);
}

void test_zero(void) {
  int zero_fd = sysopen(0);
  char test[4] = "abcd";
  char zero[4] = {0};
  sysread(zero_fd, test, 4);
  int sum = 0;
  for(int i = 0; i <4; i++ ) {
    sum |= test[i];
  }
  if(sum != 0) {
    sysputs("Failure: Buffer contains non zero\n");
  }
  //kprintf("Zero buff: %.*d\n", 4, test);
  int length = syswrite(zero_fd, test, 4);
  if (length == 4) {
    sysputs("write test passed!\n");
  }
  sysclose(zero_fd);
}

 // Test EOF for keyboard
void test_1(void) {
  // todo
  // EOF change to 'z'
  int kb = sysopen(KBMON);
  char buf[5];
  // Set on
  int ioctl_res = sysioctl(kb, CMD_EOF, 0x7A);
  // Should echo
  int res = sysread(kb, buf, 5);
  if(res == 5) {
    kprintf("Failure: EOF not reached!\n");
  }
  kprintf("Output from input: %.*s\n", res, buf);
  sysclose(kb);
  // TODO: Trap if we exit process without closing kb!

  // Turn off
  // ioctl_res = sysioctl(kb, CMD_ECHO_OFF);
  // Should not echo
  // sysread(kb, buf, 30);

}
// Test random
void test_2(void) {
  int r = sysopen(SERIAL0);
  char buf[30];
  int write = syswrite(r, buf, 30);
  if(write != -1) {
    sysputs("Write is not -1; test failed for rand! \n");
  }
  int read1 = sysread(r, buf, 30);
  int read2 = sysread(r, buf, 30);
  if(read1 == read2) {
    sysputs("Random read failed! \n");
  } else{
    sysputs("Random dev tests passed!\n");
  }

}

    /*
    char  buff[100];
    int pids[5];
    int proc_pid, con_pid;
    int i;

    sysputs("Root has been called\n");

    // Test for ready queue removal. 
   
    proc_pid = syscreate(&busy, 1024);
    con_pid = syscreate(&busy, 1024);
    sysyield();
    syskill(proc_pid);
    sysyield();
    syskill(con_pid);

    
    for(i = 0; i < 5; i++) {
      pids[i] = syscreate(&busy, 1024);
    }

    sysyield();
    
    syskill(pids[3]);
    sysyield();
    syskill(pids[2]);
    syskill(pids[4]);
    sysyield();
    syskill(pids[0]);
    sysyield();
    syskill(pids[1]);
    sysyield();

    syssleep(8000);;



    kprintf("***********Sleeping no kills *****\n");
    // Now test for sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    sysyield();
    syssleep(8000);;



    kprintf("***********Sleeping kill 2000 *****\n");
    // Now test for removing middle sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    syssleep(110);
    syskill(pids[1]);
    syssleep(8000);;

    kprintf("***********Sleeping kill last 3000 *****\n");
    // Now test for removing last sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    sysyield();
    syskill(pids[2]);
    syssleep(8000);;

    kprintf("***********Sleeping kill first process 1000*****\n");
    // Now test for first sleeping processes
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);

    syssleep(100);
    syskill(pids[0]);
    syssleep(8000);;

    // Now test for 1 process


    kprintf("***********One sleeping process, killed***\n");
    pids[0] = syscreate(&sleep2, 1024);

    sysyield();
    syskill(pids[0]);
    syssleep(8000);;

    kprintf("***********One sleeping process, not killed***\n");
    pids[0] = syscreate(&sleep2, 1024);

    sysyield();
    syssleep(8000);;



    kprintf("***********Three sleeping processes***\n");    // 
    pids[0] = syscreate(&sleep1, 1024);
    pids[1] = syscreate(&sleep2, 1024);
    pids[2] = syscreate(&sleep3, 1024);


    // Producer and consumer started too
    proc_pid = syscreate( &producer, 4096 );
    con_pid = syscreate( &consumer, 4096 );
    sprintf(buff, "Proc pid = %d Con pid = %d\n", proc_pid, con_pid);
    sysputs( buff );


    processStatuses psTab;
    int procs;
    



    syssleep(500);
    procs = sysgetcputimes(&psTab);

    for(int j = 0; j <= procs; j++) {
      sprintf(buff, "%4d    %4d    %10d\n", psTab.pid[j], psTab.status[j], 
	      psTab.cpuTime[j]);
      kprintf(buff);
    }


    syssleep(10000);
    procs = sysgetcputimes(&psTab);

    for(int j = 0; j <= procs; j++) {
      sprintf(buff, "%4d    %4d    %10d\n", psTab.pid[j], psTab.status[j], 
	      psTab.cpuTime[j]);
      kprintf(buff);
    }

    sprintf(buff, "Root finished\n");
    sysputs( buff );
    sysstop();
    
    for( ;; ) {
     sysyield();
    }
    */
    