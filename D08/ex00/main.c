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

int	main()
{
	SPI_MasterInit();

	int	i = 0;
	while (i < 4) // START FRAME
	{
		SPI_MasterTransmit(0b0);
		i++;
	}
	/*********************LED 6*******************/
	SPI_MasterTransmit(0b11100001); // 3 first neutral (1s) and 5 bits for global brightness
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0xFF);

	SPI_MasterTransmit(0b11100000); // 3 first neutral (1s) and 5 bits for global brightness
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0xFF);

	SPI_MasterTransmit(0b11100000); // 3 first neutral (1s) and 5 bits for global brightness
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0b0);
	SPI_MasterTransmit(0xFF);

	i = 0;
	while (i < 4) // END FRAME
	{
		SPI_MasterTransmit(0b11111111);
		i++;
	}
}
