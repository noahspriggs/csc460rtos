#include "osinternal.h"

typedef struct mutex_descriptor {
  unsigned int locked; // 0 if free, 1 or greater if locked (could be locked multiple times)
  PID owner; // the current owner of the mutex
  PID queue[MAXTHREAD]; // the wait queue
  int queue_length; // the number of tasks waiting in the queue
  PRIORITY current_owner_original_priority; // the priority of the owner before it got the lock
} MUTEX_DESCRIPTOR;

volatile static MUTEX_DESCRIPTOR mutexes[MAXMUTEX];
volatile static unsigned int numberofmutexes = 0;
volatile static  int parent[MAXTHREAD] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
// the direct parent of a
// task is the one that has the highest priority (lower number) in it's wait queue
volatile static  int reverse_parent[MAXTHREAD] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
volatile static  int parent_reason[MAXTHREAD] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

MUTEX Kernel_Mutex_Init() {
  MUTEX mid = numberofmutexes;

  if (numberofmutexes == MAXMUTEX) {
    // ILLEGAL STATE, LOOP FOREVER, TOO MANY MUTEXES
    for (;;);
  }

  numberofmutexes++;

  volatile MUTEX_DESCRIPTOR* mut = &(mutexes[mid]);

  mut->locked = 0;
  mut->owner = 0;
  mut->queue_length = 0;

  Cp->passthrough = mid;
  return mid;
}


void Kernel_Mutex_Lock(MUTEX m) {

  volatile MUTEX_DESCRIPTOR* mut = &(mutexes[m]);
  int i = 0;

  // check if the mutex is free first
  if (mut->locked == 0)
  {
    // it's ours now
    mut->locked = 1;
    mut->owner = Cp->pid;
    mut->current_owner_original_priority = Cp->priority;
  }
  else if (mut->owner == Cp->pid) // still our mutex, locking again
  {
    // increment the lock count
    mut->locked++;
  }
  else // the mutex is not free
  {
    // add current process to end of queue
    mut->queue[mut->queue_length] = Cp->pid;
    mut->queue_length++;

    PID highest_priority_waiting_task = mut->owner;
    PRIORITY current_min = Process[mut->owner].priority;
    for (i = 0; i < mut->queue_length; i++)
    {
      if (Process[mut->queue[i]].priority < current_min)
      {
        highest_priority_waiting_task = mut->queue[i];

        current_min = Process[mut->queue[i]].priority;
      }
    }

    if (highest_priority_waiting_task != mut->owner)
    {
      parent[mut->owner] = highest_priority_waiting_task;
      reverse_parent[highest_priority_waiting_task] = mut->owner;
      parent_reason[mut->owner] = m;
    }

    PID parent_follow = highest_priority_waiting_task;
    while (reverse_parent[parent_follow] != -1) { //has children
      // actually set the new priority

      Change_Priority(reverse_parent[parent_follow], Process[parent_follow].priority);

      parent_follow = reverse_parent[parent_follow];
    }

    // now block the current process
    Cp->state = BLOCKED;
  }
  
}

void Kernel_Mutex_Unlock(MUTEX m) {
  volatile MUTEX_DESCRIPTOR* mut = &(mutexes[m]);
  int i = 0;
 
  // is it our mutex?
  if (mut->owner == Cp->pid)
  {
    
    // decrement the lock count
    mut->locked--;

    // if the lock count is now zero
    if (mut->locked == 0)
    {
      if (parent_reason[Cp->pid] == m)
      {
        // set the current tasks priority to its original priority
        
        Change_Priority(Cp->pid, mut->current_owner_original_priority);

      
        
        parent_reason[Cp->pid] = -1;
        reverse_parent[parent[Cp->pid]] = -1;
        parent[Cp->pid] = -1;
      }

      // is there someone waiting in the queue?
      if (mut->queue_length > 0)
      {
        
        // take the first member off the queue
        mut->owner = mut->queue[0];
        mut->locked = 1;
        mut->current_owner_original_priority = Process[mut->owner].priority;

        PID highest_priority_waiting_task = mut->owner;

        PRIORITY current_min = mut->current_owner_original_priority;
        for (i = 1; i < mut->queue_length; i++)
        {
          if (Process[mut->queue[i]].priority < current_min)
          {
            highest_priority_waiting_task = mut->queue[i];
            current_min = Process[mut->queue[i]].priority;
          }

          mut->queue[i - 1] = mut->queue[i];
        }

        // and shorten the queue
        mut->queue_length--;

        if (highest_priority_waiting_task != mut->owner)
        {
          parent[mut->owner] = highest_priority_waiting_task;
          reverse_parent[highest_priority_waiting_task] = mut->owner;
          parent_reason[mut->owner] = m;
        }

        PID parent_follow = highest_priority_waiting_task;
        while (reverse_parent[parent_follow] != -1) { //has children
          // actually set the new priority

          Change_Priority(reverse_parent[parent_follow], Process[parent_follow].priority);

          parent_follow = reverse_parent[parent_follow];
        }

        // now unblock the waiting process
        if (Process[mut->owner].state == BLOCKED)
        {
          Process[mut->owner].state = READY;
        }
        else if (Process[mut->owner].state == BLOCKED_SUSPENDED)
        {
          Process[mut->owner].state = SUSPENDED;
        }
        else
        {
          // ILLEGAL STATE, LOOP FOREVER
          for (;;);
        }
      }
      else // there was nobody in the queue
      {
        // nothing to do here, locked is already zero

      } // endif queue_length > 0
    } // endif locked == 0
  } // endif we're the owner
  else // not our mutex, not allowed to unlock it
  {
    // ILLEGAL OPERATION, LOOP FOREVER
    for (;;);
  }
}

void Kernel_Mutex_Unlock_All(PID task) {
  // called when a process is terminated, needs to release all the mutexes it holds
  int i = 0;
  int j = 0;
  PID temp = 0;
  volatile MUTEX_DESCRIPTOR* mut = &(mutexes[0]);

  temp = Cp->pid;
  Cp->pid = task;

  for (i = 0; i < numberofmutexes; i++) {
    mut = &(mutexes[i]);

    if (mut->owner == task) {
      for (j = mut->locked; j > 0; j--) {
        Kernel_Mutex_Unlock(i);
      }
    }
  }

  Cp->pid = temp;
}

MUTEX Mutex_Init(void) {
  if (KernelActive) {
    Disable_Interrupt();
    Cp->request = MUTEX_INIT;
    asm("call Enter_Kernel"::);
    return Cp->passthrough;
  }
  else {
    return Kernel_Mutex_Init();
  }
}

void Mutex_Lock(MUTEX m) {
  if (KernelActive) {
    Disable_Interrupt();
    Cp->request = MUTEX_LOCK;
    Cp->passthrough = m;
    asm("call Enter_Kernel"::);
    return;
  }
  else {
    // ILLEGAL OPERATION, LOOP FOREVER
    for (;;);
    return;
  }
}

void Mutex_Unlock(MUTEX m) {
  if (KernelActive) {
    Disable_Interrupt();
    Cp->request = MUTEX_UNLOCK;
    Cp->passthrough = m;
    asm("call Enter_Kernel"::);
    return;
  }
  else {
    // ILLEGAL OPERATION, LOOP FOREVER
    for (;;);
    return;
  }
}
