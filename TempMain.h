extern volatile unsigned long pulse_count;
extern volatile int temp;  // temporary distance
extern volatile unsigned char out_of_range; // distance > 400 cm
extern volatile unsigned char state_change = 1; // Flag if state has changed
extern volatile unsigned char range_flag_on = 0; // Flag to fire rangefinder
extern volatile unsigned char count_update = 1; 

unsigned long distance_of_rangefinder(void);