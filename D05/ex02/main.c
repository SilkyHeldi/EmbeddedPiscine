#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdbool.h>

#define MAGIC_NUMBER 0xE1E1

// check ID not already taken + check length < 255 

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

void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address and Data Registers */
	EEAR = uiAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}

unsigned char EEPROM_read(unsigned int uiAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE));
	/* Set up address register */
	EEAR = uiAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from Data Register */
	return EEDR;
}

bool safe_eeprom_read(void *buffer, size_t offset, size_t length)
{
	uart_printstr("REAAAAAD\r\n");
	if ((offset < 6) || (offset > 1018) || (length > 1018)) // impossible address or length
	{
		uart_printstr("Impossible address or length for read\r\n");
		return (false);
	}
	int	i = (int)offset - 6;
	size_t	index = offset - 6;
	uint8_t	mag[2] = {0};
	uint8_t	len[2] = {0};
	int	t = 0;

	while (i >= 0)
	{
		/**********************MAGIC CHECK*********************/
		if (index == (offset - 6)) // FIRST CHECK INDEX FOR MAGIC
		{
			uart_printstr("First magic check\r\n");
			EEPROM_read(index);
			mag[0] = EEDR;
			t = 1;
		}
		EEPROM_read(index + t);
		mag[1] = EEDR; // new potential mag byte put as second byte
		uart_printstr("mag[0] = ");
		uart_printhex(mag[0]);
		uart_printstr("\r\n");
		uart_printstr("mag[1] = ");
		uart_printhex(mag[1]);
		uart_printstr("\r\n");
		uint16_t	mag_check = ((mag[0] << 8) | mag[1]);
		if (mag_check == MAGIC_NUMBER)
		{
			uart_printstr("Magic number found\r\n");
			/*******************LENGTH CHECK*************************/
			if (t == 1)
				index++;
			index++; // now on length bytes
			EEPROM_read(index);
			len[0] = EEDR;
			index++;
			EEPROM_read(index);
			len[1] = EEDR;
			uart_printstr("len[0] = ");
			uart_printhex(len[0]);
			uart_printstr("\r\n");
			uart_printstr("len[1] = ");
			uart_printhex(len[1]);
			uart_printstr("\r\n");
			uint16_t	len_check = ((len[0] << 8) | len[1]);

			size_t	available_length = len_check - offset - (index + 3);
			if (length > available_length)
			{
				uart_printstr("Length is too large to read\r\n");
				return (false);
			}

			size_t	body_count = 0;
			while (body_count < length)
			{
				EEPROM_read(index);
				*((uint8_t*)buffer + body_count) = EEDR;
				uart_printstr("Read once at index :");
				uart_printhex(index);
				uart_printstr(" = ");
				uart_printhex(EEDR);
				uart_printstr("\r\n");
				body_count++;
				index++;
			}
			*((uint8_t*)buffer + body_count) = '\0';
			uart_printstr("Successfully read\r\n");
			return (true);
		}
		else
			mag[0] = mag[1]; // put second mag byte part to check as first byte
		index--;
		i--;
		t = 0;
	}
	uart_printstr("Not my magic number\r\n");
	return (false);
}

