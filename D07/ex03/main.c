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

void	adc_init()
{
	// doc 24.8 : about temperature sensor
	// set Voltage reference to Internal 1.1V with external capacitor (11)
	ADMUX |= (1 << REFS1);
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

int	main()
{
	uint16_t	adc;
	adc_init();
	// choose the wanted pin with 
	// input channel selection table for the last 4 bits of ADMUX

	uart_init();

	while (1)
	{
		// we want NTC which is on ADC8 (ADC_) (1000)
		// INTERNAL TEMPERATURE
		ADMUX |= (1 << MUX3);
		ADMUX &= ~(1 << MUX2);
		ADMUX &= ~(1 << MUX1);
		ADMUX &= ~(1 << MUX0);
		// start conversion (measurement)
		// we'll have to wait the end of transmission on ADSC
		ADCSRA |= (1 << ADSC); // set to 1 for next measurement
		while (ADCSRA & (1 << ADSC))
		{}
		adc = ((ADC * 25) / 314);
		uart_printnumber(adc);
		uart_printstr("\r\n");
		_delay_ms(20);
	}
}