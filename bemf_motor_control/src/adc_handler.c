#include "../include/adc_handler.h"

// 全局变量
volatile unsigned int adc_results[6] = {0};
volatile unsigned char adc_scan_complete = 0;
volatile unsigned int bemf_neutral_voltage = 2048;  // 默认中点电压

// 静态变量
static unsigned int adc_reference_mv = 3300;  // 3.3V参考电压
static unsigned char current_adc_channel = 0;
static unsigned char continuous_scan_enabled = 0;

// 滤波器数据
#define FILTER_SIZE 8
static unsigned int moving_avg_buffer[6][FILTER_SIZE];
static unsigned char moving_avg_index[6] = {0};
static unsigned int median_buffer[6][5];
static unsigned char median_index[6] = {0};

// ADC初始化
void adc_init(void) {
    // 配置ADC控制寄存器
    // 假设ADC控制寄存器为ADCON0, ADCON1等
    
    // 设置ADC时钟分频 (假设使用系统时钟/16)
    // ADCON1 |= 0x06;  // 设置时钟分频
    
    // 启用ADC模块
    // ADCON0 |= 0x01;  // 启用ADC
    
    // 设置ADC参考电压为VDD
    // ADCON1 &= 0x3F;  // 使用VDD作为参考
    
    // 配置ADC输入通道为模拟输入
    // 这里需要根据具体MCU设置相应的寄存器
    
    // 初始化滤波器缓冲区
    adc_reset_filters();
    
    // 设置默认参考电压
    adc_set_reference(3300);  // 3.3V
    
    // 校准ADC
    adc_calibrate();
    
    // 启用ADC中断
    // EADC = 1;  // 启用ADC中断
}

// ADC校准
void adc_calibrate(void) {
    unsigned int i, sum = 0;
    
    // 读取多次零点电压进行校准
    for(i = 0; i < 16; i++) {
        // 这里可以添加零点校准逻辑
        sum += adc_read_raw(ADC_CH_BEMF_U);
    }
    
    // 计算中性点电压
    bemf_neutral_voltage = sum / 16;
}

// 设置ADC参考电压
void adc_set_reference(unsigned int ref_voltage_mv) {
    adc_reference_mv = ref_voltage_mv;
}

// ADC原始读取
unsigned int adc_read_raw(unsigned char channel) {
    if(channel >= 6) return 0;
    
    // 选择ADC通道
    current_adc_channel = channel;
    
    // 设置通道选择寄存器
    // ADCON0 = (ADCON0 & 0xC3) | (channel << 2);
    
    // 启动转换
    adc_start_conversion(channel);
    
    // 等待转换完成
    while(!adc_is_conversion_complete()) {
        // 等待转换完成
    }
    
    // 读取ADC结果 (假设12位ADC结果在ADRESH:ADRESL中)
    unsigned int result = 0;
    // result = (ADRESH << 8) | ADRESL;
    
    // 模拟ADC读取 (实际应用中需要根据硬件实现)
    result = 2048 + (channel * 100);  // 模拟数据
    
    return result;
}

// ADC通道读取
unsigned int adc_read_channel(unsigned char channel) {
    unsigned int raw_value = adc_read_raw(channel);
    
    // 应用移动平均滤波
    return adc_moving_average(channel, raw_value);
}

// 转换为电压值
unsigned int adc_read_voltage_mv(unsigned char channel) {
    unsigned int adc_value = adc_read_channel(channel);
    
    // 转换为毫伏: (ADC_value * Vref) / 4096
    return (unsigned int)((unsigned long)adc_value * adc_reference_mv / ADC_RESOLUTION);
}

// 启动ADC转换
void adc_start_conversion(unsigned char channel) {
    current_adc_channel = channel;
    
    // 启动转换
    // ADCON0 |= 0x02;  // 设置GO/DONE位启动转换
}

