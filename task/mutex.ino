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

MUTEX Kernel_Mutex_Init() {
	MUTEX mid = numberofmutexes;

	if(numberofmutexes == MAXMUTEX) {
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

		// promote the process with the lock to the highest priority in the queue
		// or it's original priority, whichever is higher
		PRIORITY current_min = mut->current_owner_original_priority;
		for (i = 0; i < mut->queue_length; i++)
		{
			if (Process[mut->queue[i]].pid < current_min)
			{
				current_min = Process[mut->queue[i]].pid;
			}
		}

		// actually set the new priority
		Change_Priority(mut->owner, current_min);

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
			// set the current tasks priortity to its original priority
			Cp->priority = mut->current_owner_original_priority;

			// is there someone waiting in the queue?
			if (mut->queue_length > 0)
			{
				// take the first member off the queue
				mut->owner = mut->queue[0];
				mut->locked = 1;
				mut->current_owner_original_priority = Process[mut->owner].priority;

				// shuffle the queue along (and keep note of the max priortiy) becuase we need
				// to promote the process with the lock to the highest priority in the queue
				// or it's original priority, whichever is higher
				PRIORITY current_min = mut->current_owner_original_priority;

				for (i = 1; i < mut->queue_length; i++)
				{
					// update maximum
					if (Process[mut->queue[i]].pid < current_min)
					{
						current_min = Process[mut->queue[i]].pid;
					}

					// shuffle queue along
					mut->queue[i - 1] = mut->queue[i];
				}

				// actually set the new priority
				Change_Priority(mut->owner, current_min);

				// and shorten the queue
				mut->queue_length--;

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
