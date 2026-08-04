TARGET := wio_ai_boot_stub
BUILD_DIR ?= build
PREFIX := arm-none-eabi-

ifdef GCC_PATH
CC := $(GCC_PATH)/$(PREFIX)gcc
OBJCOPY := $(GCC_PATH)/$(PREFIX)objcopy
SIZE := $(GCC_PATH)/$(PREFIX)size
else
CC := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
endif

MCU_FLAGS := -mcpu=cortex-m7 -mthumb
LINKER_SCRIPT := STM32H725AEIX_BOOT.ld
OBJECT := $(BUILD_DIR)/boot_stub.o
ELF := $(BUILD_DIR)/$(TARGET).elf
HEX := $(BUILD_DIR)/$(TARGET).hex
BIN := $(BUILD_DIR)/$(TARGET).bin
MAP := $(BUILD_DIR)/$(TARGET).map

.PHONY: all clean

all: $(ELF) $(HEX) $(BIN)

$(BUILD_DIR):
	mkdir -p $@

$(OBJECT): boot_stub.S Makefile | $(BUILD_DIR)
	$(CC) -x assembler-with-cpp -c $(MCU_FLAGS) \
		-ffunction-sections -fdata-sections $< -o $@

$(ELF): $(OBJECT) $(LINKER_SCRIPT)
	$(CC) $(MCU_FLAGS) -nostdlib -Wl,--gc-sections -Wl,--build-id=none \
		-T$(LINKER_SCRIPT) -Wl,-Map=$(MAP) $(OBJECT) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

clean:
	rm -rf $(BUILD_DIR)
