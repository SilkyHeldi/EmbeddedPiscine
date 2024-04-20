#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#define ACK 1
#define NACK 0
#define BLOCK 1
#define NONBLOCK 0

void	set_binary(unsigned char value, unsigned char nth_binary, int n)
{
	if ((value & nth_binary) == 0)
		PORTB &= ~(1 << n) ;
	else
		PORTB |= (1 << n);
}

void	uart_printstr(char *str);
void	uart_tx(char c);

void uart_printhex(uint8_t byte)
{
	uart_printstr("0x");
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

void	set_timer()
{
	/*****************************************/
	/*****************TIMER1****************/
	// ICR1 = 16000000/ 1024;
	OCR1A = 16000000/ 1024; // 1000ms
	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();

	// counter max value = MAX (= OCR1A fast PWM) 1111
	// doc 16.11.1 - Table 16-4 
	TCCR1A |= (1 << WGM10);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// counter clock select = prescaler = 1024 (101)
	// doc 16.11.1 - Table 16-5
	TCCR1B |= (1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B |= (1 << CS12);

	// doc 15.9.7 + p.623 table
	// set interrupt for timer0
	TIMSK1 |= (1 << TOIE1);
}

int	value = 5;

ISR(TIMER1_OVF_vect)
{
	set_binary(value, (1 << 0), PORTB0);
	set_binary(value, (1 << 1), PORTB1);
	set_binary(value, (1 << 2), PORTB2);
	set_binary(value, (1 << 3), PORTB4);
	value--;
}

// I2C peripheral = J1
int	main()
{
	DDRB |= (1 << DDB0); // LED0 output Data Direction register
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	DDRB |= (1 << DDB4);
	DDRD &= ~(1 << DDD2); //button SW1 input

	uart_init();
	i2c_init();
	int	IPressed = 0;
	int	TheyPressed = 0;
	int	GameStart = 0;


	TWCR = ((1 << TWEN) | (1 << TWEA)); // slave receiver mode by default
	TWAR = ((0x03 << 1) | 0);// set our own address, 0x03 for future uses
	// i2c_write((0x03 << 1) | 1);

	while (1)
	{
		/***************SLAVE ZONE****************/
		if (GameStart == 0)
			i2c_read(NACK, NONBLOCK);

		if (IPressed && TheyPressed && (GameStart == 0))
		{
			GameStart = 1;
			set_timer();
			while (value >= 0)
			{
				// check player pressing
			}
			cli();
		}

		/**********SLAAAVE***********/
		if (TWDR == 0b11 && (GameStart == 0) && !IPressed)
		{
			TheyPressed = 1;
			uart_printstr("Master Pressed !\r\n");
			//start_game;
			while ((PIND & (1 << PIND2)))
			{
			}
			IPressed = 1;
			uart_printstr("I AM THE SLAVE\r\n");
			TWCR |= (1<< TWINT);
			// TWDR = 0x10;
			uart_printstr("SLAVE will send RESPONSE\r\n");
			// while (!(TWCR & (1 << TWINT))) // wait an action from the master
			// {}
			// uart_printstr("MASTER SENT SOMETHING\r\n");
			// TWCR = ((1 << TWEN) | (1 << TWINT));
			uart_printhex(TW_STATUS);
			i2c_write((0x03 << 1) | 0);
			uart_printhex(TW_STATUS);
			i2c_write(0x10);
			uart_printhex(TW_STATUS);
			uart_printstr("SLAVE HAS SENT RESPONSE\r\n");
		}

		/***********MASTEEEEEER***********/
		if (!(PIND & (1 << PIND2)) && (GameStart == 0) && !TheyPressed)
		{
			i2c_start(); // now master
			i2c_write((0x03 << 1) | 0); // to whom am I gonna speak ?
			i2c_write(0b11); // chosen message : I PLAY
			IPressed = 1;
			uart_printstr("I AM THE MASTER\r\n");
			i2c_stop();
			uart_printstr("After MASTER\r\n");
			i2c_start();
			uart_printstr("After MASTER start\r\n");
			// i2c_write((0x03 << 1) | 1); // I WANT TO READ
			// i2c_write((0x03 << 1) | 1); // I WANT TO READ
			// i2c_write((0x03 << 1) | 1); // I WANT TO READ
			//TEEEEEEEEEEEEEEEST
			// while (!(TWCR & (1 << TWINT)))
			// {}
			// TWCR |= (1<< TWEA); // test
			i2c_read(ACK, NONBLOCK);
			uart_printstr("Launched read nonblock\r\n");
			uart_printhex(TW_STATUS);
			while (!(TWDR == 0x10))
			{}

			TheyPressed = 1;
			uart_printstr("Slave Pressed !\r\n");
			i2c_stop();
			TWCR = ((1 << TWEN) | (1 << TWEA)); // slave receiver
		}
		if (GameStart == 1)
		{
			i2c_read(NACK, NONBLOCK);
			if (!(PIND & (1 << PIND2)))
			{
				i2c_start(); // now master
				i2c_write((0x03 << 1) | 0); // to whom am I gonna speak ?
				i2c_write(0b111); // chosen message : I PRESSED TO WIN
				uart_printstr("YOU'RE THE WINNER!!!\r\n");
				//WINNER LIGHTS
				TWCR = ((1 << TWEN) | (1 << TWEA)); // slave receiver
				GameStart = 2;
			}
			if (TWDR == 0b111)
			{
				uart_printstr("YOU'RE THE LOSER!!!\r\n");
				//LOSER LIGHTS
			}
		}
	}
}