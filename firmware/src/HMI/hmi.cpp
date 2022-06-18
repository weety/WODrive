#include <string.h>

#include <hmi.hpp>

HMI::HMI() {
}

void HMI::Setup() {
    lcd.Setup();
    // Form_UpdataThread();
}

/**
 * @brief 修改标题
 * 
 * @param title 
 */
void HMI::Form_setState(const char *str, StateType stat, uint16_t color) {
    if (0 == strcmp(current_state_str, str)) return;
    strcpy(current_state_str, str);
    current_state_str_refresh_flag = true;
    current_state_color = color;
    current_state = stat;
}

void HMI::Form_setLineIndex(int idx) {
    lineIndex = idx;
}

/**
 * @brief 追加一行内容到末尾
 * 
 * @param line 内容
 * @param color 颜色
 */
void HMI::Form_addLine(const char *line, uint16_t color) {
    switch (lineIndex) {
        case 0:
            lineIndex++;
            if (0 == strcmp(line0, line)) break;
            strcpy(line0, line);
            line0_color = color;
            line0_flag = true;
            break;
        case 1:
            lineIndex++;
            if (0 == strcmp(line1, line)) break;
            strcpy(line1, line);
            line1_color = color;
            line1_flag = true;
            break;
        case 2:
            lineIndex++;
            if (0 == strcmp(line2, line)) break;
            strcpy(line2, line);
            line2_color = color;
            line2_flag = true;
            break;
        case 3:
            lineIndex++;
            if (0 == strcmp(line3, line)) break;
            strcpy(line3, line);
            line3_color = color;
            line3_flag = true;
            break;
        case 4:
            if (0 == strcmp(line3, line)) break;
            strcpy(line0, line1);
            strcpy(line1, line2);
            strcpy(line2, line3);
            strcpy(line3, line);
            line0_color = line1_color;
            line1_color = line2_color;
            line2_color = line3_color;
            line3_color = color;
            line0_flag = true;
            line1_flag = true;
            line2_flag = true;
            line3_flag = true;
            break;
        default:
            lineIndex = 0;
            break;
    }
}

/**
 * @brief 设置指定行内容
 * 
 * @param index 行索引
 * @param line 内容
 * @param color 颜色
 */
void HMI::Form_setLine(int index, const char *line, uint16_t color) {
    switch (index) {
        case 0:
            if (0 == strcmp(line0, line)) return;
            strcpy(line0, line);
            line0_color = color;
            line0_flag = true;
            break;
        case 1:
            if (0 == strcmp(line1, line)) return;
            strcpy(line1, line);
            line1_color = color;
            line1_flag = true;
            break;
        case 2:
            if (0 == strcmp(line2, line)) return;
            strcpy(line2, line);
            line2_color = color;
            line2_flag = true;
            break;
        case 3:
            if (0 == strcmp(line3, line)) return;
            strcpy(line3, line);
            line3_color = color;
            line3_flag = true;
            break;
        default:
            break;
    }
    strcpy(line0, line);
    line0_color = color;
}

/**
 * @brief 表格数据更新
 * 
 */
