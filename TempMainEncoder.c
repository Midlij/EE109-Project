
// Include needed library and header files
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "TempMain.h"

// Declaring variables 
volatile unsigned char pwm_signal = 0;	// Variable to store the value of PWM signal that is calculated
volatile int near_count = 200 ;			// Variable to be used in storing "near" value in EEPROM
volatile int far_count = 400; 			// Variable to be used in storing "far" value in EEPROM
volatile char threshold;				// Flag to check which line on the LCD screen should be moved to
volatile unsigned char a, b;			// Variables to determine what state the rotary encoder is in
volatile unsigned char old_state = 0;	// Old state of the rotary encoder
volatile unsigned char new_state = 0;	// New state of the rotary encoder
volatile int encoder_count_update = 0;	// Flag to check if the value on the rotary encoder has changed
volatile short near = 100;				// Variable to count the value of near threshold while the rotary encoder is being rotated
volatile short far = 200;				// Variable to count the value of far threshold while the rotary encoder is being rotated

// Prototypes
unsigned long distance_of_rangefinder(void);
char checkPCInput();

/*
	encoder_main() - function to check if PC3 (threshold button) is pressed, and if so, move to the corresponding 
	threshold, adjust the value of the threshold if needed, and store the new value in EEPROM
*/
void encoder_main(void)
{
		 	checkPCInput();
			char count_lcd[6];
			lcd_moveto(0,5);
			snprintf(count_lcd, 6, "%3d", near);
			lcd_stringout(count_lcd);
			eeprom_update_word((void *) near_count , near);	// Store the new value of "near" in EEPROM
			
			checkPCInput();
			char count_lcd2[6];
			lcd_moveto(1,5);
			snprintf(count_lcd2, 6, "%3d", far);
			lcd_stringout(count_lcd2);
			eeprom_update_word((void *) far_count, far);	// Store the new value of "far" in EEPROM
}

/*
	checkPCInput() - function to check if PC3 is pressed for one time (debouncing), then use threshold variable to 
	switch from line 0 to line 1 or from line 1 to line 0 on the LCD screen. Uses change_lines() function to transition
	between lines
*/
char checkPCInput(void)
{
	if ((PINC & (1 << PC3)) == 0)
	{
		while((PINC & (1 << PC3)) == 0){};
		threshold = ~threshold;		// Invert the threshold 
		change_lines();
	}
	else
	{
		return(0);
	}
}

/*
	store_eeprom() - function to store values of "near" and "far" in EEPROM and retain them 
	whenever the LCD screen is restarted
*/
void store_eeprom(void)
{
	near = eeprom_read_word((void *) near_count);
	far = eeprom_read_word((void *) far_count);
}

/*
	calculate_pwm_signal() - calculate the value of the Pulse Width Modulation signal using the distance acquired
	by the ultrasonic sensor by putting the the distance in a linear equations with two known points
*/
void calculate_pwm_signal(void)
{
 	pwm_signal = (((distance_of_rangefinder()/10)-near)*255)/(far - near);
 	OCR2A = pwm_signal;		// Set the value of PWM signal to OCR2A register
}	

/*
	call_buzzer() - function to check if the integer part of the distance acquired by the ultrasonic sensor is less
	than the near threshold. If so, play a note for one second. Uses play_note() function from buzzer.c file
*/
void call_buzzer(void)
{
	if(distance_of_rangefinder()/10 < near)
	{
		play_note();	// Function to play a note for one second
	}
}

/*
	encoder_init() - initializes encoder ports and determines the state we are currently in
*/
void encoder_init(void)
{
	// Encoder initialization
	PORTC |= ((1 << PC1) | (1 << PC2));
	PORTC |= (1 << PC3);
	PCICR |= (1 << PCIE1);
	PCMSK1 |= ((1 << PCINT10) | (1 << PCINT9));
	
	if (!b && !a)
    {
		old_state = 0;	// State 0
	}
    else if (!b && a)
    {
		old_state = 1;	// state 1
	}
    else if (b && !a)
    {
		old_state = 2;	// State 2
	}
    else
    {
		old_state = 3;	// State 3
	}
    new_state = old_state;	// Set old_state equal to new_state
}

/*
	determine_near_far() - function to read the input bits and determine a and b.
*/
void determine_near_far(void)
{
	unsigned char c = PINC;
	a = (c & (1 << PC1));
	b = (c & (1 << PC2));
}

/*
	change_lines() - function to move from one line on the LCD screen to the other based on
	the threshold we are at on the LCD screen
*/
void change_lines(void)
{
	if(threshold == 0)	// if threshold has value 0 
	{
		lcd_moveto(0,5);	// Move to line 0 on the LCD screen
	}
	if(threshold == 1)	// if threshold has value 1
	{
		lcd_moveto(1,5);	// Move to line 1 on the LCD screen
	}
}

/*
	Run when the encoder has a change of state. Determine new state and
	increment the count as appropriate. Use flag to update counters near and far.
	Limits the values of "near" and "far" to be between 1 and 400 cm, and the value of both 
	thresholds (near and far) can't decrease below 1 cm nor increase above 400 cm
*/
ISR(PCINT1_vect)
{
    // Read the input bits and determine A and B.
	determine_near_far();
	
	// For each state, examine the two input bits to see if state
	// has changed, and if so set "new_state" to the new state,
	// and adjust the count value.
    if (old_state == 0) 
    {
	    // Handle A and B inputs for state 0
	    if(a != 0)
	    {
	    	new_state = 1;
	    	if(threshold == 0)
	    	{
	    		near++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}		
	    	}
	    	else
	    	{
	    		far++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
	    }
	    else if(b != 0)
	    {
	    	new_state = 2;
	    	if(threshold == 0)
	    	{
	    		near--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else
	    	{
	    		far--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
	    }

	}
	else if (old_state == 1) {

	    // Handle A and B inputs for state 1
		if(a == 0)
		{
			new_state = 0;
			if(threshold == 0)
	    	{
	    		near--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else
	    	{
	    		far--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
		else if(b != 0)
		{
			new_state = 3;
			if(threshold == 0)
	    	{
	    		near++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else
	    	{
	    		far++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
	}
	else if (old_state == 2) {

	    // Handle A and B inputs for state 2
		if(a != 0)
		{
			
			new_state = 3;
			if(threshold == 0)
	    	{
	    		near--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else
	    	{
	    		far--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
		else if(b == 0)
		{
			new_state = 0;
			if(threshold == 0)
	    	{
	    		near++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
				near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else
	    	{
	    		far++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
	}
	else 
	{   // old_state = 3
		// Handle A and B inputs for state 3
		if(a == 0)
		{
			new_state = 2;
			if(threshold == 0)
	    	{
	    		near++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}	
	    	}
	    	else
	    	{
	    		far++;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
		else if(b == 0)
		{
			new_state = 1;
			if(threshold == 0)
	    	{
	    		near--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
					near = far - 5;
				}
	    		if (near > 400) 
				{
					near = 400;
				}
				if (near < 1) 
				{
					near = 1;
				}
	    	}
	    	else if(threshold == 1)
	    	{
	    		far--;
	    		// Make "near" and "far" 5 centimeters apart
	    		if(near + 5 >= far)
				{
 					far = near + 5;
				}
	    		if (far > 400) 
				{
					far = 400;
				}
				if (far < 1) 
				{
					far = 1;
				}
	    	}
		}
	}
	// If state changed, update the value of old_state,
	// and set a flag that the state has changed.
	if (new_state != old_state) 
	{
	    encoder_count_update = 1;
	    old_state = new_state;
	}
}

 