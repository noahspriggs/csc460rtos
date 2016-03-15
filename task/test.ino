#include "os.h"
//
// LAB - TEST 4
//  Noah Spriggs, Murray Dunne, Daniel McIlvaney
//
// EXPECTED RUNNING ORDER: P0,P1,P2,P3,TIMER,P0,P1,P2
//
// P0     wait 3            signal 2, signal 1
// P1     wait 1
// P2     wait 2
// P3            timer->signal 3

MUTEX mut;
EVENT evt1;
EVENT evt2;
EVENT evt3;
volatile int q;
void pin(int task) {
  int x;
  if (task == 1) {
  PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); 
  PORTB &= ~(1<<PB1); //pin 52 off//51 on
    //for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
    
   PORTB &= ~(1 << PB2 ); 
    } else if (task == 2) {
    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off
  PORTB &= ~(1<<PB1); //pin 52 off
  
    //for ( x = 0; x < 32000; ++x )q = x;
    
      PORTB &= ~(1 << PB3 ); //51 off
    } else {
    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off
  PORTB |= (1<<PB1);  //pin 52 on

    //for ( x = 0; x < 32000; ++x )q = x;
    
  PORTB &= ~(1<<PB1);  //pin 52 on
    }
}
void Task_P0(int parameter)
{
  for(;;)
  {
    pin(1);
    Event_Wait(evt3);
    pin(1);
    Event_Signal(evt2);
    pin(1);
    Event_Signal(evt1);   
    pin(1);
  }
}

void Task_P1(int parameter)
{
    pin(2);
  Event_Wait(evt1);
    pin(2);
    Task_Terminate();
    pin(2);
    for(;;);
}

void Task_P2(int parameter)
{
    pin(3);
  Event_Wait(evt2);
    pin(3);
    Task_Terminate();
    pin(3);
    for(;;);
}

void Task_P3(int parameter)
{
    for(;;) {
  PORTL &= ~(1<<PL0);
  
  PORTL |= (1<<PL0);
    
    }
  }

void configure_timer()
{
    //Clear timer config.
    TCCR3A = 0;
    TCCR3B = 0;  
    //Set to CTC (mode 4)
    TCCR3B |= (1<<WGM32);

    //Set prescaller to 256
    TCCR3B |= (1<<CS32);

    //Set TOP value (0.1 seconds)
    OCR3A = 6250;

    //Set timer to 0 (optional here).
    TCNT3 = 0;
    
    //Enable interupt A for timer 3.
    TIMSK3 |= (1<<OCIE3A);
}

void timer_handler()
{
    Event_Signal(evt3);
}

ISR(TIMER3_COMPA_vect)
{
    TIMSK3 &= ~(1<<OCIE3A);
    timer_handler();
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

  */ DDRL |= (1<<PL0); //pin 49
  DDRL |= (1<<PL1); //pin 48
  DDRB |= (1<<PB0); //pin 53
  DDRB |= (1<<PB1); //pin 52
  DDRB |= (1<<PB2); //pin 51  
  DDRB |= (1<<PB3); //pin 50
  evt1 = Event_Init();
    evt2 = Event_Init();
    evt3 = Event_Init();

  Task_Create(Task_P1, 1, 0);
  Task_Create(Task_P2, 2, 0);
  Task_Create(Task_P0, 0, 0);
  Task_Create(Task_P3, 3, 0);
  
    configure_timer();
}
