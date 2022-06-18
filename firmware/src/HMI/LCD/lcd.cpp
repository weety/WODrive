#include <lcd.hpp>

#include "..\communication\interface_uart.h"

LCD::LCD() {
    LCD_TypeDef LCD_InitStruct = {
        .SCLK_GPIO_Port = SRC_SCL_GPIO_Port,
        .MOSI_GPIO_Port = SRC_SDA_GPIO_Port,
        .RES_GPIO_Port = SRC_RES_GPIO_Port,
        .DC_GPIO_Port = SRC_DC_GPIO_Port,
        .CS_GPIO_Port = SRC_CS_GPIO_Port,
        .BLK_GPIO_Port = SRC_BL_GPIO_Port,
        .SCLK_GPIO_Pin = SRC_SCL_Pin,
        .MOSI_GPIO_Pin = SRC_SDA_Pin,
        .RES_GPIO_Pin = SRC_RES_Pin,
        .DC_GPIO_Pin = SRC_DC_Pin,
        .CS_GPIO_Pin = SRC_CS_Pin,
        .BLK_GPIO_Pin = SRC_BL_Pin,
        .WritePin = HAL_GPIO_WritePin,
    };

    LCD_RegisterIO(&LCD_InitStruct);
}

/**
 * @brief LCD初始化
 * 
 */
void LCD::Setup() {
    // 两次初始化确保屏幕初始化正常
    LCD_Init();
    LCD_Init();
    Fill(0, 0, LCD_W, LCD_H, Color.BackColor);
    // // 创建线程
    // StartThread();
}

/**
 * @brief 颜色插值，蓝到绿到红线性插值
 * 
 * @param floor     下限
 * @param ceiling   上限
 * @param value     输入值
 * @return uint16_t RGB565值
 */
uint16_t LCD::getColor(float floor, float ceiling, float value) {
    if (value >= ceiling) return 0xF800;
    if (value <= floor) return 0x001F;
    double rate = ((double)value - (double)floor) / ((double)ceiling - (double)floor);

    if (rate > 0.5) {
        int R_val = (int)((rate - 0.5) * 2 * 31);
        int G_val = (int)((1 - (rate - 0.5) * 2) * 63);
        int B_val = 0;

        return R_val << 11 | G_val << 5 | B_val;
    } else {
        int R_val = 0;
        int G_val = (int)(rate * 2 * 63);
        int B_val = (int)((1 - rate * 2) * 31);

        return R_val << 11 | G_val << 5 | B_val;
    }
}

/**
 * @brief   颜色插值，绿到红线性插值
 * 
 * @param ceiling   上限
 * @param value     输入值
 * @return uint16_t RGB565值
 */
uint16_t LCD::getColor(float ceiling, float value) {
    if (value >= ceiling) return 0xF800;
    if (value <= 0) return 0x07E0;
    float rate = value / ceiling;

    int R_val = (int)(rate * 31);
    int G_val = (int)((1 - rate) * 63);
    int B_val = 0;

    return R_val << 11 | G_val << 5 | B_val;
}

/**
 * @brief   在指定区域填充颜色
 * 
 * @param xsta  起始坐标
 * @param ysta  起始坐标
 * @param xend  终止坐标
 * @param yend  终止坐标
 * @param color 要填充的颜色
 */
void LCD::Fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color) {
    uint16_t i, j;
    LCD_SetAddress(xsta, ysta, xend - 1, yend - 1);  //设置显示范围
    for (i = ysta; i < yend; i++) {
        for (j = xsta; j < xend; j++) {
            LCD_WR_DATA16(color);
        }
    }
}

/**
 * @brief   在指定位置画点
 * 
 * @param x     画点坐标
 * @param y     画点坐标
 * @param color 点颜色
 */
void LCD::DrawPoint(uint16_t x, uint16_t y, uint16_t color) {
    LCD_SetAddress(x, y, x, y);  //设置光标位置
    LCD_WR_DATA16(color);
}

/**
 * @brief   画线
 * 
 * @param x1    起始坐标x
 * @param y1    起始坐标y
 * @param x2    结束坐标x
 * @param y2    结束坐标y
 * @param color 颜色
 */
