#include <avr/io.h>
#include <util/delay.h>
/*********************INTRODUCTION*************************/
// doc 19.2
// Serial Peripheral Interface (SPI) = llows high-speed synchronous data transfer between the
// ATmega328P and peripheral devices or between several AVR devices

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
void SPI_MasterTransmit(char cData)
{
	/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)))
	;
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

uint16_t	adc = 0;

void	SPI_lights_off()
{
	start_frame();
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	set_one_led(0b11100000, 0x0, 0x0, 0x0);
	end_frame();
}

int	main()
{
	SPI_MasterInit();
	adc_init();

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
		adc = (ADC);

		if ((adc < (1023 * 0.01)))
			SPI_lights_off();
		else if ((adc > (1023 * 0.01)) && (adc <= (1023 * 0.33)))
		{
			/******LED D6*****/
			start_frame();
			set_one_led(0b11100001, 0xFF, 0x0, 0x0); // LED 6
			set_one_led(0b11100000, 0x0, 0x0, 0x0);
			set_one_led(0b11100000, 0x0, 0x0, 0x0);
			end_frame();
		}
		else if ((adc > (1023 * 0.33)) && (adc <= (1023 * 0.66)))
		{
			/******LED D6*****/
			start_frame();
			set_one_led(0b11100001, 0xFF, 0x0, 0x0); // LED 6
			set_one_led(0b11100001, 0x0, 0xFF, 0x0); // LED 7
			set_one_led(0b11100000, 0x0, 0x0, 0x0);
			end_frame();
		}
		else
		{
			/******LED D6*****/
			start_frame();
			set_one_led(0b11100001, 0xFF, 0x0, 0x0); // LED 6
			set_one_led(0b11100001, 0x0, 0xFF, 0x0); // LED 7
			set_one_led(0b11100001, 0x0, 0x0, 0xFF); // LED 8
			end_frame();
		}
	}
}
