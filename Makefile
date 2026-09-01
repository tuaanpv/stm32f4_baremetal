TARGET  = firmware
BUILD_DIR = build

CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE    = arm-none-eabi-size

CPU     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS  = $(CPU) -Wall -Wextra -O0 -g3 -std=c11 -ffunction-sections -fdata-sections
CFLAGS += -Iinc
# -nostartfiles: skip the standard C runtime startup (we provide our own
#   Reset_Handler in startup_stm32f407xx.s instead of libc's crt0).
# --specs=nosys.specs: stub out the syscalls (_read, _write, _sbrk, ...) that
#   newlib expects to exist, since there is no OS underneath. This lets us
#   still link libc/libm for things like memcpy/memset/strchr (needed by
#   FatFs) without pulling in a full hosted C runtime.
LDFLAGS = $(CPU) -Tlinker/stm32f407.ld -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
LDFLAGS += -nostartfiles --specs=nosys.specs -lc -lm

C_SOURCES = $(wildcard src/*.c)
ASM_SOURCES = startup/startup_stm32f407xx.s

OBJECTS  = $(addprefix $(BUILD_DIR)/, $(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/, $(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c src
vpath %.s startup

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(CPU) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR):
	mkdir -p $@

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $(BUILD_DIR)/$(TARGET).bin 0x08000000

# or use OpenOCD:
# openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program $(BUILD_DIR)/$(TARGET).elf verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash
