/**
 * 电机带载启动演示程序
 * Motor Load Starting Demonstration
 */

class MotorLoadStartingDemo {
    constructor() {
        this.motor = {
            ratedPower: 75, // kW
            ratedVoltage: 380, // V
            ratedCurrent: 140, // A
            ratedSpeed: 1450, // rpm
            loadType: 'variable', // constant, variable, power
            loadInertia: 50 // kg·m²
        };
        
        this.monitoring = {
            current: 0,
            voltage: 0,
            speed: 0,
            torque: 0,
            temperature: 25,
            startTime: null
        };
        
        this.protection = {
            overcurrent: new OvercurrentProtection(),
            overload: new OverloadProtection(),
            overtemperature: new OvertemperatureProtection()
        };
        
        this.isRunning = false;
        this.startMethod = 'soft'; // soft, starDelta, autotransformer
    }

    // 主启动方法
    start(method = 'soft') {
        if (this.isRunning) {
            console.log('电机已在运行中');
            return;
        }

        this.startMethod = method;
        this.isRunning = true;
        this.monitoring.startTime = Date.now();

        console.log(`开始${this.getMethodName(method)}启动`);
        
        // 启动保护监控
        this.startProtectionMonitoring();
        
        // 根据方法选择启动策略
        switch (method) {
            case 'soft':
                this.softStart();
                break;
            case 'starDelta':
                this.starDeltaStart();
                break;
            case 'autotransformer':
                this.autotransformerStart();
                break;
            case 'direct':
                this.directStart();
                break;
            default:
                this.softStart();
        }
    }

    // 软启动
    softStart() {
        const rampTime = 15; // 秒
        const steps = 150; // 步数
        const stepTime = (rampTime * 1000) / steps;
        
        let step = 0;
        const interval = setInterval(() => {
            step++;
            const progress = step / steps;
            
            // 更新电压和频率
            this.monitoring.voltage = this.motor.ratedVoltage * progress;
            this.monitoring.speed = this.motor.ratedSpeed * progress;
            
            // 计算电流（考虑负载特性）
            this.calculateCurrent();
            
            // 更新转矩
            this.calculateTorque();
            
            // 显示状态
            this.displayStatus();
            
            if (step >= steps) {
                clearInterval(interval);
                this.completeStartup();
            }
        }, stepTime);
    }

    // 星三角启动
    starDeltaStart() {
        console.log('第一阶段：星形连接启动');
        
        // 星形阶段（5秒）
        this.starStage();
        
        setTimeout(() => {
            console.log('第二阶段：切换到三角形连接');
            this.transitionStage();
            
            setTimeout(() => {
                console.log('第三阶段：三角形连接运行');
                this.deltaStage();
                this.completeStartup();
            }, 500);
        }, 5000);
    }

    starStage() {
        // 星形连接时电压为线电压的1/√3
        this.monitoring.voltage = this.motor.ratedVoltage / Math.sqrt(3);
        this.monitoring.speed = this.motor.ratedSpeed * 0.3;
        this.calculateCurrent();
        this.calculateTorque();
        this.displayStatus();
    }

    transitionStage() {
        // 切换过程中短暂断电
        this.monitoring.voltage = 0;
        this.monitoring.speed = this.motor.ratedSpeed * 0.25;
        this.monitoring.current = 0;
        this.displayStatus();
    }

    deltaStage() {
        // 三角形连接，全电压运行
        this.monitoring.voltage = this.motor.ratedVoltage;
        this.monitoring.speed = this.motor.ratedSpeed;
        this.calculateCurrent();
        this.calculateTorque();
        this.displayStatus();
    }

    // 自耦变压器启动
    autotransformerStart() {
        const stages = [
            { voltage: 0.65, duration: 3000 },
            { voltage: 0.85, duration: 3000 },
            { voltage: 1.0, duration: 0 }
        ];

        stages.forEach((stage, index) => {
            setTimeout(() => {
                this.monitoring.voltage = this.motor.ratedVoltage * stage.voltage;
                this.monitoring.speed = this.motor.ratedSpeed * stage.voltage;
                this.calculateCurrent();
                this.calculateTorque();
                this.displayStatus();
                
                if (index === stages.length - 1) {
                    this.completeStartup();
                }
            }, this.getStageStartTime(stages, index));
        });
    }

