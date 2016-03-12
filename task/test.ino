#include "os.h"

int t1,t2,t3;

int q;
void Task_P1(int parameter)
{
	int  x ;
  for (;;) {


    PORTB &= ~(1 << PB3 );
    PORTB |= (1 << PB2 ); //51 on

    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */
    for ( x = 0; x < 32000; ++x )q = x; /* do nothing */


    Task_Suspend(t1);
  }
}

void Task_P2(int parameter)
{
	int  x;

  for (;;) {

    PORTB &= ~(1 << PB2 ); 
      PORTB |= (1 << PB3 ); //51 off

    for ( x = 0; x < 32000; ++x )q = x;
    for ( x = 0; x < 32000; ++x )q = x;
    for ( x = 0; x < 32000; ++x )q = x;

    
    Task_Suspend(t2);

  }
}

void Task_P3(int parameter)
{
  int  x;

  for (;;) {

    PORTB &= ~(1 << PB2 ); 
      PORTB &= ~(1 << PB3 ); //51 off

    for ( x = 0; x < 32000; ++x )q = x;
    for ( x = 0; x < 32000; ++x )q = x;
    for ( x = 0; x < 32000; ++x )q = x;


    Task_Resume(t1);
    Task_Resume(t2);
    Task_Yield();

  }
}


void a_main(int parameter)
{
	/*
	//Place these as necessary to display output if not already doing so inside the RTOS
	//initialize pins
	DDRB |= (1<<PB1);	//pin 52
	DDRB |= (1<<PB2);	//pin 51	
	DDRB |= (1<<PB3);	//pin 50
	
	
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB |= (1<<PB3);	//pin 50 on


	PORTB &= ~(1<<PB1);	//pin 52 off
	PORTB &= ~(1<<PB2);	//pin 51 off
	PORTB &= ~(1<<PB3);	//pin 50 off

	*/

  DDRB |= (1<<PB0); //pin 52
  DDRB |= (1<<PB1); //pin 52
  DDRB |= (1<<PB2); //pin 51  
  DDRB |= (1<<PB3); //pin 50
	t1 = Task_Create(Task_P1, 1, 0);
	t2 = Task_Create(Task_P2, 1, 0);

  t3 = Task_Create(Task_P3, 2, 0);
}
