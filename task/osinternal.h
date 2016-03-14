#ifndef _OSINTERNAL_H_  
#define _OSINTERNAL_H_  
#include "os.h"



typedef void (*voidfuncptr) (int);      /* pointer to void f(void) */


typedef enum process_states
{
  DEAD = 0,
  READY,
  RUNNING,
  SUSPENDED,
  BLOCKED,
  BLOCKED_SUSPENDED,
  SLEEPING,
  SLEEPING_SUSPENDED
} PROCESS_STATES;

/**
    This is the set of kernel requests, i.e., a request code for each system call.
*/
typedef enum kernel_request_type
{
  NONE = 0,
  CREATE,
  NEXT,
  TERMINATE,
  SUSPEND,
  RESUME,
  SLEEP,
  GETARG,
  MUTEX_INIT,
  MUTEX_LOCK,
  MUTEX_UNLOCK,
  EVENT_INIT,
  EVENT_WAIT,
  EVENT_SIGNAL
} KERNEL_REQUEST_TYPE;


typedef struct ProcessDescriptor
{
  PID pid;
  volatile unsigned char *sp;   /* stack pointer into the "workSpace" */
  unsigned char workSpace[WORKSPACE];
  PROCESS_STATES state;
  voidfuncptr  code;   /* function to be executed as a task */
  KERNEL_REQUEST_TYPE request;
  PRIORITY priority;
  PRIORITY newpriority;
  int param;
  int passthrough;
} PD;


extern PD Process[MAXTHREAD];

volatile extern PD* Cp;

/** number of tasks created so far */
volatile extern unsigned int Tasks;

volatile extern unsigned int KernelActive;

void Change_Priority(PID pid, PRIORITY py);

MUTEX Kernel_Mutex_Init();
void Kernel_Mutex_Lock(MUTEX m);
void Kernel_Mutex_Unlock(MUTEX m); 
EVENT Kernel_Event_Init(void);
void Kernel_Event_Wait(EVENT e);
void Kernel_Event_Signal(EVENT e);
void Kernel_Mutex_Unlock_All(PID p);

#endif
