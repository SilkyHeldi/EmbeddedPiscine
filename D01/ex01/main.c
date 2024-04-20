#include <avr/io.h>


int main()
{
	DDRB |= (1 << PB1);
	OCR1A = 7812;

	// counter max value = MAX (= OCR1A fast PWM) 1111
	// doc 16.11.1 - Table 16-4 
	TCCR1A |= (1 << WGM10);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// F_CPU = 16000000UL = 16MHz = period of 16 millions seconds
	// pre-division sets our number of ticks/sec to 16MHz / 1024 = 15 625 ticks/sec
	// if we want to activate the LED every 0.5 sec wee need 15 625/2 =~ 7812 ticks for OCR1A

	// counter clock select = clk/1024 (pre-division) : 101
	// doc 16.11.1 - Table 16-5
	TCCR1B |= (1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B |= (1 << CS12);

	// output behaviour = OC1A pin when counter reaches OCR1A (01) + OC1B nothing (00)
	// doc 16.11.1 - Table 16-2
	TCCR1A &= ~(1 << COM1A1);
	TCCR1A |= (1 << COM1A0);
	TCCR1A &= ~(1 << COM1B1);
	TCCR1A &= ~(1 << COM1B1);

	while (1) {} 
	return (0);
}