#include "os.h"
//
// LAB - TEST 2
//  Noah Spriggs, Murray Dunne, Daniel McIlvaney

// EXPECTED RUNNING ORDER: P1,P2,P3,P1,P2,P3,P1
//
// P1 sleep              lock(attept)            lock
// P2      sleep                     resume
// P3           lock suspend               unlock

MUTEX mut;
volatile PID pid;
volatile int q;
void Task_P1(int parameter)
{
  int x;
    PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); 
  PORTB &= ~(1<<PB1); //pin 52 off//51 on

    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
  Task_Sleep(10); // sleep 100ms
    PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); 
  PORTB &= ~(1<<PB1); //pin 52 off//51 on

    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
  Mutex_Lock(mut);
    PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); 
  PORTB &= ~(1<<PB1); //pin 52 off//51 on

    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
    for(;;);
}

void Task_P2(int parameter)
{int x;
    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off
  PORTB &= ~(1<<PB1); //pin 52 off
  
    for ( x = 0; x < 32000; ++x )q = x;
  Task_Sleep(20); // sleep 200ms
    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off
  PORTB &= ~(1<<PB1); //pin 52 off
  
    for ( x = 0; x < 32000; ++x )q = x;
  Task_Resume(pid);
    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off
  PORTB &= ~(1<<PB1); //pin 52 off
  
    for ( x = 0; x < 32000; ++x )q = x;
    for(;;);
}

void Task_P3(int parameter)
{int x;
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    for ( x = 0; x < 32000; ++x )q = x;
  Mutex_Lock(mut);
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    for ( x = 0; x < 32000; ++x )q = x;
  Task_Suspend(pid);
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    for ( x = 0; x < 32000; ++x )q = x;
  Mutex_Unlock(mut);
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    for ( x = 0; x < 32000; ++x )q = x;
    for(;;);
}

void a_main(int parameter)
{
  /*
  //Place these as necessary to display output if not already doing so inside the RTOS
  //initialize pins
  DDRB |= (1<<PB1); //pin 52
  DDRB |= (1<<PB2); //pin 51  
  DDRB |= (1<<PB3); //pin 50
  
  
  PORTB |= (1<<PB1);  //pin 52 on
  PORTB |= (1<<PB2);  //pin 51 on
  PORTB |= (1<<PB3);  //pin 50 on


  PORTB &= ~(1<<PB1); //pin 52 off
  PORTB &= ~(1<<PB2); //pin 51 off
  PORTB &= ~(1<<PB3); //pin 50 off

  */

  DDRL |= (1<<PL0); //pin 49
  DDRL |= (1<<PL1); //pin 48
  DDRB |= (1<<PB0); //pin 53
  DDRB |= (1<<PB1); //pin 52
  DDRB |= (1<<PB2); //pin 51  
  DDRB |= (1<<PB3); //pin 50
  mut = Mutex_Init();

  Task_Create(Task_P1, 1, 0);
  Task_Create(Task_P2, 2, 0);
  pid = Task_Create(Task_P3, 3, 0);
}
