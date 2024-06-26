#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void	lights_off()
{
	PORTD &= ~(1 << PORTD3);
	PORTD &= ~(1 << PORTD5);
	PORTD &= ~(1 << PORTD6);
}

void	init_rgb()
{
	/*****************************************/
	/*****************TIMER0*****************/
	// OCR0A = 0;
	// OCR0B = 0; // 1000ms / 64 = 2000ms / 128
	// OCR2B = 0; // 1000ms ?

	// MAX value to 0xFF, fast PWM (011)
	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << WGM01);
	TCCR0A &= ~(1 << WGM02);

	// no prescaler
	TCCR0B |= (1 << CS00);
	TCCR0B &= ~(1 << CS01);
	TCCR0B &= ~(1 << CS02);

	//doc 15.9.1 
	TCCR0A |= (1 << COM0A1);
	TCCR0A &= ~(1 << COM0A0);

	// same mode 10
	TCCR0A |= (1 << COM0B1);
	TCCR0A &= ~(1 << COM0B0);

	OCR0A = 0;
	OCR0B = 0;

	// doc 15.9.7 + p.623 table
	// set interrupt for timer0
	// TIMSK0 |= (1 << TOIE0);

	/*****************************************/
	/*****************TIMER2*****************/
	// doc 18.11.1 Table 18-8 : MAX value to OCR2A + PWM, Phase Correct (001) (good for symmetric rising and falling edges)
	// the timer counts up to a defined value (usually a maximum value like 255 for an 8-bit timer) and then counts back down to zero.
	// different from fast PWM
	// Fast PWM does not count up and down. Instead, it counts only in one direction, from 0 to the maximum value, and then resets.
	// This mode results in a PWM waveform with a higher frequency compared to Phase Correct PWM, as the entire waveform (both rising and falling edges) is generated in one cycle of the timer.
	TCCR2A |= (1 << WGM20);
	TCCR2A &= ~(1 << WGM21);
	TCCR2B &= ~(1 << WGM22);

	// pre-scaler = 8 (010)
	TCCR2B &= ~(1 << CS20);
	TCCR2B |= (1 << CS21);
	TCCR2B &= ~(1 << CS22);

	// option (10)
	// Clear OC2B on Compare Match when up-counting. Set OC2B on Compare Match when down-counting.
	TCCR2A |= (1 << COM2B1);
	TCCR2A &= ~(1 << COM2B0);

	OCR2B = 0;

	// set interrupt for timer2
	// TIMSK2 |= (1 << TOIE2);
}

void	set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	OCR0A = g;
	OCR0B = r;
	OCR2B = b;
}

void wheel(uint8_t pos)
{
	pos = 255 - pos;
	if (pos < 85)
		set_rgb(255 - pos * 3, 0, pos * 3);
	else if (pos < 170)
	{
		pos = pos - 85;
		set_rgb(0, pos * 3, 255 - pos * 3);
	}
	else
	{
		pos = pos - 170;
		set_rgb(pos * 3, 255 - pos * 3, 0);
	}
}

int main()
{
	// doc 16.2.1
	DDRD |= (1 << DDD5); // LED 5 : R : PD5(OC0B/T1)
	DDRD |= (1 << DDD6); // LED 5 : G : PD6(OC0A/AIN0)
	DDRD |= (1 << DDD3); // LED 5 : B : PD3(OC2B/INT1)

	init_rgb();
	int	counter = 0;

	/*****************************************/
	/******************MAIN*******************/
	while (1)
	{
		counter++;
		if (counter == 255)
			counter = 0;

		wheel(counter);
		_delay_ms(50);
	}
	return (0);
}