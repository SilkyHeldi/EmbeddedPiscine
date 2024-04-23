#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void	lights_off()
{
	PORTD &= ~(1 << PORTD3);
	PORTD &= ~(1 << PORTD5);
	PORTD &= ~(1 << PORTD6);
}

int main()
{
	// doc 16.2.1
	DDRD |= (1 << DDD5); // LED 5 : R : PD5(OC0B/T1)
	DDRD |= (1 << DDD6); // LED 5 : G : PD6(OC0A/AIN0)
	DDRD |= (1 << DDD3); // LED 5 : B : PD3(OC2B/INT1)

	

	/*****************************************/
	/******************MAIN*******************/
	while (1)
	{
		PORTD |= (1 << PORTD5); // R
		_delay_ms(1000);
		lights_off();

		PORTD |= (1 << PORTD6); // G
		_delay_ms(1000);
		lights_off();
	
		PORTD |= (1 << PORTD3); // B
		_delay_ms(1000);
		lights_off();

		PORTD |= (1 << PORTD5);
		PORTD |= (1 << PORTD6);
		_delay_ms(1000);
		lights_off();

		PORTD |= (1 << PORTD6);
		PORTD |= (1 << PORTD3);
		_delay_ms(1000);
		lights_off();

		PORTD |= (1 << PORTD3);
		PORTD |= (1 << PORTD5);
		_delay_ms(1000);
		lights_off();

		PORTD |= (1 << PORTD3);
		PORTD |= (1 << PORTD5);
		PORTD |= (1 << PORTD6);
		_delay_ms(1000);
		lights_off();
	} 
	return (0);
}