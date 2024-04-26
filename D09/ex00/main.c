#include <avr/io.h>
#include <util/delay.h>

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

	i2c_start();
	// i2c expander fixed address = 0100
	// A2, A1, A0 are the levers to choose i2c exp address (111 for off)
	// LSB is for read/write
	/****I WRITE TO I2C EXPANDER****/
	i2c_write(0b01001110); // slave address
	i2c_write(0b00000110); // command byte to choose configuration port 0 (will be the one to receive first byte): 0 means output
	i2c_write(0b11110111); // data : only bit 3 as output
	i2c_write(0b00000000); // data : received by config port 1
	i2c_stop();

	while (1)
	{
		i2c_start();
		i2c_write(0b01001110); // slave address
		i2c_write(0x02); // command byte : we choose port 0 output
		i2c_write(0b11110111); // DATA : 1 = OFF and 0 = ON : we choose IO0_3
		i2c_stop();
		_delay_ms(500);
		i2c_start();
		i2c_write(0b01001110); // slave address
		i2c_write(0x02); // command byte : we choose port 0 output
		i2c_write(0b11111111); // DATA : 1 = OFF and 0 = ON
		i2c_stop();
		_delay_ms(500);
	}
}