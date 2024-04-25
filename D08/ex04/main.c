#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
/*********************INTRODUCTION*************************/
// doc 19.2
// Serial Peripheral Interface (SPI) = llows high-speed synchronous data transfer between the
// ATmega328P and peripheral devices or between several AVR devices

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

void SPI_MasterInit(void)
{
/* Set MOSI and SCK output, all others input */
	// doc 14.3.1 (Alternate Functions of Port B)
	// DDB3 instead of PB3 which controls the pull-up for MOSI
	// DDB2 = SS pin
	DDRB = (1 << DDB3) | (1 << DDB2) |(1 << DDB5);
	/* Enable SPI, Master, set clock rate fck/16 */
	// SPE = enable SPI
	// MSTR = set as master
	// SPR0 = set clock rate
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);
}
void SPI_MasterTransmit(char cData)
{
	/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)))
	;
}

uint8_t SPI_Receive(void)
{
    while (!(SPSR & (1<<SPIF)));
    return SPDR;
}

void	set_one_led(uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
	SPI_MasterTransmit(l); // 3 first neutral (1s) and 5 bits for global brightness
	SPI_MasterTransmit(b);
	SPI_MasterTransmit(g);
	SPI_MasterTransmit(r);
}

void	start_frame()
{
	int	i = 0;
	while (i < 4) // START FRAME
	{
		SPI_MasterTransmit(0b0);
		i++;
	}
}

void	end_frame()
{
	int	i = 0;
	while (i < 4) // END FRAME
	{
		SPI_MasterTransmit(0b11111111);
		i++;
	}
}

void	SPI_lights_off()
{
	start_frame();
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	end_frame();
}

uint8_t	command[12]; // for rainbow, but 9 for rgb
int	input_count = 0;
uint8_t	led6_r = 0;
uint8_t	led6_g = 0;
uint8_t	led6_b = 0;

uint8_t	led7_r = 0;
uint8_t	led7_g = 0;
uint8_t	led7_b = 0;

uint8_t	led8_r = 0;
uint8_t	led8_g = 0;
uint8_t	led8_b = 0;
int		rainbow = 0;
int		counter = 0;

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

void	set_led(uint8_t led_num, uint8_t r, uint8_t g, uint8_t b)
{
	if (led_num == '6')
	{
		led6_r = r;
		led6_g = g;
		led6_b = b;
	}
	else if (led_num == '7')
	{
		led7_r = r;
		led7_g = g;
		led7_b = b;
	}
	else if (led_num == '8')
	{
		led8_r = r;
		led8_g = g;
		led8_b = b;
	}
	start_frame();
	set_one_led(0b11100001, led6_r, led6_g, led6_b);
	set_one_led(0b11100001, led7_r, led7_g, led7_b);
	set_one_led(0b11100001, led8_r, led8_g, led8_b);
	end_frame();
	rainbow = 0;
}

bool	rainbow_cmp(uint8_t *src, uint8_t *dst)
{
	int	i = 0;
	while ((src[i] || dst[i]) && (i <= 11))
	{
		if (src[i] != dst[i])
			return (false);
		i++;
	}
	return (true);
}

void wheel(uint8_t pos)
{
	pos = 255 - pos;
	if (pos < 85)
	{
		start_frame();
		set_one_led(0b11100001, 255 - pos * 3, 0, pos * 3);
		set_one_led(0b11100001, 255 - pos * 3, 0, pos * 3);
		set_one_led(0b11100001, 255 - pos * 3, 0, pos * 3);
		end_frame();
	}
	else if (pos < 170)
	{
		pos = pos - 85;
		start_frame();
		set_one_led(0b11100001, 0, pos * 3, 255 - pos * 3);
		set_one_led(0b11100001, 0, pos * 3, 255 - pos * 3);
		set_one_led(0b11100001, 0, pos * 3, 255 - pos * 3);
		end_frame();
	}
	else
	{
		pos = pos - 170;
		start_frame();
		set_one_led(0b11100001, pos * 3, 255 - pos * 3, 0);
		set_one_led(0b11100001, pos * 3, 255 - pos * 3, 0);
		set_one_led(0b11100001, pos * 3, 255 - pos * 3, 0);
		end_frame();
	}
}

// libC AVR function for interrupts
ISR(USART_RX_vect)
{
	char	c = UDR0;
	input_count++;
	uart_tx(c);
	if (input_count <= 12)
		command[input_count - 1] = c;
	if (c == '\r') // newline detected
	{
		if (input_count == 13) // POTENTIAL RAINBOW
		{
			uint8_t	rainbow_tab[] = "#FULLRAINBOW\r";
			if (rainbow_cmp(rainbow_tab, command) == true)
			{
				rainbow = 1;
				uart_printstr("\nSuccessfully set FULL RAINBOWWWWWWWWW\r\n");
			}
			else
				uart_printstr("\r\nWrong input, try this format : #RRGGBBDX or type #FULLRAINBOW\r\n");
		}
		/***************************CHECK RGB*******************************/
		else if (input_count == 10) // POTENTIAL RGB SET
		{
			if ((command[0] == '#') && (check_rgb(command) == true) && (command[7] == 'D')
				&& (command[8] >= '6') && (command[8] <= '8') && (command[9] == '\r'))
			{
				uint8_t	R = uint8_to_hex(command[1], command[2]);
				uint8_t	G = uint8_to_hex(command[3], command[4]);
				uint8_t	B = uint8_to_hex(command[5], command[6]);
				uint8_t		LED = command[8];
				set_led(LED, R, G, B);
				uart_printstr("\nSuccessfully set new colour\r\n");
			}
			else
				uart_printstr("\r\nWrong input, try this format : #RRGGBBDX or type #FULLRAINBOW\r\n");
		}
		else
			uart_printstr("\r\nWrong input, try this format : #RRGGBBDX or type #FULLRAINBOW\r\n");
		/***************RESET BUFFER**************/
		int	i = 0;
		while (i < 12)
		{
			command [i] = '\0';
			i++;
		}
		input_count = 0;
	}
}

int	main()
{
	SPI_MasterInit();
	adc_init();
	uart_init();
	sei();
	// doc 20.11.3 : RX complete interrupt enable
	UCSR0B |= (1 << RXCIE0);
	SPI_lights_off();

	while (1)
	{
		counter++;
		if (counter == 255)
			counter = 0;
		
		if (rainbow)
			wheel(counter);
		_delay_ms(50);
	}
}
