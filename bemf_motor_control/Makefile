# BEMF无传感器电机控制系统 Makefile
# 适用于SDCC编译器 (Small Device C Compiler)

# 项目配置
PROJECT_NAME = bemf_motor_control
MCU = 8051
CLOCK_FREQ = 24000000

# 编译器配置
CC = sdcc
CFLAGS = -mmcs51 --model-small --stack-auto --iram-size 256 --xram-size 2048 --code-size 18432
CFLAGS += -DCLOCK_FREQ=$(CLOCK_FREQ)
CFLAGS += -I./include
CFLAGS += --opt-code-speed
CFLAGS += --peep-hole

# 链接器配置
LDFLAGS = --model-small --out-fmt-ihx

# 目录配置
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
DIST_DIR = dist

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.rel)
HEADERS = $(wildcard $(INC_DIR)/*.h)

# 目标文件
TARGET = $(BUILD_DIR)/$(PROJECT_NAME)
HEX_FILE = $(DIST_DIR)/$(PROJECT_NAME).hex
BIN_FILE = $(DIST_DIR)/$(PROJECT_NAME).bin

# 默认目标
all: directories $(HEX_FILE)

# 创建目录
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(DIST_DIR)

# 编译规则
$(BUILD_DIR)/%.rel: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 链接规则
$(TARGET).ihx: $(OBJECTS)
	@echo "Linking..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

# 生成HEX文件
$(HEX_FILE): $(TARGET).ihx
	@echo "Generating HEX file..."
	@cp $(TARGET).ihx $(HEX_FILE)

# 生成二进制文件
$(BIN_FILE): $(HEX_FILE)
	@echo "Generating binary file..."
	hex2bin $(HEX_FILE)
	@mv $(DIST_DIR)/$(PROJECT_NAME).bin $(BIN_FILE)

# 清理
clean:
	@echo "Cleaning..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(DIST_DIR)
	@rm -f *.asm *.lst *.map *.mem *.lk *.omf *.cdb *.adb

# 完全清理
distclean: clean
	@rm -f *~

# 显示项目信息
info:
	@echo "Project: $(PROJECT_NAME)"
	@echo "MCU: $(MCU)"
	@echo "Clock: $(CLOCK_FREQ) Hz"
	@echo "Compiler: $(CC)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"

# 代码大小分析
size: $(TARGET).ihx
	@echo "Code size analysis:"
	@grep "ROM/EPROM/FLASH" $(TARGET).map || echo "Map file not found"

# 烧录到MCU (需要配置烧录器)
flash: $(HEX_FILE)
	@echo "Flashing $(HEX_FILE) to MCU..."
	# stcgal -p /dev/ttyUSB0 $(HEX_FILE)  # 示例：使用stcgal烧录STC单片机
	@echo "Please configure your programmer in the Makefile"

# 调试目标
debug: CFLAGS += --debug
debug: $(HEX_FILE)
	@echo "Debug version built"

# 发布目标
release: CFLAGS += -DNDEBUG
release: $(HEX_FILE)
	@echo "Release version built"

# 检查语法
check:
	@echo "Syntax checking..."
	@for file in $(SOURCES); do \
		echo "Checking $$file..."; \
		$(CC) $(CFLAGS) -fsyntax-only $$file; \
	done

# 生成文档
docs:
	@echo "Generating documentation..."
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
	else \
		echo "Doxygen not found. Please install doxygen to generate documentation."; \
	fi

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build files"
	@echo "  distclean - Remove all generated files"
	@echo "  info      - Show project information"
	@echo "  size      - Show code size analysis"
	@echo "  flash     - Flash firmware to MCU"
	@echo "  debug     - Build debug version"
	@echo "  release   - Build release version"
	@echo "  check     - Check syntax"
	@echo "  docs      - Generate documentation"
	@echo "  help      - Show this help"

# 伪目标
.PHONY: all clean distclean info size flash debug release check docs help directories