#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>
#include <stdlib.h>
// doc 22.6 : TWI is interrupt oriented

#define ACK 1
#define NACK 0

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


void uart_printbin(uint8_t n)
{
    for (uint8_t i = 0; i < 8; ++i)
        uart_tx((n & (1 << (7 - i))) ? '1' : '0');
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
	TWCR = ((1 << TWEN) | (1 << TWINT) );
	// doc 22.7.1 enter Master Transmitter mode by transmitting SLA+W
	// TWCR = ((1<<TWINT) | (1<<TWEN));
	while (!(TWCR & (1 << TWINT)))
	{}
}

void	i2c_readmode(int ack)
{
	// doc 22.7.3 : switch slave to transmitter mode
	// TWAR = 0b01110000; //slave address ATH20 = 0x38 + need write as last bit
	if (ack)
		TWCR = ((1 << TWEN) | (1 << TWINT) | (1 << TWEA));
	else
		TWCR = ((1 << TWEN) | (1 << TWINT ));

	// doc 22.7.2 : Master receiver mode
	// TWCR = ((1 << TWINT) | (1 << TWEN));
	while (!(TWCR & (1 << TWINT)))
	{}
}

void	i2c_read(int ack)
{
	i2c_readmode(ack);
}

void	print_status(char *str)
{
	// 22.7.1 - Table 22-2 Status code table
	uart_printstr(str);
	uart_printstr("\r\n");
	uart_printhex(TWSR);
	uart_printstr("\r\n");
}

float hBytesToFloat(unsigned char byte1, unsigned char byte2, unsigned char byte3) {
    uint32_t data = ((uint32_t)byte1 << 12) | ((uint32_t)byte2 << 4) | ((uint32_t)byte3 >> 4);

	// 24 bits to float
    float result = (float)data;
    return (result);
}

float tBytesToFloat(unsigned char byte1, unsigned char byte2, unsigned char byte3) {
    uint32_t data = (((uint32_t)byte1 << 16) & 0x0FFFFF) | ((uint32_t)byte2 << 8) | ((uint32_t)byte3);

	// 24 bits to float
    float result = (float)data;
    return (result);
}

int	main()
{
	uart_init();

	i2c_init();

	_delay_ms(200);
	i2c_start();
	/****************CALIBRATION CHECK CMD SENT***************/
	i2c_write(0x38 << 1);
	i2c_write(0x71);
	i2c_stop();

	i2c_start();
	/****************RESULT OF CALIBRATION CHECK***************/
	i2c_write((0x38 << 1) | 1);
	i2c_read(NACK);
	uart_printstr("\r\n");
	i2c_stop();
	if (!(TWDR & (1 << 3)))
	{
		i2c_start();
		//initialization cause not calibrated
		i2c_write(0x38 << 1);
		i2c_write(0xbe);
		i2c_write(0x08);
		i2c_write(0x00);
		i2c_stop();
	}
	_delay_ms(10);


	/*******************MEASUREMENT COMPLETION by checking bit 7*****************/
	while (1)
	{
		float	final_hum = 0.0f;
		float	final_temp = 0.0f;
		int	measure = 0;
		uint8_t	byte1;
		uint8_t	byte2;
		uint8_t	byte3;
		uint8_t	byte4;
		uint8_t	byte5;
		while (measure < 3)
		{
			/****************START OF MEASUREMENT***************/
			i2c_start();
			i2c_write(0x38 << 1);
			i2c_write(0xAC);
			i2c_write(0x33);
			i2c_write(0x00);

			i2c_stop();
			_delay_ms(80);

			i2c_start();
			i2c_write((0x38 << 1) | 1);
			i2c_read(ACK); // state

			if (!(TWDR & (1 << 7))) // state says data has been sent
			{
				i2c_read(ACK);
				byte1 = TWDR;
				i2c_read(ACK);
				byte2 = TWDR;
				i2c_read(ACK);
				byte3 = TWDR;
				i2c_read(ACK);
				byte4 = TWDR;
				i2c_read(ACK);
				byte5 = TWDR;

				i2c_read(NACK); // CRC data
			}
			float	tmp_hum = hBytesToFloat(byte1, byte2, byte3);
			float	h = ((tmp_hum / 1048576.0) * 100.0);
			float	tmp_temp = tBytesToFloat(byte3, byte4, byte5);
			float	t = (((tmp_temp / 1048576.0)) * 200.0 - 50.0);

			final_hum += h;
			final_temp += t;
			measure++;
			_delay_ms(200);
		}

		final_hum /= 3; // average value with 3 measurements
		final_temp /= 3;
		char	humidity[10];
		char	temperature[10];
		// resoltion indicated in AHT20 tables
		dtostrf(final_hum, 0, 3, humidity); // smallest increment change of 0.024
		dtostrf(final_temp, 0, 2, temperature); // smallest increment change of 0.01
		uart_printstr("Temperature: ");
		uart_printstr(temperature);
		uart_printstr("°C ");
		uart_printstr("Humidity: ");
		uart_printstr(humidity);
		uart_printstr("%\r\n");
		i2c_stop();
		_delay_ms(1000);
	}

	i2c_stop();
}