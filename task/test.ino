#include "os.h"
//
// LAB - TEST 3
//  Noah Spriggs, Murray Dunne, Daniel McIlvaney
//
// EXPECTED RUNNING ORDER: P1,P2,P3,P2,P1,P3,P2,P1
//
// P1 sleep                     lock 1                               unlock 1
// P2 lock 1  sleep    lock 2                    unlock 2 unlock 1
// P3 sleep   lock 2   sleep           unlock 2

MUTEX mut1;
MUTEX mut2;
volatile int q;
void pin(int task) {
  int x;
  if (task == 1) {
  PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); 
  PORTB &= ~(1<<PB1); //pin 52 off//51 on
    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
    
    PORTB &= ~(1 << PB2 ); 
    } else if (task == 2) {
    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off
  PORTB &= ~(1<<PB1); //pin 52 off
  
    for ( x = 0; x < 32000; ++x )q = x;
    
      PORTB &= ~(1 << PB3 ); //51 off
    } else {
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    for ( x = 0; x < 32000; ++x )q = x;
    
  PORTB &= ~(1<<PB1);  //pin 52 on
    }
}


void Task_P1(int parameter)
{
  pin(1);
  Task_Sleep(30);
  
  pin(1);
  Mutex_Lock(mut1);
    
    pin(1);
    Mutex_Unlock(mut1);
    
    pin(1);
    for(;;);
}

void Task_P2(int parameter)
{
  pin(2);
    Mutex_Lock(mut1);
    
  pin(2);
    Task_Sleep(20);
  pin(2);
    Mutex_Lock(mut2);
  pin(2);
    Task_Sleep(10);
  pin(2);
    Mutex_Unlock(mut2);
  pin(2);
  Mutex_Unlock(mut1);
  pin(2);
    for(;;);
}

void Task_P3(int parameter)
{
  pin(3);
  Task_Sleep(10);
  pin(3);
    Mutex_Lock(mut2);
  pin(3);
    Task_Sleep(20);
  pin(3);
    Mutex_Unlock(mut2);
  pin(3);
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

  */  DDRL |= (1<<PL0); //pin 49
  DDRL |= (1<<PL1); //pin 48
  DDRB |= (1<<PB0); //pin 53
  DDRB |= (1<<PB1); //pin 52
  DDRB |= (1<<PB2); //pin 51  
  DDRB |= (1<<PB3); //pin 50
  mut1 = Mutex_Init();
    mut2 = Mutex_Init();

  Task_Create(Task_P1, 1, 0);
  Task_Create(Task_P2, 2, 0);
  Task_Create(Task_P3, 3, 0);
}
