#include <avr/io.h>


int main()
{
	// this time we need a frequence for not only the period but also the activity time
	// doc 16.2.1
	DDRB |= (1 << PB1);
	ICR1 = F_CPU / 1024;
	OCR1A = ICR1 / 10;

	// counter max value = MAX (= ICR1 fast PWM) 0111
	// doc 16.11.1 - Table 16-4 
	TCCR1A &= ~(1 << WGM10);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// F_CPU = 16000000UL = 16MHz = 16 millions seconds
	// pre-division sets our number of ticks/sec to 16MHz / 1024 = 15 625 ticks/sec

	// counter clock select = clk/1024 (pre-division) : 101
	// doc 16.11.1 - Table 16-5
	TCCR1B |= (1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B |= (1 << CS12);

	// output behaviour = OC1A pin when counter reaches ICR1 (10) + OC1B nothing (00)
	// OC1A = 1 (turns off) when equal to OCR1A
	// OC1A = 0 when back to 0 (turns on)
	// doc 16.11.1 - Table 16-2
	TCCR1A |= (1 << COM1A1);
	TCCR1A &= ~(1 << COM1A0);
	TCCR1A &= ~(1 << COM1B1);
	TCCR1A &= ~(1 << COM1B1);

	while (1) {} 
	return (0);
}