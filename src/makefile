# ----------- settings ---------------------------------------------------------
# -- part in avr-gcc
PART=atmega328p
# -- part in avrdude
PART_PROG=m328p
# -- avrdude port
PORT=/dev/ttyACM0
# -- avrdude programmer type
PROG=arduino
# -- bootloader section start adress (byte adress in flash memory) - datasheet gives word adress (byte_adr = 2*word_adr)
# for atmega328p there are: 0x7E00 (256 words) 0x7C00 (512 words) 0x7800 (1024 words) 0x7000 (2048 words)
BOOT_ADR=0x00007E00

# -- dependencies (libraries - includes)
DEPS=lib/libad9850.o lib/libmorse.o lib/libov7670.o
# -- directory to libraries (includes)
LIBDIR=lib/
# -- name of the project main file
PROJ=main

# -- compilers and compiler options
CC=avr-gcc
CFLAGS= -Wall -g -mmcu=$(PART)

# ----------- targets ----------------------------------------------------------
# -- build project and display device memory usage
default: $(PROJ).elf
	avr-size -C --mcu=$(PART) $< || avr-size $<

# -- programm device flash
flash: default
	avr-objcopy -j .text -j .data -O ihex $(PROJ).elf $(PROJ).hex
	avrdude -p $(PART_PROG) -P $(PORT) -c $(PROG) -U flash:w:$(PROJ).hex:i

# -- programm device eeprom
eeprom: default
	avr-objcopy -j .eeprom --change-section-lma .eeprom=0 -O ihex $(PROJ).elf $(PROJ)_eeprom.hex
	avrdude -p $(PART_PROG) -P $(PORT) -c $(PROG) -U eeprom:w:$(PROJ)_eeprom.hex:i

# -- programm device bootloader section - caution: will erase the chip, bootloader is unlocked after operation - needs to be locked manually
flash-bl: default
	avr-objcopy -j .bootloader --change-section-lma .bootloader=$(BOOT_ADR) -O ihex $(PROJ).elf $(PROJ)_boot.hex
	avrdude -p $(PART_PROG) -P $(PORT) -c $(PROG) -e
	avrdude -p $(PART_PROG) -P $(PORT) -c $(PROG) -U flash:w:$(PROJ)_boot.hex:i

# -- show asm code
asm: default
	avr-objdump -dS $(PROJ).elf

# -- delete output files
clean:
	rm -rf *.o *.elf *.hex $(LIBDIR)*.o

# ----------- core compilation targets -----------------------------------------
# -- compilation
%.o: %.c
	$(CC) $(CFLAGS) -I $(LIBDIR) -c -o $@ $< 

# -- linking
%.elf: %.o $(DEPS)
	$(CC) $(CFLAGS) -o $@ $< $(DEPS)

# -- keep object files
.PRECIOUS: %.o









