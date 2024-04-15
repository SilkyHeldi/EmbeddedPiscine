#include <avr/io.h>
#include <util/delay.h>

int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB0);

	while (1)
	{
		// PORTB= PORTX value for pin when output
		PORTB |= (1 << PORTB0);
		_delay_ms(1000);
		PORTB &= ~(1 << PORTB0);
		_delay_ms(1000);
	}
	return (0);
}