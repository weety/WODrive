#ifndef _ST7789_H_
#define _ST7789_H_
#include <stdlib.h>

#include "lcd_font.h"
#include "lcd_pic.h"
#include "stdbool.h"
#include "stdio.h"
#include "stm32f4xx_hal.h"
#include "string.h"
#include "wodrive_hal_cfg.h"

#define ST_CMD_DELAY 0x80  // special signifier for command lists

#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define USE_HORIZONTAL 2  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏

#if USE_HORIZONTAL == 0 || USE_HORIZONTAL == 1
#define LCD_W 135
#define LCD_H 240

#else
#define LCD_W 240
#define LCD_H 135
#endif

typedef struct
{
    GPIO_TypeDef *SCLK_GPIO_Port, *MOSI_GPIO_Port, *RES_GPIO_Port, *DC_GPIO_Port, *CS_GPIO_Port, *BLK_GPIO_Port;
    uint16_t SCLK_GPIO_Pin, MOSI_GPIO_Pin, RES_GPIO_Pin, DC_GPIO_Pin, CS_GPIO_Pin, BLK_GPIO_Pin;
    void (*WritePin)(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
} LCD_TypeDef;

void LCD_RegisterIO(LCD_TypeDef* LCD_InitStruct);                               //初始化GPIO
void LCD_Init(void);                                                      //初始化GPIO
void LCD_Writ_Bus(uint8_t dat);                                           //模拟SPI时序
void LCD_WR_DATA8(uint8_t dat);                                           //写入一个字节
void LCD_WR_DATA16(uint16_t dat);                                         //写入两个字节
void LCD_WR_REG(uint8_t dat);                                             //写入一个指令
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);  //设置坐标函数

#endif
