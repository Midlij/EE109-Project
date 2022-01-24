/********************************************
 *
 *  Name: Khaled Alzamel
 *  Email: Kalzamel@usc.edu
 *  Section: 11 AM FRIDAY
 *  Assignment: Project
 *
 ********************************************/

// Include needed library and header files
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "lcd.h"
#include "TempMainEncoder.h"
#include "buzzer.h"

// Define values for variables
#define Clock 16000000
#define MaxTime 46400
#define Prescaler 8
#define trigger_delay 10

// Declaring variables 
volatile unsigned char range_timer_flag = 0;	// Flag to stop or keep measuring until we get a value for echo
volatile int changed = 0;						// Flag for if we have received a value for echo
volatile unsigned long pulse_count;				// counter of pulses for the ultrasonic sensor
volatile int temp;  							// Variable in distance_of_rangefinder function to save calculated distance
volatile unsigned char out_of_range; 			// flag for if distance > 400 cm
volatile char echo = 0;							// flag to check PD3 for a value 

// Prototypes
char checkInput(void);
unsigned long distance_of_rangefinder(void);
void call_buzzer(void);

// Button Initialization
void button_init(void)
{
	PORTB |= (1 << PB4);
	PORTC |= (1 << PC3);
	PCICR |= (1 << PCIE2);
	PCMSK2 |= (1 << PCINT19);
}

// LED Initialization
void led_init(void)
{
	// Enable RGB LED's Blue and Green Segments
	
	// Green Segment 
	DDRB |= (1 << PB3);      // Set as PB3 output 
	
	// Blue Segment 
	DDRC |= (1 << PC4);       // Set as PC4 output
}

// Timer2 Initialization
void init_timer2(void)
{
	// Set to TOP mode
	TCCR2B &= ~(1 << WGM22);
	TCCR2A |= (1 << WGM21) | (1 << WGM20);
	
	TCCR2B |= (1 << CS22); 
	TCCR2B &= ~((1 << CS21) | (1 << CS20));
	
	TCCR2A |= (1 << COM2A1);
	TCCR2A &= ~(1 << COM2A0);
	
	// Load register with value 128
	OCR2A = 128;	
}

int main(void) {

	// Initialize LCD
	lcd_init();
	
	// Initialize timer 1 and timer 2
	init_timer1();
	init_timer2();
	
	// Enable global interrupts 
	sei();

	// Display splash screen with student's name
	lcd_splash();
	
	// Initialize encoder, EEPROM, and LED
	encoder_init();
	store_eeprom();
	led_init();
	
	// Display thresholds "near" and "far" on the screen
	threshold_screen_display();
	
	// Initialize buttons and buzzer
	button_init();
	buzzer_init();
	
	// Initialize rangefinder 
	rangefinder_init();
	
	while (1) {
	
		// Print "near" and "far" thresholds on the LCD screen, and save both thresholds in EEPROM
		// This function is found in TempMainEncoder.c
		encoder_main();
		
		if(checkInput())		// If PB4 is pressed
		{
			out_of_range = 0;		// Flag to verify that the distance measured by the ultrasonic sensor is not out_of_range
			rangefinder_trigger();	// Trigger PD3 to send a signal to the ultrasonic sensor to acquire a measurement
		}
		
		if (changed) 	// A flag to check that state has changed and echo has received a value
		{	
				unsigned short integer = distance_of_rangefinder()/10;	// Divide the distance in millimeters by 10 to get the distance as integer in centimeters
				unsigned char decimal = distance_of_rangefinder()%10;	// Mod 10 the distance to get the decimal value

				if(out_of_range == 0)	// If distance is between 1 cm and 400 cm
				{	
					// Function to calculate PWM signal and store it in OCR2A
					// This function is found in TempMainEncoder.c	
					calculate_pwm_signal();	
					PORTC &= ~(1 << PC4);		// Clear PC4 (Turn off RGB Blue segment)

					// Print the distance acquired by the ultrasonic sensor on the LCD 
			 		char buff[17];
					snprintf(buff, 17, " %3d.%d", integer,decimal);
					changed = 0;
					lcd_moveto(0,10);
					lcd_stringout(buff);
					// if the integer part of the distance acquired by the ultrasonic sensor is less than the near threshold. If so, play a note for one second.
					// Function is found in TempMainEncoder.c
					call_buzzer();
				}
				else	// If the distance acquired by the ultrasonic sensor is greater than 400 cm
				{
					// Function to calculate PWM signal and store it in OCR2A
					// This function is found in TempMainEncoder.c	
					calculate_pwm_signal();
					OCR2A = 0;				// Clear OCR2A to turn off the RGB Green segment
					PORTC |= (1 << PC4);	// Set PC4 to 1 (Turn on RGB Blue segment)
					char buff[17];
					snprintf(buff, 17, ">400.0");	// Display >400.0 on the LCD screen
					changed = 0;
					lcd_moveto(0,10);
					lcd_stringout(buff);
				}
		}	
	}		
}

