#include <avr/io.h>
#include <util/delay.h>
/*********************INTRODUCTION*************************/
// doc 19.2
// Serial Peripheral Interface (SPI) = llows high-speed synchronous data transfer between the
// ATmega328P and peripheral devices or between several AVR devices

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

void SPI_SlaveInit(uint8_t ss_pin)
{
    // Configurer la broche SS comme entrée
    DDRB &= ~(1 << ss_pin);
    // Activer le SPI en mode esclave
    SPCR |= (1 << SPE);
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

void	set_led6(uint8_t l, uint8_t r, uint8_t g, uint8_t b)
{
	/*********************LEDS*******************/
	set_one_led(l, r, g, b);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
}


int	main()
{
	SPI_MasterInit();

	while (1)
	{
		/******LED D6*****/
		start_frame();
		set_one_led(0b11100001, 0xFF, 0x0, 0x0); // LED 6
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		end_frame();
		_delay_ms(250);

		/******LED D7*****/
		start_frame();
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100001, 0xFF, 0x00, 0x0); // LED 7
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		end_frame();
		_delay_ms(250);

		/******LED D8*****/
		start_frame();
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100001, 0xFF, 0x0, 0x0); // LED 8
		end_frame();
		_delay_ms(250);

		/******ALL OFF*****/
		start_frame();
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		set_one_led(0b11100000, 0x0, 0x0, 0x0);
		end_frame();
		_delay_ms(250);
	}
}
