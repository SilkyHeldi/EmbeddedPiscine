#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

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

uint8_t	numbers[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
						0b01101101, 0b01111101, 0b00100111, 0b01111111, 0b01101111};
uint8_t	final_number[4] = {0};

void	set_seg(uint8_t seg, uint8_t port)
{
	i2c_start();
	i2c_write(0b01001110); // slave address
	i2c_write(0x02); // command byte : we choose port 0 output
	//now pair data bytes
	i2c_write(port); // DATA : 1 = OFF and 0 = ON : we choose IO0_3
	i2c_write(seg); // port 1 output : all segments for digits
	i2c_stop();
}

void	i2c_exp_print_number(int count)
{
	set_seg(0, 0b11111111); // to avoid catching old data, reset buffer
	set_seg(numbers[count / 1000], 0b11101111);
	count %= 1000;

	set_seg(0, 0b11111111); // to avoid catching old data, reset buffer
	set_seg(numbers[count / 100], 0b11011111);
	count %= 100;

	set_seg(0, 0b11111111); // to avoid catching old data, reset buffer
	set_seg(numbers[count / 10], 0b10111111);
	count %= 10;

	set_seg(0, 0b11111111); // to avoid catching old data, reset buffer
	set_seg(numbers[count], 0b01111111);
}

void	i2c_exp_print_number_table(uint8_t *final_number)
{
	set_seg(0, 0b11111111); // to avoid catching old data, reset buffer
	set_seg(numbers[final_number[0]], 0b11101111);

	set_seg(0, 0b11111111);
	set_seg(numbers[final_number[1]], 0b11011111);

	set_seg(0, 0b11111111);
	set_seg(numbers[final_number[2]], 0b10111111);

	set_seg(0, 0b11111111);
	set_seg(numbers[final_number[3]], 0b01111111);
}

unsigned char	binaryToDecimal(unsigned char binary) {
    unsigned char decimal = 0;
    unsigned char power = 1;

    // Convert binary to decimal
    while (binary > 0) {
        decimal += (binary % 10) * power;
        binary /= 10;
        power *= 2;
    }

    return decimal;
}

// read data every second, every access freezes every timer
// read : 0b10100011
// write : 0b10100010

uint8_t	rtc[7]; //seconds|0, minutes|1, hours|2, days|3, weekdays|4, century_months|5, years|6

void	read_rtc()
{
	i2c_start();
	i2c_write(0b10100010); // slave address + WRITE
	i2c_write(0x02); // register address : start at seconds

	i2c_start();
	i2c_write(0b10100011); // slave address + READ

	int	i = 0;
	while (i < 5) // reads 6 times
	{
		i2c_read(ACK, BLOCK);
		rtc[i] = TWDR;
		i++;
	}
	i2c_read(NACK, BLOCK); // no ack from master
	rtc[i] = TWDR;
	i2c_stop();
}

void	display_hour()
{
	uint8_t	tmp[2] = {0};
	tmp[0] = (rtc[2] >> 4); // tens on bits 5 to 4
	tmp[1] = (rtc[2] & 0b00001111); // units on bits 3 to 0

	int	tens = binaryToDecimal(tmp[0]);
	final_number[0] = tens / 10;
	uart_printstr("DEBUG : tens 0 = ");
	uart_printnumber(final_number[0]);
	uart_printstr("\r\n");
	final_number[1] = tens % 10;
	uart_printstr("DEBUG : tens 1 = ");
	uart_printnumber(final_number[1]);
	uart_printstr("\r\n");

	int	units = binaryToDecimal(tmp[1]);
	final_number[2] = units / 10;
	uart_printstr("DEBUG : units 0 = ");
	uart_printnumber(final_number[2]);
	uart_printstr("\r\n");

	final_number[3] = units % 10;
	uart_printstr("DEBUG : units 1 = ");
	uart_printnumber(final_number[3]);
	uart_printstr("\r\n");

	i2c_exp_print_number_table(final_number);
}

void	display_day_month()
{
	uint8_t	tmp[2] = {0};
	/*****DAY****/
	tmp[0] = (rtc[2] >> 4); // tens on bits 5 to 4
	tmp[1] = (rtc[2] & 0b00001111); // units on bits 3 to 0
	uart_printstr("Day/month = ");
	uart_printnumber(binaryToDecimal(tmp[0]));
	uart_printnumber(binaryToDecimal(tmp[1]));
	uart_printstr("/");

	/******MONTH*******/
	tmp[0] = ((rtc[2] >> 4) & 0b0001); // tens on bit 4 (+ we remove century)
	tmp[1] = (rtc[2] & 0b00001111); // units on bits 3 to 0
	uart_printnumber(binaryToDecimal(tmp[0]));
	uart_printnumber(binaryToDecimal(tmp[1]));
	uart_printstr("\r\n");
}

void	display_year()
{
	uint8_t	tmp[2] = {0};
	/*****DAY****/
	tmp[0] = (rtc[2] >> 4); // tens on bits 7 to 4
	tmp[1] = (rtc[2] & 0b00001111); // units on bits 3 to 0
	uart_printstr("Year = ");
	uart_printnumber(binaryToDecimal(tmp[0]));
	uart_printnumber(binaryToDecimal(tmp[1]));
	uart_printstr("\r\n");
}

int	main()
{
	i2c_init();
	uart_init();

	/****************************I2C EXPANDER CONFIG PART*****************************/
	i2c_start();
	// i2c expander fixed address = 0100
	// A2, A1, A0 are the levers to choose i2c exp address (111 for off)
	// LSB is for read/write
	i2c_write(0b01001110); // slave address
	i2c_write(0b00000110); // command byte to choose configuration port 0 (will be the one to receive first byte): 0 means output
	// now pair data bytes
	i2c_write(0b00001111); // data : four first bytes as output
	i2c_write(0b00000000); // data : received by config port 1
	i2c_stop();

	while (1)
	{
		read_rtc();
		display_year();
		display_day_month();
		display_hour();
	}
}