#include "osinternal.h"

typedef struct event_descriptor {
	unsigned int signaled; // 1 if signaled, 0 otherwise
	unsigned int is_waiter; // 1 if a process if waiting on this event, 0 otherwise
	PID waiting; // the pid of the process waiting on this event
} EVENT_DESCRIPTOR;

volatile static EVENT_DESCRIPTOR events[MAXEVENT];
volatile static unsigned int numberofevents = 0;

EVENT Kernel_Event_Init(void)
{
	EVENT eid = numberofevents;

	if(numberofevents == MAXEVENT) {
		// ILLEGAL STATE, LOOP FOREVER, TOO MANY EVENTS
		for (;;);
	}

	numberofevents++;

	volatile EVENT_DESCRIPTOR* evt = &(events[eid]);
	evt->signaled = 0;
	evt->is_waiter = 0;
	evt->waiting = 0;

	Cp->passthrough = eid;
	return eid;
}

void Kernel_Event_Wait(EVENT e)
{
	volatile EVENT_DESCRIPTOR* evt = &(events[e]);

	// if the event is already signaled
	if (evt->signaled)
	{
		// unsignal it and immediatly return
		evt->signaled = 0;
		return;
	}
	else // the event is not signaled
	{
		if (evt->is_waiter) // if there already is a process waiting
		{
			return; // no-op
		}
		else
		{
			// set the current process as the waiter
			evt->is_waiter = 1;
			evt->waiting = Cp->pid;

			// and block it
			Process[evt->waiting].state = BLOCKED;
		}
	}

}

void Kernel_Event_Signal(EVENT e)
{
	volatile EVENT_DESCRIPTOR* evt = &(events[e]);

	// if the event is already signaled
	if (evt->signaled)
	{
		// do nothing, signalling is idempotent
		return;
	}
	else // the event is unsignaled
	{
		// if there is a process waiting
		if (evt->is_waiter)
		{
			// wake up that process
			if (Process[evt->waiting].state == BLOCKED)
			{
				Process[evt->waiting].state = READY;
			}
			else if (Process[evt->waiting].state == BLOCKED_SUSPENDED)
			{
				Process[evt->waiting].state = SUSPENDED;
			}
			else
			{
				// ILLEGAL STATE, LOOP FOREVER
				for (;;);
			}

			// remove the waiter
			evt->is_waiter = 0;
			evt->waiting = 0;
		}
		else // there is no process waiting
		{
			// mark the event as signaled
			evt->signaled = 1;
		}
	}
}

EVENT Event_Init(void)
{
	if (KernelActive) {
		Disable_Interrupt();
		Cp->request = EVENT_INIT;
		asm("call Enter_Kernel"::);
		return Cp->passthrough;
	}
	else {
		return Kernel_Event_Init();
	}
}

void Event_Wait(EVENT e)
{
	if (KernelActive) {
		Disable_Interrupt();
		Cp->request = EVENT_WAIT;
		Cp->passthrough = e;
		asm("call Enter_Kernel"::);
		return;
	}
	else {
		// ILLEGAL OPERATION, LOOP FOREVER
		for (;;);
		return;
	}
}

void Event_Signal(EVENT e)
{
	if (KernelActive) {
		Disable_Interrupt();
		Cp->request = EVENT_SIGNAL;
		Cp->passthrough = e;
		asm("call Enter_Kernel"::);
		return;
	}
	else {
		// ILLEGAL OPERATION, LOOP FOREVER
		for (;;);
		return;
	}
}
