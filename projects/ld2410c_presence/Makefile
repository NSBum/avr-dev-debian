MCU       := attiny3217
F_CPU     := 20000000UL
TARGET    := ld2410c_presence
BUILD_DIR := build

CC      := avr-gcc
OBJCOPY := avr-objcopy
SIZE    := avr-size

PROGRAMMER   := atmelice_updi
AVRDUDE_PART := t3217

CFLAGS := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu11 \
           -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
           -MMD -MP -Iinclude \
           -I../../tinyavr1-libraries/twi-slave \
           -I../../tinyavr1-libraries/ld2410c \
           -I../../tinyavr1-libraries/software-uart \
           -I../../tinyavr1-libraries/dfplayer
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections

SOURCES := src/main.c \
           ../../tinyavr1-libraries/twi-slave/tinyavr1_twi_slave.c \
           ../../tinyavr1-libraries/ld2410c/tinyavr1_ld2410c.c \
           ../../tinyavr1-libraries/software-uart/tinyavr1_soft_uart.c \
           ../../tinyavr1-libraries/dfplayer/tinyavr1_dfplayer.c \
           ../../tinyavr1-libraries/dfplayer/tinyavr1_dfplayer_soft_uart.c
OBJECTS := $(BUILD_DIR)/main.o \
           $(BUILD_DIR)/tinyavr1_twi_slave.o \
           $(BUILD_DIR)/tinyavr1_ld2410c.o \
           $(BUILD_DIR)/tinyavr1_soft_uart.o \
           $(BUILD_DIR)/tinyavr1_dfplayer.o \
           $(BUILD_DIR)/tinyavr1_dfplayer_soft_uart.o
DEPS    := $(OBJECTS:.o=.d)
ELF     := $(BUILD_DIR)/$(TARGET).elf
HEX     := $(BUILD_DIR)/$(TARGET).hex

.PHONY: all clean size identify flash verify

all: $(HEX)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_twi_slave.o: ../../tinyavr1-libraries/twi-slave/tinyavr1_twi_slave.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_ld2410c.o: ../../tinyavr1-libraries/ld2410c/tinyavr1_ld2410c.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_soft_uart.o: ../../tinyavr1-libraries/software-uart/tinyavr1_soft_uart.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_dfplayer.o: ../../tinyavr1-libraries/dfplayer/tinyavr1_dfplayer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_dfplayer_soft_uart.o: ../../tinyavr1-libraries/dfplayer/tinyavr1_dfplayer_soft_uart.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ELF): $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex -R .eeprom $< $@

size: $(ELF)
	$(SIZE) --format=avr --mcu=$(MCU) $<

identify:
	avrdude -c $(PROGRAMMER) -p $(AVRDUDE_PART)

flash: $(HEX)
	avrdude -c $(PROGRAMMER) -p $(AVRDUDE_PART) -U flash:w:$(HEX):i

verify: $(HEX)
	avrdude -c $(PROGRAMMER) -p $(AVRDUDE_PART) -U flash:v:$(HEX):i

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
