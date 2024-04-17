#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int	up = 1;
unsigned int	factor = 0;

// libC AVR function for interrupts
ISR(TIMER0_OVF_vect)
{
	if (up)
	{
		if (factor == 99)
			up = 0;
		factor++;
	}
	else
	{
		if (factor == 1)
			up = 1;
		factor--;
	}
}

// doc 15.2.2 Timer0 interrupt desc
// doc 12.4 : interrupt vectors
// doc 15.9 Timer0 settings
// 15.9.7 what to set for timer0 interrupt
int main()
{
	// this time we need a frequence for not only the period but also the activity time + interrupt the light
	// doc 16.2.1
	DDRB |= (1 << DDB1);

	/*****************************************/
	/*****************TIMER1****************/
	ICR1 = 16000000/ 1024;
	OCR1A = ICR1 / 100;
	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();

	// counter max value = MAX (= ICR1 fast PWM) 0111
	// doc 16.11.1 - Table 16-4 
	TCCR1A &= ~(1 << WGM10);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// counter clock select = no pre-scale : 100 so the frequency is really high
	// doc 16.11.1 - Table 16-5
	TCCR1B |= (1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B &= ~(1 << CS12);

	// output behaviour = OC1A pin when counter reaches ICR1 (10) + OC1B nothing (00)
	// OC1A = 1 (turns off) when equal to OCR1A
	// OC1A = 0 when back to 0 (turns on)
	// doc 16.11.1 - Table 16-2
	TCCR1A |= (1 << COM1A1);
	TCCR1A &= ~(1 << COM1A0);
	TCCR1A &= ~(1 << COM1B1);
	TCCR1A &= ~(1 << COM1B1);

	/*****************************************/
	/*****************TIMER0*****************/
	OCR0A = ICR1 / 200; // 5 ms

	// MAX value to OCR0A
	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << WGM02);

	// pre-divider 1024 
	TCCR0B |= (1 << CS00);
	TCCR0B &= ~(1 << CS01);
	TCCR0B |= (1 << CS02);

	// doc 15.9.7 + p.623 table
	// set interrupt for timer0
	TIMSK0 |= (1 << TOIE0);

	/*****************************************/
	/******************MAIN*******************/
	while (1)
	{
		OCR1A = (ICR1 / 100) * factor;
	} 
	return (0);
}