// 检查转换是否完成
unsigned char adc_is_conversion_complete(void) {
    // 检查GO/DONE位
    // return !(ADCON0 & 0x02);
    return 1;  // 模拟转换完成
}

// BEMF专用读取函数
unsigned int adc_read_bemf_u(void) {
    return adc_read_channel(ADC_CH_BEMF_U);
}

unsigned int adc_read_bemf_v(void) {
    return adc_read_channel(ADC_CH_BEMF_V);
}

unsigned int adc_read_bemf_w(void) {
    return adc_read_channel(ADC_CH_BEMF_W);
}

unsigned int adc_get_bemf_neutral_point(void) {
    return bemf_neutral_voltage;
}

// 电流读取 (毫安)
unsigned int adc_read_current_ma(void) {
    unsigned int adc_value = adc_read_channel(ADC_CH_CURRENT);
    
    // 假设电流传感器灵敏度为100mV/A，基准电压为1.65V
    int current_mv = (int)adc_read_voltage_mv(ADC_CH_CURRENT) - 1650;
    if(current_mv < 0) current_mv = -current_mv;  // 取绝对值
    
    // 转换为毫安: mV / (100mV/A) * 1000mA/A
    return (unsigned int)(current_mv * 10);
}

// 电压读取 (毫伏)
unsigned int adc_read_voltage_mv(void) {
    unsigned int adc_value = adc_read_channel(ADC_CH_VOLTAGE);
    
    // 假设电压分压比为1:10
    return adc_read_voltage_mv(ADC_CH_VOLTAGE) * 10;
}

// 温度读取 (摄氏度)
unsigned int adc_read_temperature_c(void) {
    unsigned int adc_value = adc_read_channel(ADC_CH_TEMP);
    
    // 假设温度传感器为10mV/°C，25°C时为750mV
    unsigned int temp_mv = adc_read_voltage_mv(ADC_CH_TEMP);
    
    // 转换为摄氏度
    return (temp_mv - 750) / 10 + 25;
}

// 比较器初始化
void comparator_init(void) {
    // 配置比较器0用于BEMF检测
    // CM0CON0 = 0x86;  // 启用比较器0，输出到内部
    
    // 配置比较器1用于过流保护
    // CM1CON0 = 0x86;  // 启用比较器1，输出到内部
    
    // 设置比较器参考电压
    comparator_set_threshold(COMP_BEMF, bemf_neutral_voltage * adc_reference_mv / ADC_RESOLUTION);
    comparator_set_threshold(COMP_CURRENT, MAX_CURRENT * 100 / 1000);  // 过流阈值
    
    // 启用比较器中断
    // EC0IE = 1;  // 比较器0中断
    // EC1IE = 1;  // 比较器1中断
}

// 设置比较器阈值
void comparator_set_threshold(comparator_t comp, unsigned int threshold_mv) {
    // 计算对应的DAC值
    unsigned char dac_value = (unsigned char)(threshold_mv * 255 / adc_reference_mv);
    
    switch(comp) {
        case COMP_BEMF:
            // 设置比较器0阈值
            // C0VREF = dac_value;
            break;
        case COMP_CURRENT:
            // 设置比较器1阈值
            // C1VREF = dac_value;
            break;
    }
}

// 启用比较器
void comparator_enable(comparator_t comp) {
    switch(comp) {
        case COMP_BEMF:
            // CM0CON0 |= 0x80;  // 启用比较器0
            break;
        case COMP_CURRENT:
            // CM1CON0 |= 0x80;  // 启用比较器1
            break;
    }
}

// 禁用比较器
void comparator_disable(comparator_t comp) {
    switch(comp) {
        case COMP_BEMF:
            // CM0CON0 &= 0x7F;  // 禁用比较器0
            break;
        case COMP_CURRENT:
            // CM1CON0 &= 0x7F;  // 禁用比较器1
            break;
    }
}

