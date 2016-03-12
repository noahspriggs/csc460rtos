#ifndef _OSINTERNAL_H_  
#define _OSINTERNAL_H_  
#include "os.h"



typedef void (*voidfuncptr) (void);      /* pointer to void f(void) */


typedef enum process_states
{
  DEAD = 0,
  READY,
  RUNNING
} PROCESS_STATES;

/**
    This is the set of kernel requests, i.e., a request code for each system call.
*/
typedef enum kernel_request_type
{
  NONE = 0,
  CREATE,
  NEXT,
  TERMINATE
} KERNEL_REQUEST_TYPE;


typedef struct ProcessDescriptor
{
  volatile unsigned char *sp;   /* stack pointer into the "workSpace" */
  unsigned char workSpace[WORKSPACE];
  PROCESS_STATES state;
  voidfuncptr  code;   /* function to be executed as a task */
  KERNEL_REQUEST_TYPE request;
  PRIORITY priority;
  PRIORITY newpriority;
  int param;

} PD;


extern PD Process[MAXTHREAD];

volatile extern PD* Cp;

/** number of tasks created so far */
volatile extern unsigned int Tasks;

volatile extern unsigned int KernelActive;

#endif