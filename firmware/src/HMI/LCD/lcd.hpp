#ifndef _LCD_HPP_
#define _LCD_HPP_
#include <stdlib.h>

#include "lcd_font.h"
#include "lcd_pic.h"
#include "odrive_main.h"
#include "stdarg.h"
#include "stdbool.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "string.h"

extern "C" {
#include "st7789.h"
}

class LCD {
   public:
    LCD(void);
    enum ColorType {
        // RGB565
        WHITE = 0xFFFF,       //白色
        BLACK = 0x0000,       //黑色
        BLUE = 0x001F,        //蓝色
        BRED = 0XF81F,        //色
        GRED = 0XFFE0,        //色
        GBLUE = 0X07FF,       //色
        RED = 0xF800,         //红色
        MAGENTA = 0xF81F,     //色
        GREEN = 0x07E0,       //绿色
        CYAN = 0x7FFF,        //色
        YELLOW = 0xFFE0,      //黄色
        BROWN = 0XBC40,       //棕色
        BRRED = 0XFC07,       //棕红色
        GRAY = 0X8430,        //灰色
        DARKBLUE = 0X01CF,    //深蓝色
        LIGHTBLUE = 0X7D7C,   //浅蓝色
        GRAYBLUE = 0X5458,    //灰蓝色
        LIGHTGREEN = 0X841F,  //浅绿色
        LGRAY = 0XC618,       //浅灰色(PANNEL),窗体背景色
        LGRAYBLUE = 0XA651,   //浅灰蓝色(中间层颜色)
        LBBLUE = 0X2B12       //浅棕蓝色(选择条目的反色)
    };

    typedef struct
    {
        ColorType BackColor;
        ColorType TextColor;
        ColorType ErrColor;
        ColorType WarnColor;
        ColorType NormColor;
        ColorType TableColor;
        ColorType BusyColor;
    } LCDColor_TypeDef;

    LCDColor_TypeDef Color = {LCD::BLACK, LCD::WHITE, LCD::RED, LCD::YELLOW, LCD::GREEN, LCD::GRAY, LCD::YELLOW};

    enum FontSize {
        FontSize_12 = 12,
        FontSize_16 = 16,
        FontSize_24 = 24,
        FontSize_32 = 32
    };

    void Setup(void);
    void ShowTextAlignRight(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode, int totalWidth, const char* str, ...);
    void ShowText(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode, const char* str, ...);   //TODO 拟实现中英文混显
    void Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color);                                //指定区域填充颜色
    void DrawPoint(uint16_t x, uint16_t y, uint16_t color);                                                               //在指定位置画一个点
    void DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);                                    //在指定位置画一条线
    void DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);                               //在指定位置画一个矩形
    void DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);                                                 //在指定位置画一个圆
    void DrawTab(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t row, uint16_t col, uint16_t color);  //画一个表格
    void ShowChinese(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);          //显示汉字串
    void ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]);                       //显示图片
    uint16_t getColor(float floor, float ceiling, float value);
    uint16_t getColor(float ceiling, float value);

   private:
    void ShowChinese12x12(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);  //显示单个12x12汉字
    void ShowChinese16x16(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);  //显示单个16x16汉字
    void ShowChinese24x24(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);  //显示单个24x24汉字
    void ShowChinese32x32(uint16_t x, uint16_t y, uint8_t* s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);  //显示单个32x32汉字
    void ShowChar(uint16_t x, uint16_t y, uint8_t chr, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);         //显示一个字符
    void ShowString(uint16_t x, uint16_t y, const uint8_t* p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode);  //显示字符串
};

extern LCD lcd;

#endif
