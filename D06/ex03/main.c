#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>

char	uart_rx()
{
	// doc 20.7.1 : how to receive (5 to 8 bits)
	// RXCn is the Receive Complete Flag
	while (!(UCSR0A & (1<<RXC0)))
	{}
	return (UDR0);
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

	// MAX value to OCR0A
	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << WGM01);
	// s

	// pre-divider 1024 
	TCCR0B |= (1 << CS00);
	TCCR0B &= ~(1 << CS01);
	// TCCR0B |= (1 << CS02);

	//doc 15.9.1 Compare Output Mode, Fast PWM Mode,  (10)
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
	// MAX value to OCR2A (111)
	TCCR2A |= (1 << WGM20);
	// TCCR2A |= (1 << WGM21);
	// TCCR2B |= (1 << WGM22);

	// pre-divider 1024 (111)
	// TCCR2B |= (1 << CS20);
	TCCR2B |= (1 << CS21);
	// TCCR2B |= (1 << CS22);

	// option (10)
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

uint8_t uint8_to_hex(char high, char low)
{
    uint8_t result = 0;

    // Convert the high character to its corresponding value
    if (high >= '0' && high <= '9') {
        result = (high - '0') << 4; // Shift left by 4 bits
    } else if (high >= 'A' && high <= 'F') {
        result = (high - 'A' + 10) << 4; // Shift left by 4 bits
    } else if (high >= 'a' && high <= 'f') {
        result = (high - 'a' + 10) << 4; // Shift left by 4 bits
    }

    // Convert the low character to its corresponding value
    if (low >= '0' && low <= '9') {
        result |= (low - '0'); // Bitwise OR operation
    } else if (low >= 'A' && low <= 'F') {
        result |= (low - 'A' + 10); // Bitwise OR operation
    } else if (low >= 'a' && low <= 'f') {
        result |= (low - 'a' + 10); // Bitwise OR operation
    }
    return (result);
}

bool	is_hexa(char c)
{
	if (c >= 'A' && c <= 'F')
		return (true);
	if (c >= '0' && c <= '9')
		return (true);
	return (false);
}

bool	check_rgb(unsigned char *command)
{
	int	i = 1;
	while (i < 7)
	{
		if (is_hexa(command[i]) == false)
			return (false);
		i++;
	}
	return (true);
}

unsigned char	command[7];
int	input_count = 0;

// libC AVR function for interrupts
ISR(USART_RX_vect)
{
	char	c = UDR0;
	input_count++;
	uart_tx(c);
	if (input_count <= 8)
		command[input_count - 1] = c;
	if (c == '\r') // newline detected
	{
		if (input_count <= 8 && (command[0] == '#') && (check_rgb(command) == true))
		{
			uint16_t	R = uint8_to_hex(command[1], command[2]);
			uint16_t	G = uint8_to_hex(command[3], command[4]);
			uint16_t	B = uint8_to_hex(command[5], command[6]);
			set_rgb(R, G, B);
			uart_printstr("\nSuccessfully set new colour\r\n");
		}
		else
			uart_printstr("\r\nWrong input, try this format : #RRGGBB\r\n");
		/***************RESET BUFFER**************/
		int	i = 0;
		while (i < 7)
		{
			command [i] = '\0';
			i++;
		}
		input_count = 0;
	}
}

int main()
{
	// doc 16.2.1
	DDRD |= (1 << DDD5); // LED 5 : R : PD5(OC0B/T1)
	DDRD |= (1 << DDD6); // LED 5 : G : PD6(OC0A/AIN0)
	DDRD |= (1 << DDD3); // LED 5 : B : PD3(OC2B/INT1)

	init_rgb();
	uart_init();
	sei();
	// doc 20.11.3 : RX complete interrupt enable
	UCSR0B |= (1 << RXCIE0);


	/*****************************************/
	/******************MAIN*******************/
	while (1)
	{
	} 
	return (0);
}