void LCD::DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    uint16_t t;
    int xerr = 0, yerr = 0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;
    delta_x = x2 - x1;  //计算坐标增量
    delta_y = y2 - y1;
    uRow = x1;  //画线起点坐标
    uCol = y1;
    if (delta_x > 0) {
        incx = 1;
    }  //设置单步方向
    else if (delta_x == 0) {
        incx = 0;
    }  //垂直线
    else {
        incx = -1;
        delta_x = -delta_x;
    }
    if (delta_y > 0) {
        incy = 1;
    } else if (delta_y == 0) {
        incy = 0;
    }  //水平线
    else {
        incy = -1;
        delta_y = -delta_y;
    }
    if (delta_x > delta_y) {
        distance = delta_x;
    }  //选取基本增量坐标轴
    else {
        distance = delta_y;
    }
    for (t = 0; t < distance + 1; t++) {
        DrawPoint(uRow, uCol, color);  //画点
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance) {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance) {
            yerr -= distance;
            uCol += incy;
        }
    }
}

/**
 * @brief   绘制表格
 * 
 * @param x0        起始坐标x
 * @param y0        起始坐标y
 * @param width     宽度
 * @param height    高度
 * @param row       行高
 * @param col       列宽
 * @param color     颜色
 */
void LCD::DrawTab(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t row, uint16_t col, uint16_t color) {
    int i, j;
    for (i = 0; i <= row; i++) {
        DrawLine(
            x0,
            y0 + height * i / row,
            x0 + width,
            y0 + height * i / row,
            color);
    }
    for (j = 0; j <= col; j++) {
        DrawLine(
            x0 + width * j / col,
            y0,
            x0 + width * j / col,
            y0 + height,
            color);
    }
}

/**
 * @brief   画矩形
 * 
 * @param x1    起始坐标
 * @param y1    起始坐标
 * @param x2    终止坐标
 * @param y2    终止坐标
 * @param color 矩形的颜色
 */
void LCD::DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    DrawLine(x1, y1, x2, y1, color);
    DrawLine(x1, y1, x1, y2, color);
    DrawLine(x1, y2, x2, y2, color);
    DrawLine(x2, y1, x2, y2, color);
}

/**
 * @brief   画圆
 * 
 * @param x0    圆心坐标
 * @param y0    圆心坐标
 * @param r     半径
 * @param color 圆的颜色
 */
void LCD::DrawCircle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color) {
    int a, b;
    a = 0;
    b = r;
    while (a <= b) {
        DrawPoint(x0 - b, y0 - a, color);  //3
        DrawPoint(x0 + b, y0 - a, color);  //0
        DrawPoint(x0 - a, y0 + b, color);  //1
        DrawPoint(x0 - a, y0 - b, color);  //2
        DrawPoint(x0 + b, y0 + a, color);  //4
        DrawPoint(x0 + a, y0 - b, color);  //5
        DrawPoint(x0 + a, y0 + b, color);  //6
        DrawPoint(x0 - b, y0 + a, color);  //7
        a++;
        if ((a * a + b * b) > (r * r))  //判断要画的点是否过远
        {
            b--;
        }
    }
}

/**
 * @brief   显示汉字串
 * 
 * @param x     显示坐标
 * @param y     显示坐标
 * @param s     要显示的汉字串
 * @param fc    字的颜色
 * @param bc    字的背景色
 * @param sizey 字号
 * @param mode  0非叠加模式  1叠加模式
 */
void LCD::ShowChinese(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    while (*s != 0) {
        if (sizey == 12) {
            ShowChinese12x12(x, y, s, fc, bc, sizey, mode);
        } else if (sizey == 16) {
            ShowChinese16x16(x, y, s, fc, bc, sizey, mode);
        } else if (sizey == 24) {
            ShowChinese24x24(x, y, s, fc, bc, sizey, mode);
        } else if (sizey == 32) {
            ShowChinese32x32(x, y, s, fc, bc, sizey, mode);
        } else {
            return;
        }
        s += 2;
        x += sizey;
    }
}

/**
 * @brief   显示12*12汉字字符串
 * 
 * @param x     起始地址x
 * @param y     起始地址y
 * @param s     字符串
 * @param fc    字符颜色
 * @param bc    背景颜色
 * @param sizey 字号
 * @param mode  复写模式
 */
