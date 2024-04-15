#include <avr/io.h>
#include <util/delay.h>

void	set_binary(unsigned char value, unsigned char nth_binary, int n)
{
	// miss case 0
	if ((value & nth_binary) == 0)
		PORTB &= ~(1 << n) ;
	else
		PORTB |= (1 << n);
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
	unsigned char	value = 0b00000000;

	while (1)
	{
			if (!(PIND & (1 << PIND2)))
			{
				value++;
				value %= 16;
				set_binary(value, (1 << 0), PORTB0);
				set_binary(value, (1 << 1), PORTB1);
				set_binary(value, (1 << 2), PORTB2);
				set_binary(value, (1 << 4), PORTB4);
				while (!(PIND & (1 << PIND2)))
					_delay_ms(50); //avoid bounce effects

				_delay_ms(800); //avoid bounce effects
			}
	}
	return (0);
}