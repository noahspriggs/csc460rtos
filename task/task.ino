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


volatile unsigned int NextP;

/** number of tasks created so far */
volatile unsigned int Tasks;

volatile unsigned int KernelActive;

volatile unsigned char *KernelSp;

volatile unsigned char *CurrentSp;\


static void Dispatch()
{
  /* find the next READY task
      Note: if there is no READY task, then this will loop forever!.
  */
  while (Process[NextP].state != READY) {
    NextP = (NextP + 1) % MAXTHREAD;
  }

  Cp = &(Process[NextP]);
  CurrentSp = Cp->sp;
  Cp->state = RUNNING;

  NextP = (NextP + 1) % MAXTHREAD;
}

void Task_Yield()
{


  if (KernelActive) {
    Disable_Interrupt();
    Cp ->request = NEXT;

    
  PORTL &= ~(1 << PL1);
    asm ( "call Enter_Kernel":: );
    
  PORTL |= (1 << PL1);
    Enable_Interrupt();
  }
}

void Task_Terminate()
{
  if (KernelActive) {
    Disable_Interrupt();
    Cp -> request = TERMINATE;
    asm ( "call Enter_Kernel":: );
    //Enter_Kernel();
    /* never returns here! */
  }
}


int  Task_GetArg(void) {
  return Cp->param;
}


void Task_Suspend( PID p ) {
  //if already blocking, use BLOCKED_SUSPENDED state to indicate
  if (Process[p].state == BLOCKED) {
    Process[p].state = BLOCKED_SUSPENDED;
  //from any other state new state = SUSPENDED
  } else {
    Process[p].state = SUSPENDED;
  }

  //if suspended ourself, enter kernel, otherwise continue (suspended process is not currently running so we dont need to stop anything)
  if (p == Cp->pid) {
    asm ( "call Enter_Kernel":: );
  }
} 

void Task_Resume( PID p ) {
  //if called resume on self, has no effect, was obviously not suspended
  if(p != Cp->pid) {
    if(Process[p].state == BLOCKED_SUSPENDED) {
      Process[p].state = BLOCKED;

    } else {
      Process[p].state = READY;
    }
  }
}


void Task_Sleep(TICK t) {

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
  sp = sp - 35;

  p->sp = sp;   /* stack pointer into the "workSpace" */
  p->code = f;    /* function to be executed as a task */
  p->request = NONE;


  p->state = READY;


  Cp->passthrough = x;
  return x;

}

int main() {
  int x;

  Tasks = 0;
  KernelActive = 0;
  NextP = 0;
  //Reminder: Clear the memory for the task on creation.
  for (x = 0; x < MAXTHREAD; x++) {
    memset(&(Process[x]), 0, sizeof(PD));
    Process[x].state = DEAD;
  }

  //setup tasks
  a_main(0);


  Disable_Interrupt();
  /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

  /* here we go...  */
  KernelActive = 1;

  Dispatch();  /* select a new task to run */

  while (1) {
    int x;
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
    
    if (Cp->request == CREATE) {
        Kernel_Create_Task( Cp->code , Cp->newpriority, Cp->param);
    } else if (Cp->request == NONE || Cp->request == NEXT) {
      Cp->state = READY;
        Dispatch();
    } else {
      Cp->state = DEAD;
        Dispatch();
    }

  }
  /* NEVER RETURNS!!! */
  
  return 0;
}