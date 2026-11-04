MCU       := attiny3217
F_CPU     := 20000000UL
TARGET    := test_dfplayer_3217
BUILD_DIR := build

CC      := avr-gcc
OBJCOPY := avr-objcopy
SIZE    := avr-size

PROGRAMMER   := atmelice_updi
AVRDUDE_PART := t3217

LIB_ROOT := ../../tinyavr1-libraries

CFLAGS := -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=gnu11 \
           -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
           -MMD -MP -Iinclude -I$(LIB_ROOT)/software-uart \
           -I$(LIB_ROOT)/dfplayer
LDFLAGS := -mmcu=$(MCU) -Wl,--gc-sections

OBJECTS := $(BUILD_DIR)/main.o \
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

$(BUILD_DIR)/main.o: src/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_soft_uart.o: $(LIB_ROOT)/software-uart/tinyavr1_soft_uart.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_dfplayer.o: $(LIB_ROOT)/dfplayer/tinyavr1_dfplayer.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/tinyavr1_dfplayer_soft_uart.o: $(LIB_ROOT)/dfplayer/tinyavr1_dfplayer_soft_uart.c | $(BUILD_DIR)
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
