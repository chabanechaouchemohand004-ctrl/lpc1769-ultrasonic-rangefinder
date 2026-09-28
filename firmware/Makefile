# Compilation en ligne de commande (sans Keil) : make | make test | make clean
CROSS   ?= arm-none-eabi-
CC      := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE    := $(CROSS)size

BUILD   := build
TARGET  := $(BUILD)/rangefinder
SRCS    := src/main.c src/ultrasonic.c src/uart0.c \
           startup/startup_lpc17xx.c startup/system_LPC17xx.c
OBJS    := $(SRCS:%.c=$(BUILD)/%.o)

CFLAGS  := -mcpu=cortex-m3 -mthumb -Os -g -std=gnu99 -Wall -Wextra \
           -ffunction-sections -fdata-sections -ffreestanding \
           -fno-tree-loop-distribute-patterns -Isrc -Icmsis
LDFLAGS := -mcpu=cortex-m3 -mthumb -nostdlib -Tstartup/lpc1769.ld \
           -Wl,--gc-sections -Wl,-Map=$(TARGET).map

all: $(TARGET).bin $(TARGET).hex
	@$(SIZE) $(TARGET).elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -lgcc -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	python3 ../tools/lpc_checksum.py $@

$(TARGET).hex: $(TARGET).bin
	$(OBJCOPY) -I binary -O ihex $< $@

test:
	$(MAKE) -C test

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
