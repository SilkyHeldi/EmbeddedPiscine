#include <avr/io.h>
#include <util/delay.h>

int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB0);
	DDRD &= ~(1 << DDD2);

	while (1)
	{
		if (PIND & (1 << PIND2))
		{
			// PORTB= PORTX value for pin when output
			PORTB &= ~(1 << PORTB0);
			_delay_ms(1000);
		}
		else
		{
			PORTB |= (1 << PORTB0);
			_delay_ms(1000);

		}
	}
	return (0);
}