#ifndef _HMI_HPP_
#define _HMI_HPP_
#include <stdlib.h>

#include "lcd.hpp"
#include "odrive_main.h"

#define EVENT_HMI_UPDATE		(1<<0)

class HMI {
   public:
    enum StateType {
        INIT,
        READY,
        ERROR,
        NO_DC,
        HOMING
    };
    //osThreadId Form_Updata_thread_id_;
    rt_thread_t Form_Updata_thread_id_;
    volatile bool Form_Updata_thread_id_valid_ = false;
    struct rt_event evt;
    struct rt_timer tim;
    HMI(void);
    void Setup(void);
    void Form_setState(const char *str, StateType stat, uint16_t color);
    void Form_setLineIndex(int idx);
    void Form_addLine(const char *line, uint16_t color);
    void Form_setLine(int index, const char *line, uint16_t color);
    void Form_UpdataThread();
    void Form_Updata_Loop();
    void Form_Updata(void);
    void Form_Updata_Load(void);
    void Form_Table_Load(void);
    void Form_Table_Updata(void);

   private:
    const uint32_t stack_size_ = 4096;                        
    StateType current_state;
    StateType last_state;
    char current_state_str[10];
    uint16_t current_state_color;
    volatile bool current_state_str_refresh_flag = true;
    int lineIndex = 0;
    char line0[64];
    char line1[64];
    char line2[64];
    char line3[64];
    uint16_t line0_color;
    uint16_t line1_color;
    uint16_t line2_color;
    uint16_t line3_color;
    bool line0_flag = false;
    bool line1_flag = false;
    bool line2_flag = false;
    bool line3_flag = false;
};

#endif
