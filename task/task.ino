#include "os.h"
#include "osinternal.h"



extern void a_main(int i);

extern void CSwitch();
extern void Exit_Kernel();    /* this is the same as CSwitch() */
extern void Enter_Kernel();

#define Disable_Interrupt()   asm volatile ("cli"::)
#define Enable_Interrupt()    asm volatile ("sei"::)


PD Process[MAXTHREAD];

volatile PD* Cp;



/** number of tasks created so far */
volatile unsigned int Tasks;

volatile unsigned int KernelActive;

volatile unsigned char *KernelSp;

volatile unsigned char *CurrentSp;

PID RunList[MINPRIORITY][MAXTHREAD];

int PriorityCounts[MINPRIORITY];

volatile int WakeUpTime[MAXTHREAD];

void Remove_From_RunList(PID pid) { 
  PRIORITY currentPriority = Process[pid].priority;
  int i = 0;
    while(RunList[currentPriority][i] != pid) {
      i++;
    }
    while(i<MAXTHREAD-1) {
      RunList[currentPriority][i] = RunList[currentPriority][i+1];
      i++;
    }
    PriorityCounts[currentPriority]--;
}

void Add_To_RunList(PID pid) {
    int i=0;
    PRIORITY py = Process[pid].priority;
    RunList[py][PriorityCounts[py]] = pid;
    PriorityCounts[py]++;
}

void Change_Priority(PID pid, PRIORITY py) {
  
  if (Process[pid].priority != py) {
    Remove_From_RunList(pid);
    Process[pid].priority = py;
    Add_To_RunList(pid);
  }
}

void NextP(PRIORITY py) {
  PID p = RunList[py][0];
  Remove_From_RunList(p);
  Add_To_RunList(p);
}

int next = 0;

static int Dispatch()
{
  /* find the next READY task
      Note: if there is no READY task, then this will loop forever!.
  */
      
/*  while (Process[next].state != READY) {
    next = (next + 1) % MAXTHREAD;
  }
*/
    int breaker = 0;
    //for each priority, starting from highest
    for(int i = 0; i<MINPRIORITY;i++) {
      //check each element, if one is found ready, the use it
      for(int j = 0; j<PriorityCounts[i]; j++) {
        if (Process[RunList[i][0]].state == READY) {

          Cp = &(Process[RunList[i][0]]);
          breaker = 1;
          break;
        }

        NextP(i);
      }
      if(breaker) break;
      if (i == MINPRIORITY-1) {
        return 0;
      }
    }

  CurrentSp = Cp->sp;
  Cp->state = RUNNING;

  NextP(Cp->priority);
  return 1;
}

void Task_Yield()
{


  if (KernelActive) {
    Disable_Interrupt();
    Cp ->request = NEXT;

    
    asm ( "call Enter_Kernel":: );
    
  
   // Enable_Interrupt();
  }
}

void Task_Terminate()
{

  if (KernelActive) {
    Disable_Interrupt();
    Cp->request = TERMINATE;

    asm ( "call Enter_Kernel":: );
  }
}

void Kernel_Task_Terminate() {
  Kernel_Mutex_Unlock_All(Cp->pid);
  Remove_From_RunList(Cp->pid);
  Cp->state = DEAD;
}


int  Task_GetArg(void) {
  if(KernelActive){
    Disable_Interrupt();
    Cp->request = GETARG;

    asm ( "call Enter_Kernel":: );

    return Cp->passthrough;
  } else {
    for(;;);
  }
}


void Kernel_Task_Suspend( PID p ) {
  //if already blocking, use BLOCKED_SUSPENDED state to indicate
  if (Process[p].state == BLOCKED) {
    Process[p].state = BLOCKED_SUSPENDED;
  //from any other state new state = SUSPENDED
  } else if (Process[p].state == SLEEPING) {
    Process[p].state = SLEEPING_SUSPENDED;
  } else {
    Process[p].state = SUSPENDED;
  }

} 


