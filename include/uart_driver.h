#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "config.h"

// ============================================================================
// UART配置定义
// ============================================================================

// UART端口定义
typedef enum {
    UART_PORT_0 = 0,       // UART0
    UART_PORT_1             // UART1
} uart_port_t;

// UART配置结构体
typedef struct {
    uint32_t baudrate;      // 波特率
    uint8_t data_bits;      // 数据位
    uint8_t stop_bits;      // 停止位
    uint8_t parity;         // 校验位
    uint8_t flow_control;   // 流控制
} uart_config_t;

// UART状态结构体
typedef struct {
    uint8_t is_initialized; // 初始化状态
    uint8_t is_enabled;     // 使能状态
    uint32_t tx_count;      // 发送计数
    uint32_t rx_count;      // 接收计数
    uint32_t error_count;   // 错误计数
} uart_status_t;

// UART缓冲区结构体
typedef struct {
    uint8_t buffer[UART_BUFFER_SIZE];  // 缓冲区
    uint16_t head;                     // 头指针
    uint16_t tail;                     // 尾指针
    uint16_t count;                    // 数据计数
} uart_buffer_t;

// ============================================================================
// 函数声明
// ============================================================================

// UART驱动初始化
void uart_driver_init(void);

// UART配置设置
void uart_set_config(uart_port_t port, uart_config_t* config);

// UART使能/禁用
void uart_enable(uart_port_t port, uint8_t enable);

// UART发送单个字符
void uart_send_char(uart_port_t port, uint8_t ch);

// UART发送字符串
void uart_send_string(const char* str);

// UART发送数据
void uart_send_data(uart_port_t port, uint8_t* data, uint16_t length);

// UART接收单个字符
uint8_t uart_receive_char(uart_port_t port);

// UART接收数据
uint16_t uart_receive_data(uart_port_t port, uint8_t* data, uint16_t length);

// UART处理接收数据
void uart_process_rx(void);

// UART接收中断处理
void uart_rx_handler(void);

// UART发送状态
void uart_send_status(void);

// UART发送浮点数
void uart_send_float(float value);

// UART发送整数
void uart_send_int(int32_t value);

// UART发送十六进制
void uart_send_hex(uint32_t value);

// UART清空缓冲区
void uart_clear_buffer(uart_port_t port);

// UART获取缓冲区数据量
uint16_t uart_get_buffer_count(uart_port_t port);

// UART检查是否有数据
uint8_t uart_has_data(uart_port_t port);

// UART设置波特率
void uart_set_baudrate(uart_port_t port, uint32_t baudrate);

// UART自检
uint8_t uart_self_test(uart_port_t port);

// 获取UART配置
uart_config_t* uart_get_config(uart_port_t port);

// 获取UART状态
uart_status_t* uart_get_status(uart_port_t port);

// 获取UART缓冲区
uart_buffer_t* uart_get_buffer(uart_port_t port);

// UART调试
void uart_debug_print(void);

// UART参数显示
void uart_parameter_display(void);

// ============================================================================
// 内联函数
// ============================================================================

// 检查UART是否初始化
static inline uint8_t uart_is_initialized(uart_port_t port) {
    uart_status_t* status = uart_get_status(port);
    return status->is_initialized;
}

// 检查UART是否使能
static inline uint8_t uart_is_enabled(uart_port_t port) {
    uart_status_t* status = uart_get_status(port);
    return status->is_enabled;
}

// 检查UART缓冲区是否满
static inline uint8_t uart_buffer_is_full(uart_port_t port) {
    uart_buffer_t* buffer = uart_get_buffer(port);
    return (buffer->count >= UART_BUFFER_SIZE);
}

// 检查UART缓冲区是否空
static inline uint8_t uart_buffer_is_empty(uart_port_t port) {
    uart_buffer_t* buffer = uart_get_buffer(port);
    return (buffer->count == 0);
}

// 获取UART缓冲区可用空间
static inline uint16_t uart_buffer_available(uart_port_t port) {
    uart_buffer_t* buffer = uart_get_buffer(port);
    return (UART_BUFFER_SIZE - buffer->count);
}

#endif // UART_DRIVER_H