# Makefile for Sensorless BEMF Motor Control
# Optimized for 24MHz MCU operation

# Project name
PROJECT = bemf_sensorless

# MCU configuration
MCU = atmega328p
F_CPU = 24000000UL
PROGRAMMER = usbasp
PORT = usb

# Compiler and tools
CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE = avr-size
AVRDUDE = avrdude

# Compiler flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -std=c99
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wl,--gc-sections -Wl,--relax

# Optimization flags for 24MHz operation
CFLAGS += -mcall-prologues -mno-tablejump

# Debug flags (uncomment for debugging)
# CFLAGS += -g -DDEBUG

# Include directories
INCLUDES = -I.

# Source files
SOURCES = main.c bemf_sensorless.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Header files
HEADERS = bemf_sensorless.h mcu_config.h

# Output files
TARGET_HEX = $(PROJECT).hex
TARGET_ELF = $(PROJECT).elf
TARGET_LSS = $(PROJECT).lss
TARGET_MAP = $(PROJECT).map

# Default target
all: $(TARGET_HEX) size

# Build hex file
$(TARGET_HEX): $(TARGET_ELF)
	@echo "Creating hex file: $@"
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Build elf file
$(TARGET_ELF): $(OBJECTS)
	@echo "Linking: $@"
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ -Wl,-Map,$(TARGET_MAP)

# Compile source files
%.o: %.c $(HEADERS)
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Show memory usage
size: $(TARGET_ELF)
	@echo "Memory usage:"
	$(SIZE) --format=avr --mcu=$(MCU) $(TARGET_ELF)

# Generate assembly listing
listing: $(TARGET_LSS)

$(TARGET_LSS): $(TARGET_ELF)
	@echo "Creating listing: $@"
	$(OBJDUMP) -h -S $< > $@

# Programming targets
program: $(TARGET_HEX)
	@echo "Programming MCU with $(PROGRAMMER)..."
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) -U flash:w:$(TARGET_HEX):i

program-usbasp: $(TARGET_HEX)
	@echo "Programming with USBasp..."
	$(AVRDUDE) -c usbasp -p $(MCU) -U flash:w:$(TARGET_HEX):i

program-arduino: $(TARGET_HEX)
	@echo "Programming with Arduino as ISP..."
	$(AVRDUDE) -c arduino -p $(MCU) -P /dev/ttyUSB0 -b 19200 -U flash:w:$(TARGET_HEX):i

# Fuse settings for 24MHz external crystal
# Low fuse: External crystal/resonator, slowly rising power
# High fuse: Default settings, enable SPI programming
# Extended fuse: Brown-out detection at 2.7V
fuses:
	@echo "Setting fuses for 24MHz external crystal..."
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) \
		-U lfuse:w:0xF7:m -U hfuse:w:0xD9:m -U efuse:w:0xFD:m

# Fuse settings for internal 8MHz RC oscillator (if 24MHz not available)
fuses-internal:
	@echo "Setting fuses for internal 8MHz RC oscillator..."
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) \
		-U lfuse:w:0xE2:m -U hfuse:w:0xD9:m -U efuse:w:0xFD:m

# Read current fuse settings
read-fuses:
	@echo "Reading current fuse settings..."
	$(AVRDUDE) -c $(PROGRAMMER) -p $(MCU) -P $(PORT) \
		-U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -f $(OBJECTS) $(TARGET_ELF) $(TARGET_HEX) $(TARGET_LSS) $(TARGET_MAP)
	rm -f *.o *.elf *.hex *.lss *.map

# Clean everything including backup files
distclean: clean
	@echo "Cleaning all generated files..."
	rm -f *~ *.bak *.orig

# Create backup of source files
backup:
	@echo "Creating backup..."
	tar -czf $(PROJECT)_backup_$(shell date +%Y%m%d_%H%M%S).tar.gz \
		*.c *.h Makefile README.md

# Development targets
debug: CFLAGS += -g -DDEBUG -O0
debug: $(TARGET_HEX)

release: CFLAGS += -Os -DNDEBUG
release: $(TARGET_HEX)

# Check code with static analysis (requires cppcheck)
check:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c99 $(SOURCES) $(HEADERS)

# Format code (requires astyle)
format:
	@echo "Formatting code..."
	astyle --style=linux --indent=spaces=4 --max-code-length=80 \
		--pad-oper --pad-header --unpad-paren --align-pointer=name \
		--suffix=none $(SOURCES) $(HEADERS)

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build hex file and show size"
	@echo "  clean        - Remove build files"
	@echo "  distclean    - Remove all generated files"
	@echo "  size         - Show memory usage"
	@echo "  listing      - Generate assembly listing"
	@echo "  program      - Program MCU with default programmer"
	@echo "  program-usbasp   - Program with USBasp"
	@echo "  program-arduino  - Program with Arduino as ISP"
	@echo "  fuses        - Set fuses for 24MHz external crystal"
	@echo "  fuses-internal   - Set fuses for internal 8MHz"
	@echo "  read-fuses   - Read current fuse settings"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  backup       - Create backup of source files"
	@echo "  check        - Run static code analysis"
	@echo "  format       - Format source code"
	@echo "  help         - Show this help"

# Project information
info:
	@echo "Project: $(PROJECT)"
	@echo "MCU: $(MCU)"
	@echo "F_CPU: $(F_CPU)"
	@echo "Programmer: $(PROGRAMMER)"
	@echo "Source files: $(SOURCES)"
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"

# Dependencies
$(OBJECTS): $(HEADERS)

# Phony targets
.PHONY: all clean distclean size listing program program-usbasp program-arduino
.PHONY: fuses fuses-internal read-fuses debug release backup check format help info