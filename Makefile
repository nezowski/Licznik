PROGRAMMER = stk500v1
PROGRAM_BAUD_RATE = 19200
PROGRAM_PORT = /dev/ttyACM1

MCU = t84a

MCU_ARCHITECTURE = attiny84
F_CPU = 8000000

LFUSE = 0x62
HFUSE = 0xdf
EFUSE = 0xff

CFLAGS = -DF_CPU=$(F_CPU)UL
CFLAGS += -mmcu=$(MCU_ARCHITECTURE)
CFLAGS += -Os

AVRDUDE_FLAGS = -c $(PROGRAMMER)
AVRDUDE_FLAGS += -p $(MCU)
AVRDUDE_FLAGS += -b $(PROGRAM_BAUD_RATE)
AVRDUDE_FLAGS += -P $(PROGRAM_PORT)


compile: main.c
	@echo Compiling...
	avr-gcc $(CFLAGS) main.c -o main.elf

send: compile

	@echo Sending...
	avrdude $(AVRDUDE_FLAGS) -U flash:w:main.elf

disassemble: compile

	@echo Disassembling...
	../adump.py attiny84a -S main.elf | nvim

clean:
	rm *.elf
