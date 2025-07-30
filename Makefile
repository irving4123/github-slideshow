# ============================================================================
# 8051 MCU 无传感器 BEMF 电机控制项目 Makefile
# ============================================================================

# 项目名称
PROJECT_NAME = motor_control_8051

# 编译器设置
CC = sdcc
AS = sdas8051
AR = sdar
LD = sdld

# 目标MCU
MCU = 8051
TARGET = --model-large

# 编译选项
CFLAGS = -c $(TARGET) --opt-code-size --opt-code-speed
ASFLAGS = -plosgff
LDFLAGS = $(TARGET) --code-loc 0x0000 --xram-loc 0x0000

# 目录设置
SRC_DIR = src
INCLUDE_DIR = include
LIB_DIR = lib
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# 源文件
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.rel)

# 头文件路径
INCLUDES = -I$(INCLUDE_DIR)

# 目标文件
TARGET_HEX = $(BIN_DIR)/$(PROJECT_NAME).hex
TARGET_IHX = $(BIN_DIR)/$(PROJECT_NAME).ihx
TARGET_MAP = $(BIN_DIR)/$(PROJECT_NAME).map

# 默认目标
all: $(TARGET_HEX)

# 创建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)
	mkdir -p $(BIN_DIR)

# 编译源文件
$(OBJ_DIR)/%.rel: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

# 链接目标文件
$(TARGET_IHX): $(OBJ_FILES)
	$(CC) $(LDFLAGS) -o $@ $^

# 生成HEX文件
$(TARGET_HEX): $(TARGET_IHX)
	packihx $< > $@

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重新编译
rebuild: clean all

# 烧录到MCU (需要根据实际烧录器调整)
flash: $(TARGET_HEX)
	@echo "烧录到MCU..."
	@echo "请根据实际烧录器调整烧录命令"
	# 示例: stcgal -p /dev/ttyUSB0 -P stc89 $(TARGET_HEX)

# 调试信息
debug: CFLAGS += --debug
debug: $(TARGET_HEX)

# 优化编译
release: CFLAGS += --opt-code-size --opt-code-speed
release: $(TARGET_HEX)

# 显示帮助
help:
	@echo "可用目标:"
	@echo "  all      - 编译项目"
	@echo "  clean    - 清理编译文件"
	@echo "  rebuild  - 重新编译"
	@echo "  flash    - 烧录到MCU"
	@echo "  debug    - 调试版本编译"
	@echo "  release  - 发布版本编译"
	@echo "  help     - 显示此帮助"

# 显示项目信息
info:
	@echo "项目信息:"
	@echo "  项目名称: $(PROJECT_NAME)"
	@echo "  目标MCU: $(MCU)"
	@echo "  编译器: $(CC)"
	@echo "  源文件: $(SRC_FILES)"
	@echo "  目标文件: $(OBJ_FILES)"

# 检查依赖
check:
	@echo "检查依赖..."
	@which $(CC) > /dev/null || (echo "错误: 未找到编译器 $(CC)" && exit 1)
	@echo "编译器检查通过"

# 安装依赖 (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install sdcc

# 安装依赖 (CentOS/RHEL)
install-deps-centos:
	sudo yum install sdcc

# 安装依赖 (macOS)
install-deps-macos:
	brew install sdcc

# 代码格式化
format:
	@echo "格式化代码..."
	@find $(SRC_DIR) -name "*.c" -exec clang-format -i {} \;
	@find $(INCLUDE_DIR) -name "*.h" -exec clang-format -i {} \;

# 代码检查
lint:
	@echo "代码检查..."
	@find $(SRC_DIR) -name "*.c" -exec cppcheck --enable=all {} \;

# 生成文档
docs:
	@echo "生成文档..."
	@mkdir -p docs
	@doxygen Doxyfile 2>/dev/null || echo "请安装doxygen以生成文档"

# 运行测试
test:
	@echo "运行测试..."
	@echo "测试功能待实现"

# 显示文件大小
size: $(TARGET_HEX)
	@echo "文件大小信息:"
	@ls -lh $(TARGET_HEX)
	@echo "代码段大小:"
	@size $(TARGET_IHX) 2>/dev/null || echo "无法获取大小信息"

# 显示编译统计
stats:
	@echo "编译统计:"
	@echo "源文件数量: $(words $(SRC_FILES))"
	@echo "目标文件数量: $(words $(OBJ_FILES))"
	@echo "头文件数量: $(words $(wildcard $(INCLUDE_DIR)/*.h))"

# 伪目标
.PHONY: all clean rebuild flash debug release help info check install-deps format lint docs test size stats