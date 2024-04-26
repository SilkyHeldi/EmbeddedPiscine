#include <avr/io.h>
#include <util/delay.h>

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

uint8_t	numbers[10] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
						0b01101101, 0b01111101, 0b00100111, 0b01111111, 0b01101111};

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

int	main()
{
	adc_init();
	i2c_init();
	i2c_start();

	
	// i2c expander fixed address = 0100
	// A2, A1, A0 are the levers to choose i2c exp address (111 for off)
	// LSB is for read/write
	/****CONFIGURATION***/
	i2c_write(0b01001110); // slave address
	i2c_write(0b00000110); // command byte to choose configuration port 0 (will be the one to receive first byte): 0 means output
	// now pair data bytes
	i2c_write(0b00001111); // data : four first bytes as output
	i2c_write(0b00000000); // data : received by config port 1
	i2c_stop();

	i2c_exp_print_number(0);
	while (1)
	{
		// we want potentiometer which is on ADC0 (ADC_POT) (0000)
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
		i2c_exp_print_number(ADC);
	}


	/******************CAUTION******************/
	// FOR DIGIT SEGMENTS 1 = ON and 0 = OFF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}