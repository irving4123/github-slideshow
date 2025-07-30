#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// MCU 硬件配置
// ============================================================================

// 系统时钟配置
#define SYSTEM_CLOCK_FREQ     24000000    // 24MHz
#define TIMER_CLOCK_DIV       12          // 定时器时钟分频

// 存储器配置
#define FLASH_SIZE            18432       // 18KB FLASH
#define SRAM_SIZE             2048        // 2KB SRAM
#define EEPROM_SIZE           128         // 128B EEPROM

// ADC配置
#define ADC_RESOLUTION        12          // 12位ADC
#define ADC_REF_VOLTAGE       3.3         // 参考电压3.3V
#define ADC_CHANNELS          8           // ADC通道数

// PWM配置
#define PWM_CHANNELS          6           // 6通道PWM
#define PWM_RESOLUTION        16          // 16位PWM
#define PWM_FREQ              20000       // PWM频率20kHz

// 定时器配置
#define TIMER_COUNT           4           // 4个16位定时器
#define TIMER_RESOLUTION      16          // 16位定时器

// UART配置
#define UART_COUNT            2           // 2路UART
#define UART_BAUDRATE         115200      // 波特率

// ============================================================================
// 电机参数配置
// ============================================================================

// 电机基本参数
#define MOTOR_POLE_PAIRS      4           // 极对数
#define MOTOR_RATED_VOLTAGE   24.0        // 额定电压(V)
#define MOTOR_RATED_CURRENT   2.0         // 额定电流(A)
#define MOTOR_RATED_SPEED     3000        // 额定转速(RPM)
#define MOTOR_RATED_POWER     50.0        // 额定功率(W)

// 电机电气参数
#define MOTOR_PHASE_RESISTANCE    0.5     // 相电阻(Ω)
#define MOTOR_PHASE_INDUCTANCE    0.001   // 相电感(H)
#define MOTOR_BEMF_CONSTANT       0.05    // 反电动势常数(V/rpm)

// ============================================================================
// 控制参数配置
// ============================================================================

// BEMF检测参数
#define BEMF_DETECTION_THRESHOLD  0.1     // BEMF检测阈值(V)
#define BEMF_FILTER_TIME_CONST   0.001    // BEMF滤波时间常数(s)
#define BEMF_DETECTION_DELAY     0.0001   // BEMF检测延迟(s)

// 启动参数
#define STARTUP_FREQ_INIT     1.0         // 初始启动频率(Hz)
#define STARTUP_FREQ_RAMP     10.0        // 启动频率斜坡(Hz/s)
#define STARTUP_DURATION      2.0         // 启动持续时间(s)
#define STARTUP_CURRENT_LIMIT 1.5         // 启动电流限制(A)

// 闭环控制参数
#define SPEED_CONTROL_KP      0.5         // 速度控制比例系数
#define SPEED_CONTROL_KI      0.1         // 速度控制积分系数
#define SPEED_CONTROL_KD      0.01        // 速度控制微分系数

#define CURRENT_CONTROL_KP     10.0        // 电流控制比例系数
#define CURRENT_CONTROL_KI     100.0       // 电流控制积分系数
#define CURRENT_CONTROL_KD     0.1         // 电流控制微分系数

// 保护参数
#define OVER_CURRENT_THRESHOLD    3.0     // 过流阈值(A)
#define OVER_VOLTAGE_THRESHOLD    30.0    // 过压阈值(V)
#define UNDER_VOLTAGE_THRESHOLD   10.0    // 欠压阈值(V)
#define OVER_TEMP_THRESHOLD       85.0    // 过温阈值(°C)

// ============================================================================
// 算法参数配置
// ============================================================================

// 位置估算参数
#define POSITION_ESTIMATION_GAIN  1.0     // 位置估算增益
#define POSITION_FILTER_ALPHA     0.95    // 位置滤波系数

// 速度估算参数
#define SPEED_ESTIMATION_GAIN     1.0     // 速度估算增益
#define SPEED_FILTER_ALPHA        0.9     // 速度滤波系数

// 电流检测参数
#define CURRENT_SENSOR_GAIN       10.0    // 电流传感器增益
#define CURRENT_OFFSET_COMP       0.0     // 电流偏移补偿

// ============================================================================
// 通信参数配置
// ============================================================================

// UART通信参数
#define UART_BUFFER_SIZE      128         // UART缓冲区大小
#define UART_TIMEOUT_MS       100         // UART超时时间(ms)

// 调试参数
#define DEBUG_ENABLE          1           // 启用调试
#define DEBUG_UART_PORT       0           // 调试UART端口
#define DEBUG_LEVEL           2           // 调试级别(0-3)

// ============================================================================
// 系统参数配置
// ============================================================================

// 任务调度参数
#define TASK_PRIORITY_HIGH   3           // 高优先级任务
#define TASK_PRIORITY_MED    2           // 中优先级任务
#define TASK_PRIORITY_LOW    1           // 低优先级任务

// 中断优先级
#define INT_PRIORITY_PWM     0           // PWM中断优先级
#define INT_PRIORITY_ADC     1           // ADC中断优先级
#define INT_PRIORITY_TIMER   2           // 定时器中断优先级
#define INT_PRIORITY_UART    3           // UART中断优先级

// 系统状态定义
typedef enum {
    SYSTEM_STATE_INIT = 0,       // 初始化状态
    SYSTEM_STATE_STANDBY,        // 待机状态
    SYSTEM_STATE_STARTUP,        // 启动状态
    SYSTEM_STATE_RUNNING,        // 运行状态
    SYSTEM_STATE_STOP,           // 停止状态
    SYSTEM_STATE_FAULT           // 故障状态
} system_state_t;

// 故障代码定义
typedef enum {
    FAULT_NONE = 0,              // 无故障
    FAULT_OVER_CURRENT,          // 过流故障
    FAULT_OVER_VOLTAGE,          // 过压故障
    FAULT_UNDER_VOLTAGE,         // 欠压故障
    FAULT_OVER_TEMP,             // 过温故障
    FAULT_BEMF_DETECTION,        // BEMF检测故障
    FAULT_COMMUNICATION          // 通信故障
} fault_code_t;

#endif // CONFIG_H