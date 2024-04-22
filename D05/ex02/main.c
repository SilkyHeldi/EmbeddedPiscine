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
	if ((offset < 6) || (offset > 1018) || (length > 1018)) // impossible address or length
		return (false);
	int	i = (int)offset - 6;
	size_t	index = offset - 6;
	uint8_t	mag[2] = {0};
	uint8_t	len[2] = {0};

	while (i)
	{
		/**********************MAGIC CHECK*********************/
		if (index == (offset - 6)) // FIRST CHECK INDEX FOR MAGIC
		{
			EEPROM_read(index);
			mag[0] = EEDR;
			index++;
		}
		EEPROM_read(index);
		mag[1] = EEDR; // new potential mag byte put as second byte
		uint16_t	mag_check = ((mag[0] << 8) | mag[1]);
		if (mag_check == MAGIC_NUMBER)
		{
			/*******************LENGTH CHECK*************************/
			index++; // now on length bytes
			EEPROM_read(index);
			len[0] = EEDR;
			index++;
			EEPROM_read(index);
			len[1] = EEDR;
			uint16_t	len_check = ((len[0] << 8) | len[1]);

			size_t	available_length = len_check - offset - (index + 3);
			if (length > available_length)
				return (false);

			size_t	body_count = 0;
			while (body_count < length)
			{
				EEPROM_read(index);
				*((uint8_t*)buffer + body_count) = EEDR;
				body_count++;
				index++;
			}
			*((uint8_t*)buffer + body_count) = '\0';
			return (true);
		}
		else
			mag[0] = mag[1]; // put second mag byte part to check as first byte
		index--;
		i--;
	}
	return (false);
}

bool safe_eeprom_write(void * buffer, size_t offset, size_t length)
{
	uint8_t	mag[2] = {0};
	uint8_t	len[2] = {0};
	uint8_t	*tmp = (uint8_t*)buffer;

	if ((offset < 6) || (offset > 1018) || (length > 1018)) // impossible address or length
		return (false);
	
	size_t	index = offset - 6; // magic[2] + length[2] + ID[2]
	int	i = (int)offset - 6;
	while (i)
	{
		/**********************MAGIC CHECK*********************/
		if (index == (offset - 6)) // FIRST CHECK INDEX FOR MAGIC
		{
			EEPROM_read(index);
			mag[0] = EEDR;
			index++;
		}
		mag[1] = EEPROM_read(index); // new potential mag byte put as second byte
		uint16_t	mag_check = ((mag[0] << 8) | mag[1]);

		if (mag_check == MAGIC_NUMBER)
		{
			/*******************LENGTH CHECK*************************/
			index++; // now on length bytes
			EEPROM_read(index);
			len[0] = EEDR;
			index++;
			EEPROM_read(index);
			len[1] = EEDR;
			uint16_t	len_check = ((len[0] << 8) | len[1]);


			size_t	available_length = len_check - offset - (index + 3);
			if (length > available_length)
				return (false);
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
				if (EEDR != tmp[body_count])
					EEPROM_write(index, tmp[body_count]);
				body_count++;
				index++;
			}
			index = anchor;

			/*************SET ID****************/
			uint16_t	new_id = 1;
			uint8_t	new_id_tab[2];
			new_id_tab[1] = ((new_id & 0xFF00) >> 8);
			EEPROM_write(index, new_id_tab[1]);
			index++;
			new_id_tab[0] = (new_id & 0x00FF);
			EEPROM_write(index, new_id_tab[0]);
			index++;

			/*************SET LENGTH****************/
			uint8_t	new_length_tab[2];
			new_length_tab[1] = ((length & 0xFF00) >> 8);
			EEPROM_write(index, new_length_tab[1]);
			index++;
			new_length_tab[0] = (length & 0x00FF);
			EEPROM_write(index, new_length_tab[0]);

			/*************SET MAGIC****************/
			index++;
			uint8_t	new_mag_tab[2];
			new_mag_tab[1] = ((MAGIC_NUMBER & 0xFF00) >> 8);
			EEPROM_write(index, new_mag_tab[1]);
			index++;
			new_mag_tab[0] = (MAGIC_NUMBER & 0x00FF);
			EEPROM_write(index, new_mag_tab[0]);

			return (true);
		}
		else
			mag[0] = mag[1]; // put second mag byte part to check as first byte
		index--;
		i--;
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
	return (true); // no existant data written by me found before
}

int	main()
{
	uart_init();
	char	str[5];
	bool	test = false;
	test = safe_eeprom_write("test", 0x05, 4);
	if (test == true)
		uart_printstr("TROU\r\n");
	else
		uart_printstr("false\r\n");
	test = safe_eeprom_read(str, 0x05, 4);
	if (test == true)
		uart_printstr("TROU\r\n");
	else
		uart_printstr("false\r\n");
}