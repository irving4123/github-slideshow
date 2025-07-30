#include "adc_driver.h"
#include "uart_driver.h"
#include <string.h>

// ============================================================================
// 全局变量
// ============================================================================

// ADC配置结构体
static adc_config_t adc_config;

// ADC状态结构体
static adc_status_t adc_status;

// ADC数据结构体
static adc_data_t adc_data;

// ADC转换完成标志
static uint8_t adc_conversion_complete_flag = 0;

// ADC原始数据
static uint16_t adc_raw_values[8] = {0};

// ============================================================================
// ADC驱动初始化
// ============================================================================

void adc_driver_init(void)
{
    // 初始化ADC配置
    adc_config.mode = ADC_MODE_SINGLE;
    adc_config.channel_mask = 0xFF; // 使能所有通道
    adc_config.sample_time = 100;   // 100个时钟周期采样时间
    adc_config.resolution = 12;     // 12位分辨率
    adc_config.reference_voltage = ADC_REF_VOLTAGE;
    
    // 初始化ADC状态
    adc_status.is_initialized = 1;
    adc_status.is_converting = 0;
    adc_status.conversion_count = 0;
    adc_status.error_count = 0;
    
    // 初始化ADC数据
    memset(&adc_data, 0, sizeof(adc_data_t));
    
    // 初始化原始数据
    memset(adc_raw_values, 0, sizeof(adc_raw_values));
    
    // 配置ADC寄存器 (这里需要根据实际MCU寄存器进行配置)
    // 示例代码，需要根据实际硬件调整
    /*
    ADC_CON = 0x80;  // 使能ADC
    ADC_CHS = 0x00;  // 选择通道0
    ADC_CLK = 0x01;  // 设置时钟
    */
    
    uart_send_string("ADC driver initialized\r\n");
}

// ============================================================================
// ADC配置设置
// ============================================================================

void adc_set_config(adc_config_t* config)
{
    if (config != NULL) {
        memcpy(&adc_config, config, sizeof(adc_config_t));
    }
}

// ============================================================================
// ADC使能/禁用
// ============================================================================

void adc_enable(uint8_t enable)
{
    if (enable) {
        // 使能ADC
        // ADC_CON |= 0x80;
        adc_status.is_initialized = 1;
    } else {
        // 禁用ADC
        // ADC_CON &= ~0x80;
        adc_status.is_initialized = 0;
    }
}

// ============================================================================
// ADC通道使能/禁用
// ============================================================================

void adc_channel_enable(adc_channel_t channel, uint8_t enable)
{
    if (enable) {
        adc_config.channel_mask |= (1 << channel);
    } else {
        adc_config.channel_mask &= ~(1 << channel);
    }
}

// ============================================================================
// 启动ADC转换
// ============================================================================

void adc_start_conversion(void)
{
    if (!adc_status.is_initialized) {
        return;
    }
    
    // 启动转换
    // ADC_CON |= 0x40;  // 启动转换位
    adc_status.is_converting = 1;
    adc_conversion_complete_flag = 0;
}

// ============================================================================
// ADC转换完成处理
// ============================================================================

void adc_conversion_complete(void)
{
    if (adc_status.is_converting) {
        // 读取转换结果
        // adc_raw_values[current_channel] = ADC_DATA;
        
        // 模拟读取数据 (实际应用中需要从硬件寄存器读取)
        static uint8_t current_channel = 0;
        adc_raw_values[current_channel] = (uint16_t)(current_channel * 100 + 500);
        
        // 转换为电压值
        adc_data.voltage_data[current_channel] = adc_raw_to_voltage(adc_raw_values[current_channel]);
        
        // 更新通道
        current_channel = (current_channel + 1) % 8;
        
        // 更新状态
        adc_status.is_converting = 0;
        adc_status.conversion_count++;
        adc_conversion_complete_flag = 1;
        
        // 更新特定数据
        adc_data.current_data[0] = adc_data.voltage_data[0] / CURRENT_SENSOR_GAIN;
        adc_data.current_data[1] = adc_data.voltage_data[1] / CURRENT_SENSOR_GAIN;
        adc_data.current_data[2] = adc_data.voltage_data[2] / CURRENT_SENSOR_GAIN;
        
        adc_data.dc_voltage = adc_data.voltage_data[3] * 2.0f; // 假设分压比
        adc_data.temperature = (adc_data.voltage_data[4] - 0.5f) * 100.0f; // 温度传感器转换
    }
}

// ============================================================================
// 读取ADC原始数据
// ============================================================================

