#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile int	pressed = 1;
// libC AVR function for interrupts
ISR(INT0_vect)
{
	if (pressed)
	{
		pressed = 0;
		EICRA |= (1 << ISC01);
		EICRA |= (1 << ISC00);
		PORTB ^=  (1 << PORTB0);
	}
	else
	{
		pressed = 1;
		EICRA |= (1 << ISC01);
		EICRA &= ~(1 << ISC00);
	}
}

int	main()
{
	DDRB |= (1 << PB0); // output
	DDRD &= ~(1 << DDD2); //input

	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();
	// activates interrupt INT0 linked to button SW1
	// schema + doc 13.2.2 Bit0
	EIMSK |= (1 << INT0);

	// sets INT0 interrupt detection mode on EICRA (External Interrupt Control Reg A
	// = 10 : falling edge (when button pushed)
	// doc 13.2.1 - Table 13-2
	EICRA |= (1 << ISC01);
	EICRA &= ~(1 << ISC00);
	while (1)
	{}
}