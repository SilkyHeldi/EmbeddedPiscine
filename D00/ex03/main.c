#include <avr/io.h>
#include <util/delay.h>

int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB0); //output
	// DDDX = which pin(bit) for port D
	DDRD &= ~(1 << DDD2); //input
	int	state = 0;

	while (1)
	{
		while (state == 0) // currently off
		{
			while (!(PIND & (1 << PIND2)))
			{
				state = 1;
				PORTB |= (1 << PORTB0); // PORTB = PORTX value for pin when output
				_delay_ms(50); //avoid bounce effects
			}
		}
		while (state == 1) // currently on
		{
			while (!(PIND & (1 << PIND2)))
			{
				state = 0;
				PORTB &= ~(1 << PORTB0);
				_delay_ms(50);
			}
		}
	}
	return (0);
}