void Task_Suspend(PID p) {
  if(KernelActive){
    Disable_Interrupt();
    Cp->request = SUSPEND;
    Cp->passthrough = p;

    asm ( "call Enter_Kernel":: );
  } else {
    Kernel_Task_Suspend(p);
  }

}

void Kernel_Task_Resume( PID p ) {
  //if called resume on self, has no effect, was obviously not suspended
  if(p != Cp->pid) {
    if(Process[p].state == BLOCKED_SUSPENDED) {
      Process[p].state = BLOCKED;

    } else if (Process[p].state == SLEEPING_SUSPENDED) {
      Process[p].state = SLEEPING;
    } else {
      Process[p].state = READY;
    }
  }
}

void Task_Resume(PID p) {
  if(KernelActive){
    Disable_Interrupt();
    Cp->request = RESUME;
    Cp->passthrough = p;

    asm ( "call Enter_Kernel":: );
  } else {
    //illegal operation
    for(;;);
  }

}


void Task_Sleep(TICK t) {
  if(KernelActive){
    Disable_Interrupt();
    Cp->request = SLEEP;
    Cp->passthrough = t;

    asm ( "call Enter_Kernel":: );
  } else {
     //illegal operation
    for(;;);
  }
}

void Kernel_Task_Sleep(TICK t) {
  WakeUpTime[Cp->pid] = t+1;
  Cp->state = SLEEPING;
}

PID  Task_Create( void (*f)(int), PRIORITY py, int arg) {
  if (KernelActive){
  Disable_Interrupt();
    Cp->request = CREATE;
    Cp->code = f;
    Cp->newpriority = py;
    Cp->param = arg;

    asm ( "call Enter_Kernel":: );

    return Cp->passthrough;
  
  } else {
    return Kernel_Create_Task(f,py,arg);
  }
  
}



