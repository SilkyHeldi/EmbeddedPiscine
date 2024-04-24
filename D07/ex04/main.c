#include <avr/io.h>
#include <util/delay.h>

void	uart_init()
{
	// UART config to 8N1 (8-bit, no parity, stop-bit = 1)
	// doc 20.6 : enable transmitter 0
	// doc 20.7 : enable receiver 0
	UCSR0B |= (1 << TXEN0);
	UCSR0B |= (1 << RXEN0);

	// doc 20.11.4 - Table 20-8 : async mode chosen caue asked for "UART" with no S 00
	UCSR0C &= ~(1 << UMSEL01);
	UCSR0C &= ~(1 << UMSEL00);

	// doc 20.11.4 - Table 20-9 : parity mode (checks of parity) = none 00
	UCSR0C &= ~(1 << UPM01);
	UCSR0C &= ~(1 << UPM00);

	// doc 20.11.4 - Table 20-10 : stop bit select = one (bit set to 0)
	UCSR0C &= ~(1 << USBS0);

	// doc 20.11.4 - Table 20-11 : character size = 8-bit (011)
	UCSR0C &= ~(1 << UCSZ02);
	UCSR0C |= (1 << UCSZ01);
	UCSR0C |= (1 << UCSZ00);

	// doc 20.11.4 - Table 20-12 : clock plarity for sync mode only, set to 0 for async
	UCSR0C &= ~(1 << UCPOL0);

	// doc 20.3.1 - Table 20-1 : baudrate or UBRRn calculation
	// doc 20.11.5 : USART baud rate set with UBRRnH-L
	// UBRRn = (F_CPU / 8 * BAUD) - 1
	UBRR0L = (float)((F_CPU / (16.0 * UART_BAUDRATE) + 0.5)) - 1;
	UBRR0H = 0;
}

void	 uart_tx(char c)
{
	// doc 20.6.2 example of code
	// doc 20.6.3 : checks when transmit buffer is empty
	while (!(UCSR0A & (1<<UDRE0)))
	{}
	// doc 20.6.1 : sending frames (5 to 8 bits)
	UDR0 = c;
}

void	uart_printstr(char *str)
{
	int	i = 0;
	while (str[i])
	{
		uart_tx(str[i]);
		i++;
	}
}

void uart_printhex(uint8_t byte)
{
	char	c = byte >> 4;
	if(c < 10)
            uart_tx('0' + c);
        else
            uart_tx('A' + c - 10);
	c = byte & 0x0F;
	if(c < 10)
        uart_tx('0' + c);
    else
        uart_tx('A' + c - 10);
	//nibble = 4 bits
}

void	uart_printnumber(uint16_t n)
{
	if (n >= 10)
	{
		uart_printnumber(n / 10);
		uart_printnumber(n % 10);
	}
	else
		uart_tx(n + '0');
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

void	adc_init()
{
	// set Voltage reference to AVcc (01)
	ADMUX &= ~(1 << REFS1);
	ADMUX |= (1 << REFS0);

	// prescaler for F_CPU  = 128 : frequency 125000 = 125kH (111)
	ADCSRA |= (1 << ADPS2);
	ADCSRA |= (1 << ADPS1);
	ADCSRA |= (1 << ADPS0);

	// Enable ADC (ADEN)
	ADCSRA |= (1 << ADEN);

}

// potentiometer : measures difference of potential 
// between two points within a circuit 

void	lights_off()
{
	PORTB &= ~(1 << PB0);
	PORTB &= ~(1 << PB1);
	PORTB &= ~(1 << PB2);
	PORTB &= ~(1 << PB4);

	PORTD &= ~(1 << PD3);
	PORTD &= ~(1 << PD5);
	PORTD &= ~(1 << PD6);
}

int	main()
{
	// doc 16.2.1
	DDRB |= (1 << DDB0);
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	DDRB |= (1 << DDB4);

	DDRD |= (1 << DDD5); // LED 5 : R : PD5(OC0B/T1)
	DDRD |= (1 << DDD6); // LED 5 : G : PD6(OC0A/AIN0)
	DDRD |= (1 << DDD3); // LED 5 : B : PD3(OC2B/INT1)
	uint16_t	adc;
	adc_init();
	uart_init();
	init_rgb();

	while (1)
	{
		// we want NTC which is on ADC0 (ADC_POT) (0000)
		// POTENTIONMETER
		ADMUX &= ~(1 << MUX3);
		ADMUX &= ~(1 << MUX2);
		ADMUX &= ~(1 << MUX1);
		ADMUX &= ~(1 << MUX0);
		// start conversion (measurement)
		// we'll have to wait the end of transmission on ADSC
		ADCSRA |= (1 << ADSC); // set to 1 for next measurement
		while (ADCSRA & (1 << ADSC))
		{}
		adc = (ADC);
	
		if ((adc < (1023 * 0.01)))
		{
			lights_off();
			wheel(adc / 4); // since colours go from 0 to 255 and adc to 1023
		}
		else if ((adc > (1023 * 0.01)) && (adc <= (1023 * 0.25)))
		{
			lights_off();
			PORTB |= (1 << PB0);
			wheel(adc / 4);
		}
		else if ((adc > (1023 * 0.25)) && (adc <= (1023 * 0.50)))
		{
			lights_off();
			wheel(adc / 4);
			PORTB |= (1 << PB0);
			PORTB |= (1 << PB1);
		}
		else if ((adc > (1023 * 0.50)) && (adc <= (1023 * 0.75)))
		{
			lights_off();
			wheel(adc / 4);
			PORTB |= (1 << PB0);
			PORTB |= (1 << PB1);
			PORTB |= (1 << PB2);
		}
		else
		{
			lights_off();
			wheel(adc / 4);
			PORTB |= (1 << PB0);
			PORTB |= (1 << PB1);
			PORTB |= (1 << PB2);
			PORTB |= (1 << PB4);
		}
		_delay_ms(50);
	}
}