// 获取比较器输出
unsigned char comparator_get_output(comparator_t comp) {
    switch(comp) {
        case COMP_BEMF:
            // return (CM0CON0 & 0x40) ? 1 : 0;  // 读取比较器0输出
            return 0;  // 模拟输出
        case COMP_CURRENT:
            // return (CM1CON0 & 0x40) ? 1 : 0;  // 读取比较器1输出
            return 0;  // 模拟输出
    }
    return 0;
}

// 移动平均滤波
unsigned int adc_moving_average(unsigned char channel, unsigned int new_value) {
    if(channel >= 6) return new_value;
    
    // 更新滤波器缓冲区
    moving_avg_buffer[channel][moving_avg_index[channel]] = new_value;
    moving_avg_index[channel] = (moving_avg_index[channel] + 1) % FILTER_SIZE;
    
    // 计算平均值
    unsigned long sum = 0;
    unsigned char i;
    for(i = 0; i < FILTER_SIZE; i++) {
        sum += moving_avg_buffer[channel][i];
    }
    
    return (unsigned int)(sum / FILTER_SIZE);
}

// 中值滤波
unsigned int adc_median_filter(unsigned char channel, unsigned int new_value) {
    if(channel >= 6) return new_value;
    
    // 更新中值滤波缓冲区
    median_buffer[channel][median_index[channel]] = new_value;
    median_index[channel] = (median_index[channel] + 1) % 5;
    
    // 简单的3点中值滤波
    unsigned int a = median_buffer[channel][0];
    unsigned int b = median_buffer[channel][1];
    unsigned int c = median_buffer[channel][2];
    
    if(a > b) {
        if(b > c) return b;
        else if(a > c) return c;
        else return a;
    } else {
        if(a > c) return a;
        else if(b > c) return c;
        else return b;
    }
}

// 重置滤波器
void adc_reset_filters(void) {
    unsigned char ch, i;
    
    for(ch = 0; ch < 6; ch++) {
        moving_avg_index[ch] = 0;
        median_index[ch] = 0;
        
        for(i = 0; i < FILTER_SIZE; i++) {
            moving_avg_buffer[ch][i] = bemf_neutral_voltage;
        }
        
        for(i = 0; i < 5; i++) {
            median_buffer[ch][i] = bemf_neutral_voltage;
        }
    }
}

// 多通道扫描
void adc_scan_channels(unsigned int* results, unsigned char num_channels) {
    unsigned char i;
    
    for(i = 0; i < num_channels && i < 6; i++) {
        results[i] = adc_read_channel(i);
    }
}

// 启动连续扫描
void adc_start_continuous_scan(void) {
    continuous_scan_enabled = 1;
    // 启动定时器触发ADC扫描
}

// 停止连续扫描
void adc_stop_continuous_scan(void) {
    continuous_scan_enabled = 0;
}

// ADC中断处理
void adc_interrupt_handler(void) interrupt 6 {
    // ADC转换完成中断处理
    if(continuous_scan_enabled) {
        adc_results[current_adc_channel] = adc_read_raw(current_adc_channel);
        
        // 切换到下一个通道
        current_adc_channel = (current_adc_channel + 1) % 6;
        adc_start_conversion(current_adc_channel);
        
        if(current_adc_channel == 0) {
            adc_scan_complete = 1;  // 一轮扫描完成
        }
    }
    
    // 清除中断标志
    // ADIF = 0;
}

// 比较器中断处理
void comparator_interrupt_handler(void) interrupt 5 {
    // 比较器中断处理
    
    // 检查比较器0中断 (BEMF)
    // if(C0IF) {
    //     C0IF = 0;  // 清除中断标志
    //     // BEMF过零检测中断处理
    // }
    
    // 检查比较器1中断 (过流)
    // if(C1IF) {
    //     C1IF = 0;  // 清除中断标志
    //     // 过流保护中断处理
    //     pwm_emergency_stop();
    // }
}