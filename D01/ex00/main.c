#include <avr/io.h>


int main()
{
	// DDRX = Data Direction register, pin input or output ?
	// DDBX = which pin(bit) for port B
	DDRB |= (1 << DDB1);
	while (1)
	{
		// PORTB= PORTX value for pin when output
		PORTB ^=  (1 << PORTB1);
		for(uint32_t i = 0; i < 1350000; i++){}
	}
	return (0);
}