void HMI::Form_Table_Updata(void) {
     // // MOSFET 温度数据
    // lcd.ShowText(178, 32,
    //              lcd.getColor(axes[0]->fet_thermistor_.config_.temp_limit_lower, axes[0]->fet_thermistor_.temperature_),
    //              lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"%.2f", (double)axes[0]->fet_thermistor_.temperature_);
    // 当前温度
    lcd.ShowTextAlignRight(70, 55, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f",
                           (double)axes[0]->fet_thermistor_.temperature_);
    lcd.ShowTextAlignRight(165, 55, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f",
                           (double)axes[1]->fet_thermistor_.temperature_);

    // 当前电流Iq/Id
    lcd.ShowTextAlignRight(70, 75, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.1f/%.1f", (double)axes[0]->motor_.current_control_.Iq_measured, (double)axes[0]->motor_.current_control_.Id_measured);
    lcd.ShowTextAlignRight(165, 75, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.1f/%.1f", (double)axes[1]->motor_.current_control_.Iq_measured, (double)axes[1]->motor_.current_control_.Id_measured);
    // 当前速度
    lcd.ShowTextAlignRight(70, 95,
                           lcd.getColor(axes[0]->controller_.config_.vel_limit, abs(axes[0]->encoder_.vel_estimate_)),
                           lcd.Color.BackColor, lcd.FontSize_16, 0, 65, "%.2f", (double)axes[0]->encoder_.vel_estimate_);
    lcd.ShowTextAlignRight(165, 95,
                           lcd.getColor(axes[1]->controller_.config_.vel_limit, abs(axes[1]->encoder_.vel_estimate_)),
                           lcd.Color.BackColor, lcd.FontSize_16, 0, 65, "%.2f", (double)axes[1]->encoder_.vel_estimate_);                       
    // 当前位置
    lcd.ShowTextAlignRight(70, 115, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f", (double)axes[0]->encoder_.pos_estimate_);
    lcd.ShowTextAlignRight(165, 115, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f", (double)axes[1]->encoder_.pos_estimate_);
    // // 力矩设定值
    // lcd.ShowTextAlignRight(165, 55, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f", (double)axes[0]->controller_.torque_setpoint_);
    // 电流Iq/Id设定值
    // lcd.ShowTextAlignRight(165, 75, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.1f/%.1f", (double)axes[0]->motor_.current_control_.Iq_setpoint, (double)axes[0]->motor_.current_control_.Id_setpoint);
    // // 速度设定值
    // lcd.ShowTextAlignRight(165, 95,
    //                        lcd.getColor(axes[0]->controller_.config_.vel_limit, abs(axes[0]->controller_.input_vel_)),
    //                        lcd.Color.BackColor, lcd.FontSize_16, 0, 65, "%.2f", (double)axes[0]->controller_.input_vel_);  
    // // 位置设定值
    // lcd.ShowTextAlignRight(165, 115, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, 65, " %.2f", (double)axes[0]->controller_.input_pos_);
}

/**
 * @brief  表格加载
 * 
 */
void HMI::Form_Table_Load() {
    // 清屏
    lcd.ShowText(0, 55, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                              ");
    lcd.ShowText(0, 75, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                              ");
    lcd.ShowText(0, 95, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                              ");
    lcd.ShowText(0, 115, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                              ");
    // 绘制表框
    lcd.DrawLine(2, 55, 2, 135, lcd.Color.TableColor);
    lcd.DrawLine(140, 55, 140, 135, lcd.Color.TableColor);
    lcd.DrawLine(238, 55, 238, 135, lcd.Color.TableColor);
    // // 绘制箭头
    // lcd.ShowText(140, 55, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"|");
    // lcd.ShowText(140, 75, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"|");
    // lcd.ShowText(140, 95, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"|");
    // lcd.ShowText(140, 115, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"|");
    // 绘制表项
    lcd.ShowText(10, 55, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Temp :");
    lcd.ShowText(10, 75, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Iq/Id:");
    lcd.ShowText(10, 95, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Vel. :");
    lcd.ShowText(10, 115, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Pos. :");
}

/**
 * @brief 界面数据更新
 * 
 */
void HMI::Form_Updata(void) {
    // 检查系统状态
    if (axes[0]->error_ != Axis::ERROR_NONE) {
        // Form_setLine(0, "", lcd.Color.ErrColor);
        // Form_setLine(1, "", lcd.Color.ErrColor);
        // Form_setLine(2, "", lcd.Color.ErrColor);
        // Form_setLine(3, "", lcd.Color.ErrColor);
        Form_setLineIndex(0);
        if (axes[0]->error_ & Axis::ERROR_INVALID_STATE) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("INVALID_STATE", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_DC_BUS_UNDER_VOLTAGE) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("DC_BUS_UNDER_VOLTAGE", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_DC_BUS_OVER_VOLTAGE) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("DC_BUS_OVER_VOLTAGE", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_CURRENT_MEASUREMENT_TIMEOUT) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("CURRENT_MEASUREMENT_TIMEOUT", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_BRAKE_RESISTOR_DISARMED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("BRAKE_RESISTOR_DISARMED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_MOTOR_DISARMED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("MOTOR_DISARMED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_MOTOR_FAILED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("MOTOR_FAILED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_SENSORLESS_ESTIMATOR_FAILED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("SENSORLESS_ESTIMATOR_FAILED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_ENCODER_FAILED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("ENCODER_FAILED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_CONTROLLER_FAILED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("CONTROLLER_FAILED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_POS_CTRL_DURING_SENSORLESS) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("POS_CTRL_DURING_SENSORLESS", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_WATCHDOG_TIMER_EXPIRED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("WATCHDOG_TIMER_EXPIRED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_MIN_ENDSTOP_PRESSED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("MIN_ENDSTOP_PRESSED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_MAX_ENDSTOP_PRESSED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("MAX_ENDSTOP_PRESSED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_ESTOP_REQUESTED) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("ESTOP_REQUESTED", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_HOMING_WITHOUT_ENDSTOP) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("HOMING_WITHOUT_ENDSTOP", lcd.Color.ErrColor);
        }
        if (axes[0]->error_ & Axis::ERROR_OVER_TEMP) {
            hmi.Form_setState("ERROR", HMI::ERROR, lcd.Color.ErrColor);
            Form_addLine("OVER_TEMP", lcd.Color.ErrColor);
        }
        if (lineIndex <= 1) Form_setLine(1, "                            ", lcd.Color.ErrColor);
        if (lineIndex <= 2) Form_setLine(2, "                            ", lcd.Color.ErrColor);
        if (lineIndex <= 3) Form_setLine(3, "                            ", lcd.Color.ErrColor);
    }

    // 更新状态字符串
    if (current_state_str_refresh_flag == true) {
        current_state_str_refresh_flag = false;
        char line[32];
        sprintf(line, "[%s]", current_state_str);
        lcd.ShowText(0, 0, current_state_color, lcd.Color.BackColor, lcd.FontSize_32, 0, (const char *)"       ");
        lcd.ShowText(0 + (7 - strlen(line)) * 8, 0, current_state_color, lcd.Color.BackColor, lcd.FontSize_32, 0, line);
    }
    // 电压数据
    if ((double)vbus_voltage == 12.00 || (double)vbus_voltage < 5.0) {
        lcd.ShowText(178, 0, lcd.Color.ErrColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"NO-DC", (double)vbus_voltage);
    } else {
        lcd.ShowText(178, 0,
                     lcd.getColor(odrv.config_.dc_bus_undervoltage_trip_level, odrv.config_.dc_bus_overvoltage_trip_level, vbus_voltage),
                     lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"%2.2f ", (double)vbus_voltage);
    }
    // 电流数据
    lcd.ShowText(178, 15,
                 lcd.getColor(odrv.config_.dc_max_positive_current, ibus_),
                 lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"%2.2f ", (double)ibus_);
    // // MOSFET 温度数据
    // lcd.ShowText(178, 32,
    //              lcd.getColor(axes[0]->fet_thermistor_.config_.temp_limit_lower, axes[0]->fet_thermistor_.temperature_),
    //              lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"%.2f", (double)axes[0]->fet_thermistor_.temperature_);
    // 控制模式
    switch (axes[0]->controller_.config_.control_mode) {
        case 0:
            lcd.ShowText(60, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Vol.");
            break;
        case 1:
            lcd.ShowText(60, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Tor.");
            break;
        case 2:
            lcd.ShowText(60, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Vel.");
            break;
        case 3:
            lcd.ShowText(60, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Pos.");
            break;
    }
    switch (axes[1]->controller_.config_.control_mode) {
        case 0:
            lcd.ShowText(180, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Vol.");
            break;
        case 1:
            lcd.ShowText(180, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Tor.");
            break;
        case 2:
            lcd.ShowText(180, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Vel.");
            break;
        case 3:
            lcd.ShowText(180, 30, lcd.Color.TextColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"Pos.");
            break;
    }
    // 更新表格
    switch (current_state) {
        case ERROR:
        case INIT:

            if (line0_flag == true) {
                lcd.ShowText(10, 55, lcd.Color.BackColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                            ");
                lcd.ShowText(10, 55, line0_color, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)line0);
                line0_flag = false;
            }

            if (line1_flag == true) {
                lcd.ShowText(10, 75, lcd.Color.BackColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                            ");
                lcd.ShowText(10, 75, line1_color, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)line1);
                line1_flag = false;
            }

            if (line2_flag == true) {
                lcd.ShowText(10, 95, lcd.Color.BackColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                            ");
                lcd.ShowText(10, 95, line2_color, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)line2);
                line2_flag = false;
            }

            if (line3_flag == true) {
                lcd.ShowText(10, 115, lcd.Color.BackColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"                            ");
                lcd.ShowText(10, 115, line3_color, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)line3);
                line3_flag = false;
            }

            break;
        case READY:
            if (last_state != current_state)
                hmi.Form_Table_Load();
            Form_Table_Updata();
            break;
    }
    last_state = current_state;
}

/**
 * @brief 界面线程加载
 * 
 */
void HMI::Form_Updata_Load() {
    // 绘制表头
    lcd.ShowText(0, 0, lcd.Color.NormColor, lcd.Color.BackColor, lcd.FontSize_32, 0, (const char *)"[     ]");
    lcd.ShowText(125, 0, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Vbus:");
    lcd.ShowText(226, 0, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"V");
    lcd.ShowText(125, 15, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"Ibus:");
    lcd.ShowText(226, 15, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_16, 0, (const char *)"A");
    lcd.ShowText(10, 30, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"M0:[    ]");
    lcd.ShowText(130, 30, lcd.Color.TableColor, lcd.Color.BackColor, lcd.FontSize_24, 0, (const char *)"M1:[    ]");
}

void timer_hmi_update(void* parameter)
{
    HMI *hmi = (HMI*)parameter;
	rt_event_send(&hmi->evt, EVENT_HMI_UPDATE);
}

/**
 * @brief 界面线程循环
 * 
 */
void HMI::Form_Updata_Loop() {
    //portTickType xLastWakeTime = xTaskGetTickCount();
    rt_err_t res;
	rt_uint32_t recv_set = 0;
	rt_uint32_t wait_set = EVENT_HMI_UPDATE;
    rt_event_init(&evt, "HMI_UPDATE", RT_IPC_FLAG_FIFO);
    /* register timer event */
	rt_timer_init(&tim, "hmi_update",
					::timer_hmi_update,
					this,
					200,
					RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_HARD_TIMER);
	rt_timer_start(&tim);
    while (1) {
        //vTaskDelayUntil(&xLastWakeTime, 200);
        /* wait event occur */
		rt_event_recv(&evt, wait_set, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
                        RT_WAITING_FOREVER, &recv_set);
        Form_Updata();
    }
}

/**
 * @brief 连接任务调度器Updata线程和Updata线程循环
 * 
 * @param ctx 对象的指针
 */
static void Updata_loop_link(void *ctx) {
    reinterpret_cast<HMI *>(ctx)->Form_Updata_Loop();
    reinterpret_cast<HMI *>(ctx)->Form_Updata_thread_id_valid_ = false;
}

/**
 * @brief 创建界面刷新线程
 * 
 */
void HMI::Form_UpdataThread() {
    // Form_Updata_Load();
    //osThreadDef(Form_Updata, Updata_loop_link, osPriorityBelowNormal, 0, stack_size_ / sizeof(StackType_t));
    //Form_Updata_thread_id_ = osThreadCreate(osThread(Form_Updata), this);
    Form_Updata_thread_id_ = rt_thread_create("Form_Updata", Updata_loop_link, this, stack_size_, HMI_THREAD_PRIO, 10);
    rt_thread_startup(Form_Updata_thread_id_);
    Form_Updata_thread_id_valid_ = true;
}

/**
 * @brief   使用指令字符串获取参数
 * 
 * @param name  指令字符串
 * @param val   获取到的值
 * @param size  数据最大长度
 */
// #include "autogen/type_info.hpp"
// #include "communication/ascii_protocol.hpp"
// void HMI::GetParameter(const char *name, char *val, int size) {
//     char res[32];
//     Introspectable property = root_obj.get_child(name, strlen(name) + 1);
//     const StringConvertibleTypeInfo *type_info = dynamic_cast<const StringConvertibleTypeInfo *>(property.get_type_info());
//     if (!type_info) {
//         strcpy(val, "");
//     } else {
//         if (!type_info->get_string(property, res, sizeof(res))) {
//             strcpy(val, "");
//         } else {
//             strncpy(val, (const char *)res, strlen(res) > size - 1 ? size - 1 : strlen(res));
//         }
//     }
// }