MCU = atmega328p
F_CPU = 16000000UL
UART_BAUDRATE = 115200
# FORMAT = ihex
TARGET = main
AVRDUDE_PORT = /dev/ttyUSB0
SRC = main.c 
# CFLAGS = -mmcu=$(MCU) -I. $(CFLAGS)
CC = avr-gcc

MSG_COMPILING = "Compiling..."
MSG_CLEANING = "Cleaning..."

all: hex flash

$(TARGET).bin: $(SRC)
	$(CC) $(SRC) -mmcu=$(MCU) -Os -DF_CPU=$(F_CPU) -DUART_BAUDRATE=$(UART_BAUDRATE) -o $@

$(TARGET).hex: $(TARGET).bin
	avr-objcopy -O ihex $< $@

hex: $(TARGET).hex

flash: $(TARGET).hex
	avrdude -p $(MCU) -c arduino -P $(AVRDUDE_PORT) -U flash:w:$<

clean:
	rm -rf $(TARGET).hex $(TARGET).bin

.PHONY : all hex flash clean