    getStageStartTime(stages, index) {
        return stages
            .slice(0, index)
            .reduce((total, stage) => total + stage.duration, 0);
    }

    // 直接启动
    directStart() {
        console.log('直接启动（不推荐用于大功率电机）');
        
        // 直接启动时电流冲击很大
        this.monitoring.voltage = this.motor.ratedVoltage;
        this.monitoring.current = this.motor.ratedCurrent * 6; // 6倍启动电流
        this.monitoring.speed = this.motor.ratedSpeed;
        this.calculateTorque();
        this.displayStatus();
        
        setTimeout(() => {
            this.monitoring.current = this.motor.ratedCurrent;
            this.displayStatus();
            this.completeStartup();
        }, 2000);
    }

    // 计算电流
    calculateCurrent() {
        const baseCurrent = this.motor.ratedCurrent;
        const speedRatio = this.monitoring.speed / this.motor.ratedSpeed;
        
        if (speedRatio < 0.1) {
            // 启动阶段，电流较大
            this.monitoring.current = baseCurrent * (6 - 5 * speedRatio);
        } else {
            // 正常运行阶段
            this.monitoring.current = baseCurrent * (1 + 0.2 * (1 - speedRatio));
        }
        
        // 考虑负载特性
        if (this.motor.loadType === 'variable') {
            this.monitoring.current *= Math.pow(speedRatio, 2);
        }
    }

    // 计算转矩
    calculateTorque() {
        const ratedTorque = (this.motor.ratedPower * 1000) / (this.motor.ratedSpeed * 2 * Math.PI / 60);
        const speedRatio = this.monitoring.speed / this.motor.ratedSpeed;
        
        if (this.motor.loadType === 'constant') {
            this.monitoring.torque = ratedTorque;
        } else if (this.motor.loadType === 'variable') {
            this.monitoring.torque = ratedTorque * Math.pow(speedRatio, 2);
        } else { // power
            this.monitoring.torque = ratedTorque / speedRatio;
        }
    }

    // 启动保护监控
    startProtectionMonitoring() {
        setInterval(() => {
            if (this.isRunning) {
                // 过流保护
                this.protection.overcurrent.monitor(this.monitoring.current);
                
                // 过载保护
                this.protection.overload.monitor(this.monitoring.current, this.motor.ratedCurrent);
                
                // 过温保护
                this.monitoring.temperature += 0.1; // 模拟温升
                this.protection.overtemperature.monitor(this.monitoring.temperature);
            }
        }, 100);
    }

    // 显示状态
    displayStatus() {
        const runtime = this.monitoring.startTime ? 
            ((Date.now() - this.monitoring.startTime) / 1000).toFixed(1) : 0;
        
        console.log(`
=== 电机运行状态 ===
运行时间: ${runtime}s
电压: ${this.monitoring.voltage.toFixed(1)}V
电流: ${this.monitoring.current.toFixed(1)}A
转速: ${this.monitoring.speed.toFixed(0)}rpm
转矩: ${this.monitoring.torque.toFixed(1)}N·m
温度: ${this.monitoring.temperature.toFixed(1)}°C
启动方式: ${this.getMethodName(this.startMethod)}
========================
        `);
    }

    // 完成启动
    completeStartup() {
        console.log('✅ 电机启动完成，进入正常运行状态');
        this.monitoring.current = this.motor.ratedCurrent;
        this.monitoring.voltage = this.motor.ratedVoltage;
        this.monitoring.speed = this.motor.ratedSpeed;
        this.displayStatus();
    }

    // 停止电机
    stop() {
        if (!this.isRunning) {
            console.log('电机未在运行');
            return;
        }

        console.log('停止电机');
        this.isRunning = false;
        
        // 软停止过程
        let voltage = this.monitoring.voltage;
        const interval = setInterval(() => {
            voltage -= this.motor.ratedVoltage / 50; // 逐步降低电压
            
            if (voltage <= 0) {
                clearInterval(interval);
                this.monitoring.voltage = 0;
                this.monitoring.current = 0;
                this.monitoring.speed = 0;
                console.log('电机已停止');
            } else {
                this.monitoring.voltage = voltage;
                this.calculateCurrent();
                this.calculateTorque();
                this.displayStatus();
            }
        }, 100);
    }

