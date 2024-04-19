
// 21.5.1 : important advice about setting the baud rate AFTER transmitter is set

//doc 20.11.2 : TXCn Flag for Transmit Complete interrupt (see description of the TXCIEn bit)
// doc 20.11.4 : settings of USARTn
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile int	count = 0;
volatile int	input_count = 0;
volatile char	*user = "llepiney";
volatile char	*pass = "lol42";
volatile char	usercheck[8];
volatile char	passcheck[5];

volatile int	end = 0;
volatile int	isuser = 1;
volatile int	ko = 0;

int				up = 1;
unsigned int	factor = 0;

// libC AVR function for interrupts
ISR(TIMER0_OVF_vect)
{
	if (up)
	{
		if (factor == 99)
			up = 0;
		factor++;
	}
	else
	{
		if (factor == 1)
			up = 1;
		factor--;
	}
}

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


// libC AVR function for interrupts
ISR(USART_RX_vect)
{
	char	c = UDR0;

	/*******************USER CHECK*********************/
	/**************************************************/

	if (isuser && end == 0)
	{
		if (c != 0x7F && c != '\r')
			input_count++;

		if (input_count != 0 && c == 0x7F) // backspace
		{
			uart_printstr("\b \b");
			if (input_count <= 8)
				usercheck[input_count] = '\0';
			input_count--;
		}

		/***************FIRST ENTER************/
		if (c == '\r')
		{
			uart_printstr("\r\nPassword : ");
			int l = 0;
			while (l < 8)
			{
				if (usercheck[l] == '0' || usercheck[l] == '\0')
					ko = 1;
				l++;
			}
			int	j = 0;
			while (j < 8 && j < input_count)
			{
				usercheck[j] = '\0';
				j++;
			}
			if (input_count > 8)
				ko = 1;
			isuser = 0;
			input_count = 0;
		}

		/****************CHECKING CHARACTER********************/
		else if (input_count <= 8 && c == user[input_count - 1] && c != 0x7F )
			usercheck[input_count - 1] = '1';
		else if (input_count <= 8 && c != user[input_count - 1] && c != 0x7F )
			usercheck[input_count - 1] = '0';
		if (isuser == 1 && c != 0x7F)
			uart_tx(c);
	}

	/*****************PASSWORD CHECK***************/
	/**********************************************/
	else if (isuser == 0 && end == 0)
	{
		if (c != 0x7F && c != '\r')
			input_count++;

		if (input_count != 0 && c == 0x7F) // backspace
		{
			uart_printstr("\b \b");
			if (input_count <= 5)
				passcheck[input_count] = '\0';
			input_count--;
		}
		/********FINAL ENTER********/
		if (c == '\r')
		{
			int l = 0;
			while (l < 5)
			{
				if (passcheck[l] == '0' || passcheck[l] == '\0')
					ko = 1;
				l++;
			}
			int j = 0;
			while (j < 5 && j < input_count)
			{
				passcheck[j] = '\0';
				j++;
			}

			if (input_count > 5)
				ko = 1;

			if (ko == 0)
			{
				uart_printstr("\r\nYES YES YU IS llepiney, I mean me, I mean you but as me\r\n");
				end = 1;
			}
			else
				uart_printstr("\r\nNO NO NO it's RONG\r\n\r\nUsername of yu AGAIN PWEASE: ");

			input_count = 0;
			ko = 0;
			isuser = 1;
		}

		/****************CHECKING CHARACTER********************/
		else if (input_count <= 5 && input_count != 0 && c == pass[input_count - 1] && c != 0x7F)
			passcheck[input_count - 1] = '1';
		else if (input_count <= 5 && c != pass[input_count - 1] && c != 0x7F)
			passcheck[input_count - 1] = '0';
		if (isuser == 0 && end == 0 && c != 0x7F)
			uart_tx('*');
	}

	/*********************ANYTHING ELSE**********************/
	/********************************************************/
	else
		uart_tx(c);
}

int main()
{
	DDRB |= (1 << DDB0); // LED0 output
	DDRB |= (1 << DDB1);
	DDRB |= (1 << DDB2);
	DDRB |= (1 << DDB4);

	// Figure 20-1 : block diagram of transmission
	uart_init();

	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();

	// doc 20.11.3 : RX complete interrupt enable
	UCSR0B |= (1 << RXCIE0);

	// this time we need a frequence for not only the period but also the activity time + interrupt the light
	// doc 16.2.1

	/*****************************************/
	/*****************TIMER1****************/
	ICR1 = 16000000/ 1024;
	OCR1A = ICR1 / 100;
	// global interrupts activated (I-bit of SREG)
	// doc 13.2.1
	sei();

	// counter max value = MAX (= ICR1 fast PWM) 0111
	// doc 16.11.1 - Table 16-4 
	TCCR1A &= ~(1 << WGM10);
	TCCR1A |= (1 << WGM11);
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << WGM13);

	// counter clock select = no pre-scale : 100 so the frequency is really high
	// doc 16.11.1 - Table 16-5
	TCCR1B |= (1 << CS10);
	TCCR1B &= ~(1 << CS11);
	TCCR1B &= ~(1 << CS12);

	// output behaviour = OC1A pin when counter reaches ICR1 (10) + OC1B nothing (00)
	// OC1A = 1 (turns off) when equal to OCR1A
	// OC1A = 0 when back to 0 (turns on)
	// doc 16.11.1 - Table 16-2
	TCCR1A |= (1 << COM1A1);
	TCCR1A &= ~(1 << COM1A0);
	TCCR1A &= ~(1 << COM1B1);
	TCCR1A &= ~(1 << COM1B1);

	/*****************************************/
	/*****************TIMER0*****************/
	OCR0A = ICR1 / 200; // 5 ms

	// MAX value to OCR0A
	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << WGM01);
	TCCR0B |= (1 << WGM02);

	// pre-divider 1024 
	TCCR0B |= (1 << CS00);
	TCCR0B &= ~(1 << CS01);
	TCCR0B |= (1 << CS02);


	uart_printstr("Enter your login PWEASE\r\nUsername of yu : ");
	while (1)
	{
		if (ko == 0 && end == 1)
		{
			// doc 15.9.7 + p.623 table
			// set interrupt for timer0
			if (!(TIMSK0 & (TIMSK0 | (1 << TOIE0))))
				TIMSK0 |= (1 << TOIE0);

			PORTB |= (1 << PORTB0);
			PORTB |= (1 << PORTB4);
			_delay_ms(50);
			PORTB |= (1 << PORTB2);
			_delay_ms(50);
			PORTB &= ~(1 << PORTB2);
			PORTB &= ~(1 << PORTB0);
			PORTB &= ~(1 << PORTB4);
			_delay_ms(100);

		}
		OCR1A = (ICR1 / 100) * factor;
	}
}

// screen /dev/ttyUSB0 115200
//ctrl + a + k to get out  of screen