uint16_t adc_read_raw(adc_channel_t channel)
{
    if (channel < 8) {
        return adc_raw_values[channel];
    }
    return 0;
}

// ============================================================================
// 读取ADC电压值
// ============================================================================

float adc_read_voltage(adc_channel_t channel)
{
    if (channel < 8) {
        return adc_data.voltage_data[channel];
    }
    return 0.0f;
}

// ============================================================================
// 读取电流值
// ============================================================================

float adc_read_current_a(void)
{
    return adc_data.current_data[0];
}

float adc_read_current_b(void)
{
    return adc_data.current_data[1];
}

float adc_read_current_c(void)
{
    return adc_data.current_data[2];
}

// ============================================================================
// 读取电压值
// ============================================================================

float adc_read_voltage_a(void)
{
    return adc_data.voltage_data[0];
}

float adc_read_voltage_b(void)
{
    return adc_data.voltage_data[1];
}

float adc_read_voltage_c(void)
{
    return adc_data.voltage_data[2];
}

// ============================================================================
// 读取温度值
// ============================================================================

float adc_read_temperature(void)
{
    return adc_data.temperature;
}

// ============================================================================
// 读取直流电压
// ============================================================================

float adc_get_dc_voltage(void)
{
    return adc_data.dc_voltage;
}

// ============================================================================
// 读取温度
// ============================================================================

float adc_get_temperature(void)
{
    return adc_data.temperature;
}

// ============================================================================
// ADC校准
// ============================================================================

void adc_calibrate(void)
{
    // 简单的校准过程
    uart_send_string("ADC calibration started\r\n");
    
    // 读取多次采样进行平均
    uint32_t sum[8] = {0};
    uint16_t samples = 100;
    
    for (uint16_t i = 0; i < samples; i++) {
        adc_start_conversion();
        while (!adc_conversion_complete_flag) {
            // 等待转换完成
        }
        
        for (uint8_t ch = 0; ch < 8; ch++) {
            sum[ch] += adc_raw_values[ch];
        }
    }
    
    // 计算平均值
    for (uint8_t ch = 0; ch < 8; ch++) {
        adc_raw_values[ch] = (uint16_t)(sum[ch] / samples);
        adc_data.voltage_data[ch] = adc_raw_to_voltage(adc_raw_values[ch]);
    }
    
    uart_send_string("ADC calibration completed\r\n");
}

// ============================================================================
// ADC自检
// ============================================================================

uint8_t adc_self_test(void)
{
    if (!adc_status.is_initialized) {
        return 0;
    }
    
    // 简单的自检：检查所有通道是否可读
    for (uint8_t ch = 0; ch < 8; ch++) {
        adc_start_conversion();
        while (!adc_conversion_complete_flag) {
            // 等待转换完成
        }
        
        if (adc_raw_values[ch] == 0) {
            return 0; // 自检失败
        }
    }
    
    return 1; // 自检通过
}

// ============================================================================
// 获取函数
// ============================================================================

adc_config_t* adc_get_config(void)
{
    return &adc_config;
}

adc_status_t* adc_get_status(void)
{
    return &adc_status;
}

adc_data_t* adc_get_data(void)
{
    return &adc_data;
}

// ============================================================================
// 调试函数
// ============================================================================

void adc_debug_print(void)
{
    uart_send_string("ADC Debug Info:\r\n");
    uart_send_string("Initialized: ");
    uart_send_int(adc_status.is_initialized);
    uart_send_string("\r\n");
    
    uart_send_string("Converting: ");
    uart_send_int(adc_status.is_converting);
    uart_send_string("\r\n");
    
    uart_send_string("Conversion Count: ");
    uart_send_int(adc_status.conversion_count);
    uart_send_string("\r\n");
    
    uart_send_string("DC Voltage: ");
    uart_send_float(adc_data.dc_voltage);
    uart_send_string(" V\r\n");
    
    uart_send_string("Temperature: ");
    uart_send_float(adc_data.temperature);
    uart_send_string(" C\r\n");
}

void adc_parameter_display(void)
{
    uart_send_string("ADC Parameters:\r\n");
    uart_send_string("Resolution: ");
    uart_send_int(adc_config.resolution);
    uart_send_string(" bits\r\n");
    
    uart_send_string("Reference Voltage: ");
    uart_send_float(adc_config.reference_voltage);
    uart_send_string(" V\r\n");
    
    uart_send_string("Sample Time: ");
    uart_send_int(adc_config.sample_time);
    uart_send_string(" cycles\r\n");
}