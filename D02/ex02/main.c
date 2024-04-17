#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char	value = 0b00000000;
volatile int	pressed1 = 1;

void	set_binary(unsigned char value, unsigned char nth_binary, int n)
{
	if ((value & nth_binary) == 0)
		PORTB &= ~(1 << n) ;
	else
		PORTB |= (1 << n);
}

// libC AVR function for interrupts
ISR(INT0_vect)
{
	if (pressed1)
	{
		value++;
		value %= 16;
		pressed1 = 0;
		// we change detection mode to rise to avoid press bouncing
		EICRA |= (1 << ISC01);
		EICRA |= (1 << ISC00);
		set_binary(value, (1 << 0), PORTB0);
		set_binary(value, (1 << 1), PORTB1);
		set_binary(value, (1 << 2), PORTB2);
		set_binary(value, (1 << 3), PORTB4);
	}
	else
	{
		pressed1 = 1;
		// we set back the detection to press instead of release
		EICRA |= (1 << ISC01);
		EICRA &= ~(1 << ISC00);
	}
}

// doc 14.3.3 + schema to know to which interrupt controller SW2 is linked to
// doc 13.2.4 : pin range 23:16 detected for interrupt control
ISR(PCINT2_vect)
{
	if (pressed1)
	{
		value--;
		value %= 16;
		pressed1 = 0;
		EICRA |= (1 << ISC01);
		EICRA |= (1 << ISC00);
		set_binary(value, (1 << 0), PORTB0);
		set_binary(value, (1 << 1), PORTB1);
		set_binary(value, (1 << 2), PORTB2);
		set_binary(value, (1 << 3), PORTB4);
	}
	else
	{
		pressed1 = 1;
		EICRA |= (1 << ISC01);
		EICRA &= ~(1 << ISC00);
	}
}

int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB0); //output
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	DDRB |= (1 << DDB4);
	// DDDX = which pin(bit) for port D
	DDRD &= ~(1 << DDD2); //input
	DDRD &= ~(1 << DDD4); //input
	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();

	/**********************BUTTON 1**************************/
	/********************************************************/
	// activates interrupt INT0 linked to button SW1
	// schema + doc 13.2.2 Bit0
	EIMSK |= (1 << INT0);

	// sets INT0 interrupt detection mode on EICRA (External Interrupt Control Reg A
	// = 10 : falling edge (when button pushed)
	// doc 13.2.1 - Table 13-2
	EICRA |= (1 << ISC01);
	EICRA &= ~(1 << ISC00);

	/**********************BUTTON 2**************************/
	/********************************************************/
	// doc 13.2.6 : PCINT20 on PCMSK2, activates interrupt for button sw2 (PCINT20)
	PCMSK2 |= (1 << PCINT20);
	// doc 13.2.4 : allow PIN[23:16] interrupt
	PCICR |= (1 << PCIE2);

	while (1)
	{
	}
	return (0);
}
