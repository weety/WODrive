
#define __MAIN_CPP__
#include <communication/interface_i2c.h>
#include <communication/interface_uart.h>
#include <communication/interface_usb.h>

#include <communication/interface_can.hpp>

//#include "freertos_vars.h"
#include <rthw.h>
#include "nvm_config.hpp"
#include "odrive_main.h"
#include "usart.h"

ODriveCAN::Config_t can_config;
Encoder::Config_t encoder_configs[AXIS_COUNT];
SensorlessEstimator::Config_t sensorless_configs[AXIS_COUNT];
Controller::Config_t controller_configs[AXIS_COUNT];
Motor::Config_t motor_configs[AXIS_COUNT];
OnboardThermistorCurrentLimiter::Config_t fet_thermistor_configs[AXIS_COUNT];
OffboardThermistorCurrentLimiter::Config_t motor_thermistor_configs[AXIS_COUNT];
Axis::Config_t axis_configs[AXIS_COUNT];
TrapezoidalTrajectory::Config_t trap_configs[AXIS_COUNT];
Endstop::Config_t min_endstop_configs[AXIS_COUNT];
Endstop::Config_t max_endstop_configs[AXIS_COUNT];

std::array<Axis *, AXIS_COUNT> axes;
ODriveCAN *odCAN = nullptr;
ODrive odrv{};
HMI hmi{};  

typedef Config<
    BoardConfig_t,
    ODriveCAN::Config_t,
    Encoder::Config_t[AXIS_COUNT],
    SensorlessEstimator::Config_t[AXIS_COUNT],
    Controller::Config_t[AXIS_COUNT],
    Motor::Config_t[AXIS_COUNT],
    OnboardThermistorCurrentLimiter::Config_t[AXIS_COUNT],
    OffboardThermistorCurrentLimiter::Config_t[AXIS_COUNT],
    TrapezoidalTrajectory::Config_t[AXIS_COUNT],
    Endstop::Config_t[AXIS_COUNT],
    Endstop::Config_t[AXIS_COUNT],
    Axis::Config_t[AXIS_COUNT]>ConfigFormat;

void ODrive::save_configuration(void) {
    if (ConfigFormat::safe_store_config(
            &odrv.config_,
            &can_config,
            &encoder_configs,
            &sensorless_configs,
            &controller_configs,
            &motor_configs,
            &fet_thermistor_configs,
            &motor_thermistor_configs,
            &trap_configs,
            &min_endstop_configs,
            &max_endstop_configs,
            &axis_configs)) {
        printf("saving configuration failed\r\n");
        //osDelay(5);
        rt_thread_mdelay(5);
    } else {
        odrv.user_config_loaded_ = true;
    }
}

extern "C" int load_configuration(void) {
    // Try to load configs
    if (NVM_init() ||
        ConfigFormat::safe_load_config(
            &odrv.config_,
            &can_config,
            &encoder_configs,
            &sensorless_configs,
            &controller_configs,
            &motor_configs,
            &fet_thermistor_configs,
            &motor_thermistor_configs,
            &trap_configs,
            &min_endstop_configs,
            &max_endstop_configs,
            &axis_configs)) {
        //If loading failed, restore defaults
        odrv.config_ = BoardConfig_t();
        can_config = ODriveCAN::Config_t();
        for (size_t i = 0; i < AXIS_COUNT; ++i) {
            encoder_configs[i] = Encoder::Config_t();
            sensorless_configs[i] = SensorlessEstimator::Config_t();
            controller_configs[i] = Controller::Config_t();
            motor_configs[i] = Motor::Config_t();
            fet_thermistor_configs[i] = OnboardThermistorCurrentLimiter::Config_t();
            motor_thermistor_configs[i] = OffboardThermistorCurrentLimiter::Config_t();
            trap_configs[i] = TrapezoidalTrajectory::Config_t();
            axis_configs[i] = Axis::Config_t();
            // Default step/dir pins are different, so we need to explicitly load them
            Axis::load_default_step_dir_pin_config(hw_configs[i].axis_config, &axis_configs[i]);
            Axis::load_default_can_id(i, axis_configs[i]);
            min_endstop_configs[i] = Endstop::Config_t();
            max_endstop_configs[i] = Endstop::Config_t();
            controller_configs[i].load_encoder_axis = i;
        }
    } else {
        odrv.user_config_loaded_ = true;
    }
    return odrv.user_config_loaded_;
}

