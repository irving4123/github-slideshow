#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "config.h"

// ============================================================================
// ADC配置定义
// ============================================================================

// ADC通道定义
typedef enum {
    ADC_CHANNEL_0 = 0,     // 通道0
    ADC_CHANNEL_1,         // 通道1
    ADC_CHANNEL_2,         // 通道2
    ADC_CHANNEL_3,         // 通道3
    ADC_CHANNEL_4,         // 通道4
    ADC_CHANNEL_5,         // 通道5
    ADC_CHANNEL_6,         // 通道6
    ADC_CHANNEL_7          // 通道7
} adc_channel_t;

// ADC转换模式
typedef enum {
    ADC_MODE_SINGLE = 0,   // 单次转换
    ADC_MODE_CONTINUOUS,   // 连续转换
    ADC_MODE_SCAN          // 扫描转换
} adc_mode_t;

// ADC配置结构体
typedef struct {
    adc_mode_t mode;           // 转换模式
    uint8_t channel_mask;      // 通道掩码
    uint16_t sample_time;      // 采样时间
    uint8_t resolution;        // 分辨率
    float reference_voltage;   // 参考电压
} adc_config_t;

// ADC状态结构体
typedef struct {
    uint8_t is_initialized;    // 初始化状态
    uint8_t is_converting;     // 转换状态
    uint16_t conversion_count; // 转换计数
    uint32_t error_count;      // 错误计数
} adc_status_t;

// ADC数据结构体
typedef struct {
    uint16_t raw_data[8];      // 原始数据
    float voltage_data[8];     // 电压数据
    float current_data[3];     // 电流数据
    float temperature;         // 温度数据
    float dc_voltage;          // 直流电压
} adc_data_t;

// ============================================================================
// 函数声明
// ============================================================================

// ADC驱动初始化
void adc_driver_init(void);

// ADC配置设置
void adc_set_config(adc_config_t* config);

// ADC使能/禁用
void adc_enable(uint8_t enable);

// ADC通道使能/禁用
void adc_channel_enable(adc_channel_t channel, uint8_t enable);

// 启动ADC转换
void adc_start_conversion(void);

// ADC转换完成处理
void adc_conversion_complete(void);

// 读取ADC原始数据
uint16_t adc_read_raw(adc_channel_t channel);

// 读取ADC电压值
float adc_read_voltage(adc_channel_t channel);

// 读取电流值
float adc_read_current_a(void);
float adc_read_current_b(void);
float adc_read_current_c(void);

// 读取电压值
float adc_read_voltage_a(void);
float adc_read_voltage_b(void);
float adc_read_voltage_c(void);

// 读取温度值
float adc_read_temperature(void);

// 读取直流电压
float adc_get_dc_voltage(void);

// 读取温度
float adc_get_temperature(void);

// ADC校准
void adc_calibrate(void);

// ADC自检
uint8_t adc_self_test(void);

// 获取ADC配置
adc_config_t* adc_get_config(void);

// 获取ADC状态
adc_status_t* adc_get_status(void);

// 获取ADC数据
adc_data_t* adc_get_data(void);

// ADC调试
void adc_debug_print(void);

// ADC参数显示
void adc_parameter_display(void);

// ============================================================================
// 内联函数
// ============================================================================

// 检查ADC是否初始化
static inline uint8_t adc_is_initialized(void) {
    adc_status_t* status = adc_get_status();
    return status->is_initialized;
}

// 检查ADC是否正在转换
static inline uint8_t adc_is_converting(void) {
    adc_status_t* status = adc_get_status();
    return status->is_converting;
}

// 原始数据转电压
static inline float adc_raw_to_voltage(uint16_t raw_data) {
    adc_config_t* config = adc_get_config();
    return (float)raw_data * config->reference_voltage / 4096.0f;
}

// 电压转原始数据
static inline uint16_t adc_voltage_to_raw(float voltage) {
    adc_config_t* config = adc_get_config();
    return (uint16_t)(voltage * 4096.0f / config->reference_voltage);
}

#endif // ADC_DRIVER_H