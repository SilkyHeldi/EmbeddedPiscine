// EPROM (Electrically Erasable Programmable Read-Only Memory) = non-volatile memory
// less capacity and slower

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define CHK_BITS(reg, mask) ((reg & mask) == mask)

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

void	lights_off()
{
	PORTB &= ~(1 << PORTB0);
	PORTB &= ~(1 << PORTB1);
	PORTB &= ~(1 << PORTB2);
	PORTB &= ~(1 << PORTB4);
}

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	return EEDR;
}

void debounce_SW1(void)
{
    for (uint32_t i = 0; i < (F_CPU * 6); ++i)
    {}
    while (!(CHK_BITS(PIND, (1<<PD2))))
    {}
    for (uint32_t i = 0; i < (F_CPU * 6); ++i)
    {}
}


int	main()
{
	DDRD &= ~(1 << DDD2); //button SW1 input
	DDRB |= (1 << DDB0);
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	DDRB |= (1 << DDB4);
	uart_init();

	while (1)
	{
		if (!(PIND & (1 << PIND2)))
		{
			lights_off();
			EEPROM_read(42);
			EEDR += 1;
			EEPROM_write(42, EEDR); // writes count value at address 42
			while (!(PIND & (1 << PIND2)));
			debounce_SW1();
		}

		// doc 8.6.2 : EEDR as data register for what is written or read
		EEPROM_read(42);
		if (EEDR == 4)
			EEPROM_write(42, 0);

		switch(EEDR) {
			case 0:
				PORTB |= (1 << PORTB0);
				break;
			case 1:
				PORTB |= (1 << PORTB1);
				break;
			case 2:
				PORTB |= (1 << PORTB2);
				break;
			case 3:
				PORTB |= (1 << PORTB4);
				break;
			default:
				break;
		}
		// if (EEDR == 0)
		// 	PORTB |= (1 << PORTB0);
		// if (EEDR == 1)
		// 	PORTB |= (1 << PORTB1);
		// if (EEDR == 2)
		// 	PORTB |= (1 << PORTB2);
		// if (EEDR == 3)
		// 	PORTB |= (1 << PORTB4);
	}
}