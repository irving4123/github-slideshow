#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include "bemf_config.h"

// ADC通道定义
typedef enum {
    ADC_CH_BEMF_U = ADC_BEMF_U,
    ADC_CH_BEMF_V = ADC_BEMF_V,
    ADC_CH_BEMF_W = ADC_BEMF_W,
    ADC_CH_CURRENT = ADC_CURRENT,
    ADC_CH_VOLTAGE = ADC_VOLTAGE,
    ADC_CH_TEMP = ADC_TEMPERATURE
} adc_channel_t;

// 比较器定义
typedef enum {
    COMP_BEMF = COMP0_BEMF,
    COMP_CURRENT = COMP1_CURRENT
} comparator_t;

// ADC初始化和配置
void adc_init(void);
void adc_calibrate(void);
void adc_set_reference(unsigned int ref_voltage_mv);
void adc_set_sampling_rate(unsigned int rate);

// ADC读取函数
unsigned int adc_read_channel(unsigned char channel);
unsigned int adc_read_raw(unsigned char channel);
unsigned int adc_read_voltage_mv(unsigned char channel);
void adc_start_conversion(unsigned char channel);
unsigned char adc_is_conversion_complete(void);

// 多通道ADC处理
void adc_scan_channels(unsigned int* results, unsigned char num_channels);
void adc_start_continuous_scan(void);
void adc_stop_continuous_scan(void);

// BEMF专用ADC函数
unsigned int adc_read_bemf_u(void);
unsigned int adc_read_bemf_v(void);
unsigned int adc_read_bemf_w(void);
unsigned int adc_get_bemf_neutral_point(void);

// 电流和电压监测
unsigned int adc_read_current_ma(void);
unsigned int adc_read_voltage_mv(void);
unsigned int adc_read_temperature_c(void);

// 比较器配置和控制
void comparator_init(void);
void comparator_set_threshold(comparator_t comp, unsigned int threshold_mv);
void comparator_enable(comparator_t comp);
void comparator_disable(comparator_t comp);
unsigned char comparator_get_output(comparator_t comp);

// 比较器中断处理
void comparator_interrupt_handler(void) interrupt 5;

// ADC中断处理
void adc_interrupt_handler(void) interrupt 6;

// 滤波和处理函数
unsigned int adc_moving_average(unsigned char channel, unsigned int new_value);
unsigned int adc_median_filter(unsigned char channel, unsigned int new_value);
void adc_reset_filters(void);

// 全局变量声明
extern volatile unsigned int adc_results[6];
extern volatile unsigned char adc_scan_complete;
extern volatile unsigned int bemf_neutral_voltage;

#endif // ADC_HANDLER_H