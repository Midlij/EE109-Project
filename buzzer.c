// Include needed library and header files
#include "buzzer.h"
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Declare globals used in ISR
volatile unsigned char buzzer_state = 0;
volatile int buzzer_timer = 0;
volatile unsigned long frequency = 1000;

void buzzer_init(void)
{
	DDRC |= (1 << PC5);
	TCCR0A |= (1 << WGM00);
	TIMSK0 |= (1 << OCIE0A);
	OCR0A = 71;
}

ISR(TIMER0_COMPA_vect)
{
	if (!buzzer_state) 
	{
		PORTC |= (1 << PC5);
		buzzer_state = 1;
	}
	else 
	{
		PORTC &= ~(1 << PC5);
		buzzer_state = 0;
	}
	buzzer_timer++;

	if (buzzer_timer == frequency) 
	{
		TCCR0B &= ~(1 << CS01);
		TCNT0 = 0;
		PORTC &= ~(1 << PC5);
	}
}

void play_note(void)
{
	buzzer_timer = 0;
	TCCR0B |= (1 << CS01);
}
