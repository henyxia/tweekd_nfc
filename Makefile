export CC = avr-g++

export MCU = atmega328p
export TARGET_ARCH = -mmcu=$(MCU)

export CFLAGS =  -Wall -I./ -I/usr/share/arduino/hardware/arduino/variants/standard -I./PN532_SPI -I./PN532 -I/usr/share/arduino/hardware/arduino/cores/arduino -I/usr/share/arduino/libraries/SPI -I/usr/share/arduino/libraries -DF_CPU=16000000 -Os #-g
export LDFLAGS = -g $(TARGET_ARCH) -lm -Wl,--gc-sections #	-Os

TARGET = usb
TERM = /dev/ttyACM0
CPPFLAGS = -mmcu=$(MCU)
PGMER = -c stk500v1 -b 57600 -P $(TERM)
PGMERISP = -c stk500v1 -b 115200 -P $(TERM)
ARVDUDECONF= -C /usr/local/arduino/arduino-0022/hardware/tools/avrdude.conf
export DUDE = /usr/bin/avrdude -F -v -p $(MCU) $(AVRDUDECONF)

C_SRC = main.c Arduino/wiring_digital.c
CPP_SRC = PN532_SPI/PN532_SPI.cpp PN532/PN532.cpp PN532/SPI.cpp
OBJS = $(C_SRC:.c=.o)
OBJS += $(CPP_SRC:.cpp=.o)

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.elf

%.o:%.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
%.o:%.cpp
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -j .text -j .data -O ihex $(TARGET).elf $(TARGET).hex
	avr-objcopy -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $(TARGET).elf eeprom.hex

upload: $(TARGET).hex
	stty -F $(TERM) hupcl # reset
	$(DUDE) $(PGMERISP) -U flash:w:$(TARGET).hex