void ODrive::erase_configuration(void) {
    NVM_erase();

    // FIXME: this reboot is a workaround because we don't want the next save_configuration
    // to write back the old configuration from RAM to NVM. The proper action would
    // be to reset the values in RAM to default. However right now that's not
    // practical because several startup actions depend on the config. The
    // other problem is that the stack overflows if we reset to default here.
    NVIC_SystemReset();
}

void ODrive::enter_dfu_mode() {
    if ((hw_version_major_ == 3) && (hw_version_minor_ >= 5)) {
        __asm volatile("CPSID I\n\t" ::: "memory");  // disable interrupts
        _reboot_cookie = 0xDEADBEEF;
        NVIC_SystemReset();
    } else {
        /*
        * DFU mode is only allowed on board version >= 3.5 because it can burn
        * the brake resistor FETs on older boards.
        * If you really want to use it on an older board, add 3.3k pull-down resistors
        * to the AUX_L and AUX_H signals and _only then_ uncomment these lines.
        */
        //__asm volatile ("CPSID I\n\t":::"memory"); // disable interrupts
        //_reboot_cookie = 0xDEADFE75;
        //NVIC_SystemReset();
    }
}

extern "C" int construct_objects() {
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 3
    if (odrv.config_.enable_i2c_instead_of_can) {
        // Set up the direction GPIO as input
        GPIO_InitTypeDef GPIO_InitStruct;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;

        GPIO_InitStruct.Pin = I2C_A0_PIN;
        HAL_GPIO_Init(I2C_A0_PORT, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = I2C_A1_PIN;
        HAL_GPIO_Init(I2C_A1_PORT, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = I2C_A2_PIN;
        HAL_GPIO_Init(I2C_A2_PORT, &GPIO_InitStruct);

        //osDelay(1);
        rt_thread_mdelay(1);
    #ifdef USE_I2C_INTERFACE
        i2c_stats_.addr = (0xD << 3);
        i2c_stats_.addr |= HAL_GPIO_ReadPin(I2C_A0_PORT, I2C_A0_PIN) != GPIO_PIN_RESET ? 0x1 : 0;
        i2c_stats_.addr |= HAL_GPIO_ReadPin(I2C_A1_PORT, I2C_A1_PIN) != GPIO_PIN_RESET ? 0x2 : 0;
        i2c_stats_.addr |= HAL_GPIO_ReadPin(I2C_A2_PORT, I2C_A2_PIN) != GPIO_PIN_RESET ? 0x4 : 0;
        MX_I2C1_Init(i2c_stats_.addr);
    #endif
    } else
#endif
    #ifdef USE_CAN_INTERFACE
        MX_CAN1_Init();
    #endif

    //HAL_UART_DeInit(&huart4);
    //huart4.Init.BaudRate = odrv.config_.uart_baudrate;
    //HAL_UART_Init(&huart4);
    uart_intf_init(odrv.config_.uart_baudrate);

    // Init general user ADC on some GPIOs.
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Pin = GPIO_1_Pin;
    HAL_GPIO_Init(GPIO_1_GPIO_Port, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_2_Pin;
    HAL_GPIO_Init(GPIO_2_GPIO_Port, &GPIO_InitStruct);
    // MING DEL GPIO3&4
    // GPIO_InitStruct.Pin = GPIO_3_Pin;
    // HAL_GPIO_Init(GPIO_3_GPIO_Port, &GPIO_InitStruct);
    // GPIO_InitStruct.Pin = GPIO_4_Pin;
    // HAL_GPIO_Init(GPIO_4_GPIO_Port, &GPIO_InitStruct);
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 5
    GPIO_InitStruct.Pin = GPIO_5_Pin;
    HAL_GPIO_Init(GPIO_5_GPIO_Port, &GPIO_InitStruct);
#endif

    // Construct all objects.
    odCAN = new ODriveCAN(can_config, &hcan1);
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        Encoder *encoder = new Encoder(hw_configs[i].encoder_config,
                                       encoder_configs[i], motor_configs[i]);
        SensorlessEstimator *sensorless_estimator = new SensorlessEstimator(sensorless_configs[i]);
        Controller *controller = new Controller(controller_configs[i]);

        OnboardThermistorCurrentLimiter *fet_thermistor = new OnboardThermistorCurrentLimiter(hw_configs[i].thermistor_config,
                                                                                              fet_thermistor_configs[i]);
        OffboardThermistorCurrentLimiter *motor_thermistor = new OffboardThermistorCurrentLimiter(motor_thermistor_configs[i]);

        Motor *motor = new Motor(hw_configs[i].motor_config,
                                 hw_configs[i].gate_driver_config,
                                 motor_configs[i]);
        TrapezoidalTrajectory *trap = new TrapezoidalTrajectory(trap_configs[i]);
        Endstop *min_endstop = new Endstop(min_endstop_configs[i]);
        Endstop *max_endstop = new Endstop(max_endstop_configs[i]);
        axes[i] = new Axis(i, hw_configs[i].axis_config, axis_configs[i],
                           *encoder, *sensorless_estimator, *controller, *fet_thermistor,
                           *motor_thermistor, *motor, *trap, *min_endstop, *max_endstop);

        controller_configs[i].parent = controller;
        encoder_configs[i].parent = encoder;
        motor_thermistor_configs[i].parent = motor_thermistor;
        motor_configs[i].parent = motor;
        min_endstop_configs[i].parent = min_endstop;
        max_endstop_configs[i].parent = max_endstop;
        axis_configs[i].parent = axes[i];
    }
    return 0;
}

extern "C" {
int odrive_main(void);
/*void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed portCHAR *pcTaskName) {
    for (auto &axis : axes) {
        safety_critical_disarm_motor_pwm(axis->motor_);
    }
    safety_critical_disarm_brake_resistor();
    for (;;)
        ;  // TODO: safe action
}*/

void usr_contex_scheduler_hook(struct rt_thread *from, struct rt_thread *to)
{
    RT_ASSERT(to != RT_NULL);

#ifdef ARCH_CPU_STACK_GROWS_UPWARD
    if (*((rt_uint8_t *)((rt_ubase_t)to->stack_addr + to->stack_size - 1)) != '#' ||
#else
    if (*((rt_uint8_t *)to->stack_addr) != '#' ||
#endif /* ARCH_CPU_STACK_GROWS_UPWARD */
        (rt_ubase_t)to->sp <= (rt_ubase_t)to->stack_addr ||
        (rt_ubase_t)to->sp > (rt_ubase_t)to->stack_addr + (rt_ubase_t)to->stack_size)
    {
        rt_ubase_t level;

        for (auto &axis : axes) {
            safety_critical_disarm_motor_pwm(axis->motor_);
        }
        safety_critical_disarm_brake_resistor();
        rt_kprintf("thread:%s stack overflow\n", to->name);

        level = rt_hw_interrupt_disable();
        while (level);
    }
}

/*void vApplicationIdleHook(void) {
    if (odrv.system_stats_.fully_booted) {
        odrv.system_stats_.uptime = xTaskGetTickCount();
        odrv.system_stats_.min_heap_space = xPortGetMinimumEverFreeHeapSize();
        odrv.system_stats_.min_stack_space_comms = uxTaskGetStackHighWaterMark(comm_thread) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_axis0 = uxTaskGetStackHighWaterMark(axes[0]->thread_id_) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_axis1 = uxTaskGetStackHighWaterMark(axes[1]->thread_id_) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_usb = uxTaskGetStackHighWaterMark(usb_thread) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_uart = uxTaskGetStackHighWaterMark(uart_thread) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_usb_irq = uxTaskGetStackHighWaterMark(usb_irq_thread) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_startup = uxTaskGetStackHighWaterMark(defaultTaskHandle) * sizeof(StackType_t);
        odrv.system_stats_.min_stack_space_can = uxTaskGetStackHighWaterMark(odCAN->thread_id_) * sizeof(StackType_t);

        // Actual usage, in bytes, so we don't have to math
        odrv.system_stats_.stack_usage_axis0 = axes[0]->stack_size_ - odrv.system_stats_.min_stack_space_axis0;
        odrv.system_stats_.stack_usage_axis1 = axes[1]->stack_size_ - odrv.system_stats_.min_stack_space_axis1;
        odrv.system_stats_.stack_usage_comms = stack_size_comm_thread - odrv.system_stats_.min_stack_space_comms;
        odrv.system_stats_.stack_usage_usb = stack_size_usb_thread - odrv.system_stats_.min_stack_space_usb;
        odrv.system_stats_.stack_usage_uart = stack_size_uart_thread - odrv.system_stats_.min_stack_space_uart;
        odrv.system_stats_.stack_usage_usb_irq = stack_size_usb_irq_thread - odrv.system_stats_.min_stack_space_usb_irq;
        odrv.system_stats_.stack_usage_startup = stack_size_default_task - odrv.system_stats_.min_stack_space_startup;
        odrv.system_stats_.stack_usage_can = odCAN->stack_size_ - odrv.system_stats_.min_stack_space_can;
    }
}*/

rt_ubase_t rt_thread_get_stack_max_used(rt_thread_t thread)
{
    rt_ubase_t level;
    rt_uint8_t *ptr;
#if defined(ARCH_CPU_STACK_GROWS_UPWARD)
    ptr = (rt_uint8_t *)thread->stack_addr + thread->stack_size - 1;
    while (*ptr == '#')ptr --;

    return (rt_ubase_t)ptr - (rt_ubase_t)thread->stack_addr;
#else
    ptr = (rt_uint8_t *)thread->stack_addr;
    while (*ptr == '#')ptr ++;

    return thread->stack_size - ((rt_ubase_t) ptr - (rt_ubase_t) thread->stack_addr);
#endif
}

rt_uint32_t rt_thread_get_minimum_free_heap_size(void)
{
    rt_uint32_t total;
    rt_uint32_t used;
    rt_uint32_t max_used;
    rt_memory_info(&total,
                    &used,
                    &max_used);
    return total - max_used;
}

void usr_idle_hook(void) {
    if (odrv.system_stats_.fully_booted) {
        odrv.system_stats_.uptime = rt_tick_get();
        odrv.system_stats_.min_heap_space = rt_thread_get_minimum_free_heap_size();
        odrv.system_stats_.min_stack_space_comms = rt_thread_get_stack_max_used(comm_thread);
        odrv.system_stats_.min_stack_space_axis0 = rt_thread_get_stack_max_used(axes[0]->thread_id_);
        odrv.system_stats_.min_stack_space_axis1 = rt_thread_get_stack_max_used(axes[1]->thread_id_);
        odrv.system_stats_.min_stack_space_usb = rt_thread_get_stack_max_used(usb_thread);
        odrv.system_stats_.min_stack_space_uart = rt_thread_get_stack_max_used(uart_thread);
        odrv.system_stats_.min_stack_space_usb_irq = rt_thread_get_stack_max_used(usb_irq_thread);
        odrv.system_stats_.min_stack_space_startup = rt_thread_get_stack_max_used(defaultTaskHandle);
        odrv.system_stats_.min_stack_space_can = rt_thread_get_stack_max_used(odCAN->thread_id_);

        // Actual usage, in bytes, so we don't have to math
        odrv.system_stats_.stack_usage_axis0 = axes[0]->stack_size_ - odrv.system_stats_.min_stack_space_axis0;
        odrv.system_stats_.stack_usage_axis1 = axes[1]->stack_size_ - odrv.system_stats_.min_stack_space_axis1;
        odrv.system_stats_.stack_usage_comms = stack_size_comm_thread - odrv.system_stats_.min_stack_space_comms;
        odrv.system_stats_.stack_usage_usb = stack_size_usb_thread - odrv.system_stats_.min_stack_space_usb;
        odrv.system_stats_.stack_usage_uart = stack_size_uart_thread - odrv.system_stats_.min_stack_space_uart;
        odrv.system_stats_.stack_usage_usb_irq = stack_size_usb_irq_thread - odrv.system_stats_.min_stack_space_usb_irq;
        odrv.system_stats_.stack_usage_startup = stack_size_default_task - odrv.system_stats_.min_stack_space_startup;
        odrv.system_stats_.stack_usage_can = odCAN->stack_size_ - odrv.system_stats_.min_stack_space_can;
    }
}

}

int odrive_main(void) {

    rt_thread_idle_sethook(usr_idle_hook);
    rt_scheduler_sethook(usr_contex_scheduler_hook);
    // Setup HMI
    hmi.Form_setState("INIT", HMI::INIT, lcd.Color.BusyColor);
    hmi.Setup();
    hmi.Form_Updata_Load();

    // Start ADC for temperature measurements and user measurements
    hmi.Form_addLine("start general purpose adc", lcd.Color.TextColor);
    hmi.Form_Updata();
    start_general_purpose_adc();

    // TODO: make dynamically reconfigurable
#if HW_VERSION_MAJOR == 3 && HW_VERSION_MINOR >= 3
    if (odrv.config_.enable_uart) {
        hmi.Form_addLine("enable uart", lcd.Color.TextColor);
        hmi.Form_Updata();
        SetGPIO12toUART();
    }
#endif
    //osDelay(100);
    // Init communications (this requires the axis objects to be constructed)
    hmi.Form_addLine("init communication", lcd.Color.TextColor);
    hmi.Form_Updata();
    init_communication();

    // Start pwm-in compare modules
    // must happen after communication is initialized
    hmi.Form_addLine("pwm in init", lcd.Color.TextColor);
    hmi.Form_Updata();
    pwm_in_init();

    // Set up the CS pins for absolute encoders
    hmi.Form_addLine("abs spi cs pin init", lcd.Color.TextColor);
    hmi.Form_Updata();
    for (auto &axis : axes) {
        if (axis->encoder_.config_.mode & Encoder::MODE_FLAG_ABS) {
            axis->encoder_.abs_spi_cs_pin_init();
        }
    }

    // Setup motors (DRV8301 SPI transactions here)
    hmi.Form_addLine("motor setup", lcd.Color.TextColor);
    hmi.Form_Updata();
    // axes[0]->motor_.setup();
    for (auto &axis : axes) {
        axis->motor_.setup();
    }

    // Setup encoders (Starts encoder SPI transactions)

    hmi.Form_addLine("encoder setup", lcd.Color.TextColor);
    hmi.Form_Updata();
    for (auto &axis : axes) {
        axis->encoder_.setup();
    }

    // Setup anything remaining in each axis
    for (auto &axis : axes) {
        axis->setup();
    }

    // Start PWM and enable adc interrupts/callbacks
    hmi.Form_addLine("start adc pwm", lcd.Color.TextColor);
    hmi.Form_Updata();
    start_adc_pwm();

    // This delay serves two purposes:
    //  - Let the current sense calibration converge (the current
    //    sense interrupts are firing in background by now)
    //  - Allow a user to interrupt the code, e.g. by flashing a new code,
    //    before it does anything crazy
    // TODO make timing a function of calibration filter tau
    //osDelay(1500);
    rt_thread_mdelay(1500);

    // Start state machine threads. Each thread will go through various calibration
    // procedures and then run the actual controller loops.
    // TODO: generalize for AXIS_COUNT != 2
    hmi.Form_addLine("start thread", lcd.Color.TextColor);
    hmi.Form_Updata();
    for (size_t i = 0; i < AXIS_COUNT; ++i) {
        axes[i]->start_thread();
    }

    hmi.Form_addLine("start analog thread", lcd.Color.TextColor);
    hmi.Form_Updata();
    start_analog_thread();

    // Draw Table
    hmi.Form_setState("READY", HMI::READY, lcd.Color.NormColor);
    hmi.Form_Updata();
    //hmi.Form_Table_Load();
    hmi.Form_UpdataThread();

    odrv.system_stats_.fully_booted = true;
    return 0;
}
