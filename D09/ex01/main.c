#include <avr/io.h>
#include <util/delay.h>

#define ACK 1
#define NACK 0
#define BLOCK 1
#define NONBLOCK 0

void	uart_init()
{
	// UART config to 8N1 (8-bit, no parity, stop-bit = 1)
	// doc 20.6 : enable transmitter 0
	// doc 20.7 : enable receiver 0
	UCSR0B |= (1 << TXEN0);
	UCSR0B |= (1 << RXEN0);

	// doc 20.11.4 - Table 20-8 : async mode chosen cause asked for "UART" with no S 00
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


void uart_printbin(uint8_t n)
{
    for (uint8_t i = 0; i < 8; ++i)
        uart_tx((n & (1 << (7 - i))) ? '1' : '0');
}

void	i2c_init()
{

	// doc 14.3.2 : SDA (on PC4) desc, how to enable I2C
	TWCR |= (1 << TWEN);

	//doc 22.9 : set SCL (on PC5) clock frequency in the Master modes
	// doc 22.5.2 : TWBR = ((F_CPU/F_SCL)-16)/(2*prescaler)
	//doc 22.9.3 : TWSR for prescaler, inital value set to 00 so prescaler = 1
	TWSR = 0; // no prescaler
	TWBR = ((F_CPU / 100000)-16) / (2 * 1); //100kH F_SCL frequency
}

void	i2c_start()
{
	// doc 22.6 : how to send START
	// TWINT = 0 = being sent
	// TWEN = enable
	// TWSTA = START
	TWCR = ((1<<TWINT) | (1<<TWEN) | (1<<TWSTA));

	while (!(TWCR & (1 << TWINT))) // wait for the init message to be completly sent
	{}
}

void	i2c_stop()
{
	// doc 22.6 : how to send STOP
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}

void	i2c_write(unsigned char data)
{
	// doc 22.7.3 : switch slave to receiver mode
	// TWAR = 0b01110001; //slave address ATH20 = 0x38 + need read or write as last bit

	// doc 22.6 5. : a specific value must be written to TWCR
	//instructing the TWI hardware to transmit the data packet present in TWDR
	TWDR = data;
	TWCR = ((1 << TWEN) | (1 << TWINT) ); // starts transmission
	// doc 22.7.1 enter Master Transmitter mode by transmitting SLA+W
	// TWCR = ((1<<TWINT) | (1<<TWEN));
	while (!(TWCR & (1 << TWINT)))
	{}
}

void	i2c_read(int ack, int block)
{
	// doc 22.7.3 : switch slave to transmitter mode
	// TWAR = 0b01110000; //slave address ATH20 = 0x38 + need write as last bit
	if (ack)
		TWCR = ((1 << TWEN) | (1 << TWINT) | (1 << TWEA));
	else
		TWCR = ((1 << TWEN) | (1 << TWINT ));

	// doc 22.7.2 : Master receiver mode
	// TWCR = ((1 << TWINT) | (1 << TWEN));
	if (block)
	{
		while (!(TWCR & (1 << TWINT)))
		{}
	}
}


int	main()
{
	i2c_init();
	uart_init();
	uint8_t	c;
	int	count = 0;

	i2c_start();
	// i2c expander fixed address = 0100
	// A2, A1, A0 are the levers to choose i2c exp address (111 for off)
	// LSB is for read/write
	/****I WRITE TO I2C EXPANDER****/
	i2c_write(0b01001110); // slave address
	i2c_write(0b00000110); // command byte to choose configuration port 0
	i2c_write(0b11110001); // data config port 0 : 0 means output (3 LEDS output, IO0_0 as input)
	i2c_write(0b00000000); // data config port 1
	i2c_stop();

	while (1)
	{
		i2c_start();
		i2c_write(0b01001110); // slave address
		i2c_write(0x0); // command byte : we choose port 0 input

		i2c_start();
		i2c_write(0b01001111); // slave address, last bit = 1 for READ
		i2c_read(ACK, BLOCK);
		c = TWDR;
		uart_printstr("TWDR = ");
		uart_printbin(c);
		uart_printstr("\r\n");
		// i2c_stop();

		if (!(TWDR & 1)) // button sw3 pressed if LSB == 0
		{
			count++;
			if (count == 8)
				count = 0;
			c = TWDR;

			i2c_read(ACK, BLOCK);
			while (!(c & 1))
			{
				i2c_read(ACK, BLOCK);
				c = TWDR;
				i2c_read(ACK, BLOCK);
				_delay_ms(1);
			}
			i2c_stop();
			i2c_start();
			i2c_write(0b01001110); // slave address
			i2c_write(0x02); // command byte : we choose port 0 output

			uint8_t	result = (count << 1);
			result = ~result; // invert
			i2c_write(result); // DATA : 1 = OFF and 0 = ON : we choose IO0_3
			i2c_stop();
		}
	}
}