    // 获取启动方式名称
    getMethodName(method) {
        const names = {
            'soft': '软启动',
            'starDelta': '星三角启动',
            'autotransformer': '自耦变压器启动',
            'direct': '直接启动'
        };
        return names[method] || '软启动';
    }

    // 设置负载类型
    setLoadType(type) {
        this.motor.loadType = type;
        const typeNames = {
            'constant': '恒转矩',
            'variable': '变转矩',
            'power': '恒功率'
        };
        console.log(`负载类型设置为: ${typeNames[type]}`);
    }
}

// 过流保护类
class OvercurrentProtection {
    constructor() {
        this.maxCurrent = 200; // A
        this.tripTime = 0.1; // 秒
        this.timer = 0;
    }
    
    monitor(current) {
        if (current > this.maxCurrent) {
            this.timer += 0.1;
            if (this.timer >= this.tripTime) {
                console.log('⚠️ 过流保护动作！');
                this.trip();
            }
        } else {
            this.timer = 0;
        }
    }
    
    trip() {
        console.log('🔴 切断电机电源');
        // 这里应该调用实际的保护动作
    }
}

// 过载保护类
class OverloadProtection {
    constructor() {
        this.overloadThreshold = 1.2; // 120%额定电流
        this.overloadTime = 60; // 秒
        this.timer = 0;
    }
    
    monitor(current, ratedCurrent) {
        if (current > ratedCurrent * this.overloadThreshold) {
            this.timer += 0.1;
            if (this.timer >= this.overloadTime) {
                console.log('⚠️ 过载保护动作！');
                this.trip();
            }
        } else {
            this.timer = 0;
        }
    }
    
    trip() {
        console.log('🔴 过载保护切断电源');
    }
}

// 过温保护类
class OvertemperatureProtection {
    constructor() {
        this.maxTemperature = 120; // °C
    }
    
    monitor(temperature) {
        if (temperature > this.maxTemperature) {
            console.log('⚠️ 过温保护动作！');
            this.trip();
        }
    }
    
    trip() {
        console.log('🔴 过温保护切断电源');
    }
}

// 演示函数
function demonstrateMotorStarting() {
    console.log('🚀 电机带载启动演示程序');
    console.log('================================');
    
    const demo = new MotorLoadStartingDemo();
    
    // 演示不同启动方式
    const methods = ['soft', 'starDelta', 'autotransformer', 'direct'];
    
    methods.forEach((method, index) => {
        setTimeout(() => {
            console.log(`\n📋 演示 ${demo.getMethodName(method)} 方式`);
            demo.start(method);
            
            // 运行一段时间后停止
            setTimeout(() => {
                demo.stop();
            }, 20000);
        }, index * 25000); // 每个演示间隔25秒
    });
}

// 交互式演示
function interactiveDemo() {
    const demo = new MotorLoadStartingDemo();
    
    console.log(`
🎯 电机带载启动交互式演示
可用命令:
- demo.start('soft') - 软启动
- demo.start('starDelta') - 星三角启动  
- demo.start('autotransformer') - 自耦变压器启动
- demo.start('direct') - 直接启动
- demo.stop() - 停止电机
- demo.setLoadType('constant') - 设置恒转矩负载
- demo.setLoadType('variable') - 设置变转矩负载
- demo.setLoadType('power') - 设置恒功率负载
    `);
    
    return demo;
}

// 导出模块
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        MotorLoadStartingDemo,
        OvercurrentProtection,
        OverloadProtection,
        OvertemperatureProtection,
        demonstrateMotorStarting,
        interactiveDemo
    };
}

// 浏览器环境下的全局对象
if (typeof window !== 'undefined') {
    window.MotorLoadStartingDemo = MotorLoadStartingDemo;
    window.demonstrateMotorStarting = demonstrateMotorStarting;
    window.interactiveDemo = interactiveDemo;
}