bool safe_eeprom_write(void * buffer, size_t offset, size_t length)
{
	uint8_t	mag[2] = {0};
	uint8_t	len[2] = {0};
	uint8_t	*tmp = (uint8_t*)buffer;
	int	t = 0;

	if ((offset < 6) || (offset > 1018) || (length > 1018)) // impossible address or length
	{
		uart_printstr("Impossible address or length for write\r\n");
		return (false);
	}
	
	size_t	index = offset - 6; // magic[2] + length[2] + ID[2]
	int	i = (int)offset - 6;
	while (i >= 0)
	{
		/**********************MAGIC CHECK*********************/
		if (index == (offset - 6)) // FIRST CHECK INDEX FOR MAGIC
		{
			uart_printstr("First magic check\r\n");
			EEPROM_read(index);
			mag[0] = EEDR;
			t = 1;
		}
		EEPROM_read(index + t); // new potential mag byte put as second byte
		mag[1] = EEDR;
		uint16_t	mag_check = ((mag[0] << 8) | mag[1]);

		if (mag_check == MAGIC_NUMBER)
		{
			uart_printstr("Magic number found\r\n");
			if (t == 1)
				index++;
			/*******************LENGTH CHECK*************************/
			index++; // now on length bytes
			EEPROM_read(index);
			len[0] = EEDR;
			index++;
			EEPROM_read(index);
			len[1] = EEDR;
			uart_printstr("len[0] = ");
			uart_printhex(len[0]);
			uart_printstr("\r\n");
			uart_printstr("len[1] = ");
			uart_printhex(len[1]);
			uart_printstr("\r\n");
			uint16_t	len_check = ((len[0] << 8) | len[1]);


			size_t	available_length = len_check - offset - (index + 3);
			if (length > available_length)
			{
				uart_printstr("Length is too large to write\r\n");
				return (false);
			}
			/*************************ID PART***********************************/
			index++; // now on Id byte
			index++;

			/*****************BODY PART******************/
			index++; // now on first body byte
			int	anchor = index;
			size_t	body_count = 0;
			// see if anything identical would be replaced
			while (body_count < length)
			{
				EEPROM_read(index);
				uart_printstr("EEDR read = ");
				uart_printhex(EEDR);
				uart_printstr("\r\n");
				uart_printstr("buffer[i] = ");
				uart_printhex(tmp[body_count]);
				uart_printstr("\r\n");
				if (EEDR != tmp[body_count])
				{
					uart_printstr("Not identical\r\n");
					EEPROM_write(index, tmp[body_count]);
				}
				body_count++;
				index++;
			}
			index = anchor - 6;
			uart_printstr("INDEX NOW = ");
			uart_printhex(index);
			uart_printstr("\r\n");

			/*************SET MAGIC****************/
			uint8_t	new_mag_tab[2];
			new_mag_tab[1] = ((MAGIC_NUMBER & 0xFF00) >> 8);
			EEPROM_write(index, new_mag_tab[1]);
			uart_printstr("INDEX mag[1] = ");
			uart_printhex(index);
			uart_printstr("\r\n");
			index++;
			uart_printstr("INDEX mag[0] = ");
			uart_printhex(index);
			uart_printstr("\r\n");
			new_mag_tab[0] = (MAGIC_NUMBER & 0x00FF);
			EEPROM_write(index, new_mag_tab[0]);
			uart_printstr("new_mag_tab[1] = ");
			uart_printhex(new_mag_tab[1]);
			uart_printstr("\r\n");
			uart_printstr("new_mag_tab[0] = ");
			uart_printhex(new_mag_tab[0]);
			uart_printstr("\r\n");
			index++;

			/*************SET LENGTH****************/
			uint8_t	new_length_tab[2];
			new_length_tab[1] = ((length & 0xFF00) >> 8);
			EEPROM_write(index, new_length_tab[1]);
			index++;
			new_length_tab[0] = (length & 0x00FF);
			EEPROM_write(index, new_length_tab[0]);
			uart_printstr("new_length_tab[1] = ");
			uart_printhex(new_length_tab[1]);
			uart_printstr("\r\n");
			uart_printstr("new_length_tab[0] = ");
			uart_printhex(new_length_tab[0]);
			uart_printstr("\r\n");
			index++;

			/*************SET ID****************/
			uint16_t	new_id = 1;
			uint8_t	new_id_tab[2];
			new_id_tab[1] = ((new_id & 0xFF00) >> 8);
			EEPROM_write(index, new_id_tab[1]);
			index++;
			new_id_tab[0] = (new_id & 0x00FF);
			EEPROM_write(index, new_id_tab[0]);
			uart_printstr("new_id_tab[1] = ");
			uart_printhex(new_id_tab[1]);
			uart_printstr("\r\n");
			uart_printstr("new_id_tab[0] = ");
			uart_printhex(new_id_tab[0]);
			uart_printstr("\r\n");



			uart_printstr("Wrote before but replaced only non identical bytes\r\n");
			return (true);
		}
		else
			mag[0] = mag[1]; // put second mag byte part to check as first byte
		index--;
		i--;
		t = 0;
	}
	/**********************I DID NOT WRITE BEFORE*********************/

	/*************SET MAGIC****************/
	index = 0;
	uint8_t	new_mag_tab[2];
	new_mag_tab[1] = ((MAGIC_NUMBER & 0xFF00) >> 8);
	EEPROM_write(index, new_mag_tab[1]);
	index++;
	new_mag_tab[0] = (MAGIC_NUMBER & 0x00FF);
	EEPROM_write(index, new_mag_tab[0]);

	/*************SET LENGTH****************/
	index++;
	uint8_t	new_length_tab[2];
	new_length_tab[1] = ((length & 0xFF00) >> 8);
	EEPROM_write(index, new_length_tab[1]);
	index++;
	new_length_tab[0] = (length & 0x00FF);
	EEPROM_write(index, new_length_tab[0]);


	/*************SET ID****************/
	index++;
	uint16_t	new_id = 1;
	uint8_t	new_id_tab[2];
	new_id_tab[1] = ((new_id & 0xFF00) >> 8);
	EEPROM_write(index, new_id_tab[1]);
	index++;
	new_id_tab[0] = (new_id & 0x00FF);
	EEPROM_write(index, new_id_tab[0]);
	index++;

	size_t	body_count = 0;
	while (body_count < length)
	{
		EEPROM_write(index, tmp[body_count]);
		body_count++;
		index++;
	}
	uart_printstr("Never wrote before and succesfully wrote\r\n");
	return (true); // no existant data written by me found before
}

int	main()
{
	uart_init();
	char	str[5];
	bool	test = false;
	test = safe_eeprom_write("test", 0x06, 4);
	if (test == true)
		uart_printstr("TROU\r\n");
	else
		uart_printstr("false\r\n");
	test = safe_eeprom_read(&str, 0x06, 4);
	uart_printstr(str);
	if (test == true)
		uart_printstr("TROU\r\n");
	else
		uart_printstr("false\r\n");
}