/*
	checkInput() - Checks if PB4 is pressed, and if pressed returns 1 if the button is 
	pressed one time using (debouncing), else returns 0
*/
char checkInput(void)
{
	if ((PINB & (1 << PB4)) == 0)
	{
		while((PINB & (1 << PB4)) == 0){};	// Debouncing
		return (1); 
	}
	else
	{
		return (0);
	}
}

// Initialization of Timer1
void init_timer1(void)
{
	// Set to CTC mode
	TCCR1B |= (1 << WGM12);

	// Enable Timer1 MASK
	TIMSK1 |= (1 << OCIE1A);

	// Load the MAX count
	// Assuming prescaler=8
	OCR1A = MaxTime - 1;	
}

/*
	distance_of_rangefinder() - Calculate the distance measured by the ultrasonic sensor and 
	uses the pulse count to get the distance in millimeters
*/
unsigned long distance_of_rangefinder(void)
{
	temp = ((pulse_count*0.5)*10)/58;	// Calculate distance measured by the range sensor
	
	return temp;
}

// initialization for the rangefinder
void rangefinder_init(void)
{
	//inital for trig
	DDRD |= (1 << PD2);		// Set PD2 as output
	PORTD &= ~(1<< PD2);	// Clear PD2
}

/*
	rangefinder_trigger() - triggers PD2 to send a 10 microseconds signal to the rangefinder
*/
void rangefinder_trigger(void)
{
	PORTD |= (1 << PD2);		// Set PD2 to 1
	_delay_us(trigger_delay);	// Delay for 10 microseconds
	PORTD &= ~(1 << PD2); 		// Clear PD2
}

/*
	Runs if the timer reaches the equivalent of a 400cm measurement. Prevents
	the echo from running forever
*/
ISR(TIMER1_COMPA_vect)
{
	TCCR1B &= ~(1 << CS11); // Stop counter if measurement is greater than 400 cm
	out_of_range = 1; 		// Set out of range flag to 1
	pulse_count = 0;		 
	changed = 1;			// State has changed and echo received a value
}

/*
	Runs when the echo becomes high or low on the relevant pin.
	Sets a flag to tell main that the echo has been received and needs to be
	interpreted
*/
ISR(PCINT2_vect)
{
		echo = (PIND & (1 << PD3)); 	// Set flag to check PD3 for a value
		if((PIND & (1 << PD3)) != 0)  // If we still don't have a value for echo
		{
			TCNT1 = 0;  				// Set timer to zero
			TCCR1B |= (1 << CS11); 		// starts counter
			range_timer_flag = 1; 		// Keeps measuring
			changed = 0; 				// A flag to check if state changed 
		}

		else  // If we get a value for echo
		{
			TCCR1B &= ~(1 << CS11);		// Clear counter 
			range_timer_flag = 0;  		// Stop measuring
			pulse_count = TCNT1; 		// Set timer equal to pulse_count
			changed = 1;			 	// A flag to check that state has changed
		}
}