/**
     Create a new task
*/
static PID Kernel_Create_Task( voidfuncptr f, PRIORITY py, int arg )
{
  int x;

  if (Tasks == MAXTHREAD) return -1;  /* Too many task! */

  /* find a DEAD PD that we can use  */
  for (x = 0; x < MAXTHREAD; x++) {
    if (Process[x].state == DEAD) break;
  }

  ++Tasks;
  //Kernel_Create_Task_At( &(Process[x]), f );

  PD* p = &(Process[x]);
  p->pid = x;
  p->priority = py;

  Add_To_RunList(x);

  unsigned char *sp;
  //Changed -2 to -1 to fix off by one error.
  sp = (unsigned char *) & (p->workSpace[WORKSPACE - 1]);



  /*----BEGIN of NEW CODE----*/
  //Initialize the workspace (i.e., stack) and PD here!

  //Clear the contents of the workspace
  memset(&(p->workSpace), 0, WORKSPACE);

  //Notice that we are placing the address (16-bit) of the functions
  //onto the stack in reverse byte order (least significant first, followed
  //by most significant).  This is because the "return" assembly instructions
  //(rtn and rti) pop addresses off in BIG ENDIAN (most sig. first, least sig.
  //second), even though the AT90 is LITTLE ENDIAN machine.

  //Store terminate at the bottom of stack to protect against stack underrun.
  *(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;


  //TODO: ACCEPT PARAMETER (EITHER ON STACK OR REGISTER)

  //Place return address of function at bottom of stack
  *(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;

  //Place stack pointer at top of stack
  sp = sp - 25;

  //pass the argument into the appropriate register
  *(unsigned char *)sp-- = ((unsigned int)arg) & 0xff;
  *(unsigned char *)sp-- = (((unsigned int)arg) >> 8) & 0xff;

  sp = sp-8;

  p->sp = sp;   /* stack pointer into the "workSpace" */
  p->code = f;    /* function to be executed as a task */
  p->request = NONE;


  p->state = READY;


  Cp->passthrough = x;
  return x;

}

void BackgroundTask(int parameter) {
  for(;;) {
  PORTL |= (1<<PL1);
  Task_Yield();
  
  PORTL &= ~(1<<PL1);
  }
}

volatile boolean ticks = 0;
// timer3 isr
ISR(TIMER3_COMPA_vect) {
  PORTL |= (1<<PL0);
  for(int i = 0; i<MAXTHREAD;i++) {
    if(WakeUpTime[i]>1) {
      WakeUpTime[i]--;    
    } else if (WakeUpTime[i] == 1) {
      WakeUpTime[i] = 0;
      if(Process[i].state == SLEEPING_SUSPENDED) {
        Process[i].state = SUSPENDED;  
      } else {
        Process[i].state = READY;  
      }
        
    }
  }
  PORTL &= ~(1<<PL0);
}

void Timer_Init() {
 
  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;
  //Set to CTC (mode 4)
  TCCR3B |= (1 << WGM32);

  //Set prescaler to 256
  TCCR3B |= (1 << CS02);

  //Reset at counter of 61
  OCR3A = 624; //this number should be (16*10^6) / (desired freq*prescaler) - 1

  //Enable interupt A for timer 3.
  TIMSK3 |= (1 << OCIE3A);

  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
}

int main() {
  int x;

  Timer_Init();

  
  Tasks = 0;
  KernelActive = 0;
  //Reminder: Clear the memory for the task on creation.
  for (x = 0; x < MAXTHREAD; x++) {
    memset(&(Process[x]), 0, sizeof(PD));
    Process[x].state = DEAD;
    WakeUpTime[x] = 0;
  }

  memset(PriorityCounts,0,sizeof(int)*MINPRIORITY);


  Task_Create(BackgroundTask,9,0);
  //setup tasks
  
  a_main(0);


  Disable_Interrupt();
  /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

  /* here we go...  */
  KernelActive = 1;


  while (1) {
    Dispatch();
    Cp->request = NONE; /* clear its request */

    /* activate this newly selected task */
    CurrentSp = Cp->sp;
    
  PORTB &= ~(1<<PB0); //PIN 53 OFF 
  
    asm ( "call Exit_Kernel":: );
   

  PORTB |= (1<<PB0); //PIN 53 ON
    /* if this task makes a system call, it will return to here! */

    
    /* save the Cp's stack pointer */
    Cp->sp = CurrentSp;

   // Cp->state = READY;
        //Dispatch();
    
    Cp->state = READY;

    if (Cp->request == CREATE) {
        Kernel_Create_Task( Cp->code , Cp->newpriority, Cp->param);
    } else if (Cp->request == NONE || Cp->request == NEXT) {
      //DO NOTHING
    } else if (Cp->request == SUSPEND) {
      Kernel_Task_Suspend(Cp->passthrough);
    } else if (Cp->request == RESUME) {
      Kernel_Task_Resume(Cp->passthrough);
    } else if (Cp->request == SLEEP) {
      Kernel_Task_Sleep(Cp->passthrough);
    } else if (Cp->request == MUTEX_INIT) {
      Kernel_Mutex_Init();
    } else if (Cp->request == MUTEX_LOCK) {
      Kernel_Mutex_Lock(Cp->passthrough);
    } else if (Cp->request == MUTEX_UNLOCK) {
      Kernel_Mutex_Unlock(Cp->passthrough);
    } else if (Cp->request == EVENT_INIT) {
      Kernel_Event_Init();
    } else if (Cp->request == EVENT_WAIT) {
      Kernel_Event_Wait(Cp->passthrough);
    } else if (Cp->request == EVENT_SIGNAL) {
      Kernel_Event_Signal(Cp->passthrough);
    } else if (Cp->request == GETARG) {
      Cp->passthrough = Cp->param;
    } else {
      Kernel_Task_Terminate();
    }

  }
  /* NEVER RETURNS!!! */
  
  return 0;
}
