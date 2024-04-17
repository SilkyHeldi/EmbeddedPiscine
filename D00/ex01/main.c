#include <avr/io.h>
#include <util/delay.h>

int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB0);
		// PORTB= PORTX value for pin when output
		PORTB |= (1 << PORTB0);
	return (0);
}