void LCD::ShowChinese12x12(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;        //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont12) / sizeof(typFNT_GB12);  //统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont12[k].Index[0] == *(s)) && (tfont12[k].Index[1] == *(s + 1))) {
            LCD_SetAddress(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)  //非叠加方式
                    {
                        if (tfont12[k].Msk[i] & (0x01 << j)) {
                            LCD_WR_DATA16(fc);
                        } else {
                            LCD_WR_DATA16(bc);
                        }
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else  //叠加方式
                    {
                        if (tfont12[k].Msk[i] & (0x01 << j)) {
                            DrawPoint(x, y, fc);
                        }  //画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/**
 * @brief   显示16*16汉字字符串
 * 
 * @param x     起始地址x
 * @param y     起始地址y
 * @param s     字符串
 * @param fc    字符颜色
 * @param bc    背景颜色
 * @param sizey 字号
 * @param mode  复写模式
 */
void LCD::ShowChinese16x16(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;        //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont16) / sizeof(typFNT_GB16);  //统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s + 1))) {
            LCD_SetAddress(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)  //非叠加方式
                    {
                        if (tfont16[k].Msk[i] & (0x01 << j)) {
                            LCD_WR_DATA16(fc);
                        } else {
                            LCD_WR_DATA16(bc);
                        }
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else  //叠加方式
                    {
                        if (tfont16[k].Msk[i] & (0x01 << j)) {
                            DrawPoint(x, y, fc);
                        }  //画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/**
 * @brief   显示24*24汉字字符串
 * 
 * @param x     起始地址x
 * @param y     起始地址y
 * @param s     字符串
 * @param fc    字符颜色
 * @param bc    背景颜色
 * @param sizey 字号
 * @param mode  复写模式
 */
void LCD::ShowChinese24x24(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;        //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont24) / sizeof(typFNT_GB24);  //统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s + 1))) {
            LCD_SetAddress(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)  //非叠加方式
                    {
                        if (tfont24[k].Msk[i] & (0x01 << j)) {
                            LCD_WR_DATA16(fc);
                        } else {
                            LCD_WR_DATA16(bc);
                        }
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else  //叠加方式
                    {
                        if (tfont24[k].Msk[i] & (0x01 << j)) {
                            DrawPoint(x, y, fc);
                        }  //画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/**
 * @brief   显示32*32汉字字符串
 * 
 * @param x     起始地址x
 * @param y     起始地址y
 * @param s     字符串
 * @param fc    字符颜色
 * @param bc    背景颜色
 * @param sizey 字号
 * @param mode  复写模式
 */
void LCD::ShowChinese32x32(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;        //汉字数目
    uint16_t TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    TypefaceNum = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
    HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);  //统计汉字数目
    for (k = 0; k < HZnum; k++) {
        if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s + 1))) {
            LCD_SetAddress(x, y, x + sizey - 1, y + sizey - 1);
            for (i = 0; i < TypefaceNum; i++) {
                for (j = 0; j < 8; j++) {
                    if (!mode)  //非叠加方式
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j)) {
                            LCD_WR_DATA16(fc);
                        } else {
                            LCD_WR_DATA16(bc);
                        }
                        m++;
                        if (m % sizey == 0) {
                            m = 0;
                            break;
                        }
                    } else  //叠加方式
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j)) {
                            DrawPoint(x, y, fc);
                        }  //画一个点
                        x++;
                        if ((x - x0) == sizey) {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }
        }
        continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
    }
}

/** 
 * @brief   显示单个字符
 * 
 * @param x     显示坐标
 * @param y     显示坐标
 * @param chr   要显示的字符
 * @param fc    字的颜色
 * @param bc    字的背景色
 * @param sizey 字号
 * @param mode  0非叠加模式  1叠加模式
 */
void LCD::ShowChar(uint16_t x, uint16_t y, uint8_t chr, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    uint8_t temp, sizex, t, m = 0;
    uint16_t i, TypefaceNum;  //一个字符所占字节大小
    uint16_t x0 = x;
    sizex = sizey / 2;
    TypefaceNum = (sizex / 8 + ((sizex % 8) ? 1 : 0)) * sizey;
    chr = chr - ' ';                                     //得到偏移后的值
    LCD_SetAddress(x, y, x + sizex - 1, y + sizey - 1);  //设置光标位置
    for (i = 0; i < TypefaceNum; i++) {
        if (sizey == 12) {
            temp = ascii_1206[chr][i];
        }  //调用6x12字体
        else if (sizey == 16) {
            temp = ascii_1608[chr][i];
        }  //调用8x16字体
        else if (sizey == 24) {
            temp = ascii_2412[chr][i];
        }  //调用12x24字体
        else if (sizey == 32) {
            temp = ascii_3216[chr][i];
        }  //调用16x32字体
        else {
            return;
        }
        for (t = 0; t < 8; t++) {
            if (!mode)  //非叠加模式
            {
                if (temp & (0x01 << t)) {
                    LCD_WR_DATA16(fc);
                } else {
                    LCD_WR_DATA16(bc);
                }
                m++;
                if (m % sizex == 0) {
                    m = 0;
                    break;
                }
            } else  //叠加模式
            {
                if (temp & (0x01 << t)) {
                    DrawPoint(x, y, fc);
                }  //画一个点
                x++;
                if ((x - x0) == sizex) {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

/**
 * @brief   显示字符串
 * 
 * @param x     显示坐标
 * @param y     显示坐标
 * @param p     要显示的字符串
 * @param fc    字的颜色
 * @param bc    字的背景色
 * @param sizey 字号
 * @param mode  0非叠加模式  1叠加模式
 */
void LCD::ShowString(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode) {
    while (*p != '\0') {
        ShowChar(x, y, *p, fc, bc, sizey, mode);
        x += sizey / 2;
        p++;
    }
}

/**
 * @brief   显示字符串
 * 
 * @param x     显示坐标
 * @param y     显示坐标
 * @param fc    字的颜色
 * @param bc    字的背景色
 * @param sizey 字号
 * @param mode  0非叠加模式  1叠加模式
 * @param str   格式化字符串
 * @param ...   字符串参数
 */
void LCD::ShowText(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode, const char *str, ...) {
    char strFormatted[256];
    va_list args;         //存放可变参数的数据结构
    va_start(args, str);  //初始化可变参数,需要传一个va_list类型变量,和可变参数之前的参数,这里是str
    vsprintf(strFormatted, str, args);
    ShowString(x, y, (const uint8_t *)strFormatted, fc, bc, sizey, mode);  //此函数在头文件 stdio中
    va_end(args);
}

/**
 * @brief   靠右显示字符串
 * 
 * @param x             行
 * @param y             列
 * @param fc            字体颜色
 * @param bc            背景颜色
 * @param sizey         字体大小
 * @param mode          复写模式
 * @param totalWidth    目标总宽度
 * @param str           格式化字符串
 * @param ...           字符串参数
 */
void LCD::ShowTextAlignRight(uint16_t x, uint16_t y, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode, int totalWidth, const char *str, ...) {
    char strFormatted[256];
    va_list args;         //存放可变参数的数据结构
    va_start(args, str);  //初始化可变参数,需要传一个va_list类型变量,和可变参数之前的参数,这里是str
    vsprintf(strFormatted, str, args);
    Fill(x, y, x + totalWidth - sizey * strlen(strFormatted) / 2 - 1, y + sizey, bc);
    ShowString(x + totalWidth - sizey * strlen(strFormatted) / 2, y, (const uint8_t *)strFormatted, fc, bc, sizey, mode);  //此函数在头文件 stdio中
    va_end(args);
}

/**
 * @brief   显示图片
 * 
 * @param x         起点坐标
 * @param y         起点坐标
 * @param length    图片数组长度
 * @param width     图片宽度
 * @param pic       图片数组
 */
void LCD::ShowPicture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t pic[]) {
    uint16_t i, j;
    uint32_t k = 0;
    LCD_SetAddress(x, y, x + length - 1, y + width - 1);
    for (i = 0; i < length; i++) {
        for (j = 0; j < width; j++) {
            LCD_WR_DATA8(pic[k * 2]);
            LCD_WR_DATA8(pic[k * 2 + 1]);
            k++;
        }
    }
}

LCD lcd{};