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

uint16_t	adc = 0;
int			rgb = 0;
int			LED = 0;

void	set_leds(uint8_t rgb, int led)
{
	int	i = 0;
	start_frame();
	if (rgb == 1) // RED
	{
		while (i < 3)
		{
			if (i == led)
				set_one_led(0b11100001, 0xFF, 0x0, 0x0);
			else
				set_one_led(0b11100000, 0x0, 0x0, 0x0);
			i++;
		}
	}
	else if (rgb == 2) // GREEN
	{
		i = 0;
		while (i < 3)
		{
			if (i == led)
				set_one_led(0b11100001, 0x0, 0xFF, 0x0);
			else
				set_one_led(0b11100000, 0x0, 0x0, 0x0);
			i++;
		}
	}
	else if (rgb == 3) // BLUE
	{
		i = 0;
		while (i < 3)
		{
			if (i == led)
				set_one_led(0b11100001, 0x0, 0x0, 0xFF);
			else
				set_one_led(0b11100000, 0x0, 0x0, 0x0);
			i++;
		}
	}
	end_frame();
}


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
	SPI_lights_off();

	DDRD &= ~(1 << DDD2); // button SW1
	DDRD &= ~(1 << DDD4); // button SW2

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
			rgb = 0;
		else if ((adc > (1023 * 0.01)) && (adc <= (1023 * 0.33)))
			rgb = 1;
		else if ((adc > (1023 * 0.33)) && (adc <= (1023 * 0.66)))
			rgb = 2;
		else
			rgb = 3;
		
		if (!(PIND & (1 << PIND4))) // button SW2 : switch LED
		{
			LED++;
			if (LED == 3)
				LED = 0;
			SPI_lights_off();
			while (!(PIND & (1 << PIND4)))
				_delay_ms(1);
		}

		if (!(PIND & (1 << PIND2))) // button SW1 : validate the value of potentiometer
		{
			switch(rgb) {
				case 0:
					SPI_lights_off();
					break;
				case 1:
					SPI_lights_off();
					set_leds(rgb, LED);
					break;
				case 2:
					set_leds(rgb, LED);
					break;
				case 3:
					set_leds(rgb, LED);
					break;
				default:
					break;
			}
			while (!(PIND & (1 << PIND2)))
				_delay_ms(1);
		}
	}
}
