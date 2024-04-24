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

void	print_eeprom()
{
	uint16_t	i = 0x00;
	uart_printstr("Line 0 : \r\n");
	while (i < 128)
	{
		EEPROM_read(i);
		if (EEDR == 0xFF)
			uart_printstr("_");
		else
			uart_printhex(EEDR);
		uart_printstr(" ");
		i++;
		if (!(i % 32) && i != 0)
			uart_printstr("\r\n");
	}
}

size_t	first_next_byte(size_t index)
{
	uint8_t	mag[2] = {0};
	int	t = 0;

	size_t	tmp = index;
	while (tmp < 1018)
	{
		if (tmp == index) // first check ever of mag
		{
			EEPROM_read(tmp);
			mag[0] = EEDR;
			t = 1;
		}
		EEPROM_read(index + t);
		mag[1] = EEDR;


		uint16_t	mag_check = ((mag[0] << 8) | mag[1]);
		if (mag_check == MAGIC_NUMBER)
		{
			uart_printstr("Magic number found for first_next_byte\r\n");
			if (t == 1)
				return(tmp);
			else
				return (tmp - 1);
		}
		tmp++;
		t = 0;
	}
	return (tmp); // 1019
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
			// EEPROM_read(index);
			// mag[1] = EEDR;
			t = 1;
		}
		if (t == 1)
		{
			EEPROM_read(index + 1);
			mag[0] = EEDR;
		}
		if (t == 0)
		{
			mag[0] = mag[1];
		}
		EEPROM_read(index);
		mag[1] = EEDR; // new potential mag byte put as second byte

		uart_printstr("mag[1] = ");
		uart_printhex(mag[1]);
		uart_printstr("\r\n");
		uart_printstr("mag[0] = ");
		uart_printhex(mag[0]);
		uart_printstr("\r\n");
		uint16_t	mag_check = ((mag[1] << 8) | mag[0]);
		if (mag_check == MAGIC_NUMBER)
		{
			uart_printstr("Magic number found\r\n");
			/*******************LENGTH CHECK*************************/
			// if (t == 0)
			// 	index++;
			index += 2; // now on length bytes
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

			size_t	taken_length = offset - index - 3;
			size_t	available_length = len_check - taken_length; // available length in already written block

			size_t	last_byte = index + 3 + len_check;
			// begin check
			if (offset > last_byte)
			{
				uart_printstr("Offset out of MY data range\r\n");
				return (false);
			}
			// end of read check
			if (length > available_length)
			{
				uart_printstr("Length of data to read goes after MY data range\r\n");
				return (false);
			}

			index += 3; // go from second length index to first body byte by skipping ID
			// so at data block of already written data

			index = offset;
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

			size_t	taken_length = offset - index - 3;
			size_t	available_length = len_check - taken_length; // available length of already written data by me

			size_t	last_byte = index + 3 + len_check;
			// if data will go above 1023
			if ((offset - 6) > 1018)
			{
				uart_printstr("Start of data goes above the max memory\r\n");
				return (false);
			}
			if ((offset + length) > 1023)
			{
				uart_printstr("Data to write goes above the max memory\r\n");
				return (false);
			}
			if ((offset - 6) > last_byte)
			{
				break; // we can write fresh new data
			}
			// in my range but too large
			if ((offset < last_byte) && (length > available_length))
			{
				uart_printstr("Length is too large to write in MY data range\r\n");
				return (false);
			}
			/*************************ID PART***********************************/
			index++; // now on Id byte
			index++;

			/*****************BODY PART******************/
			index++; // now on first body byte of already written data

			uart_printstr("Offset is in MY range and I have enough space to write\r\n");
			index = offset;
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

	/******************CHECK IF WE CROSS NEXT MAGIC BLOCK*********************/
	size_t	next_byte = first_next_byte(offset);
	if ((offset + length) >= next_byte)
	{
		uart_printstr("Data comes across another of MY data blocks\r\n");
		return (false);
	}

	/*************SET MAGIC****************/
	index = offset - 6;
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
	uart_printstr("Never wrote before and succesfully wrote a fresh nw block and its headers\r\n");
	return (true); // no existant data written by me found before
}

void	clear_eeprom(uint8_t start, uint8_t end)
{
	uint8_t	i = start;
	while (i < end)
	{
		EEPROM_write(i, 0xFF);
		i++;
	}
}

int	main()
{
	uart_init();
	char	str[5];

	clear_eeprom(0, 32);
	// GOOD TESTS AFTER WRITING TOTs
	safe_eeprom_write("test", 0x06, 4);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x06, 4);
	uart_printstr(str);
	uart_printstr("\r\n");
	safe_eeprom_write("aa", 0x08, 2);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x06, 4);
	uart_printstr(str);
	uart_printstr("\r\n");
	safe_eeprom_write("aabbcc", 0x08, 6);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x08, 6);
	uart_printstr(str);
	uart_printstr("\r\n");

	safe_eeprom_write("tot", 0x06, 3);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x06, 4);
	uart_printstr(str);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x06, 3);
	uart_printstr(str);
	uart_printstr("\r\n");
	safe_eeprom_read(&str, 0x06, 2);
	uart_printstr(str);
	uart_printstr("\r\n");
	uart_printstr("\r\n");
}