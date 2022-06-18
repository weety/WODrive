#include <st7735.h>

LCD_TypeDef lcdIO;

static const uint8_t
    generic_st7735R[] = {              // 7735R init, part 1 (red or green tab)
        16,                            // 15 commands in list:
        ST77XX_SWRESET, ST_CMD_DELAY,  //  1: Software reset, 0 args, w/delay
        150,                           //     150 ms delay
        ST77XX_SLPOUT, ST_CMD_DELAY,   //  2: Out of sleep mode, 0 args, w/delay
        255,                           //     500 ms delay
        ST7735_FRMCTR1, 3,             //  3: Framerate ctrl - normal mode, 3 arg:
        0x01, 0x2C, 0x2D,              //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR2, 3,             //  4: Framerate ctrl - idle mode, 3 args:
        0x01, 0x2C, 0x2D,              //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
        ST7735_FRMCTR3, 6,             //  5: Framerate - partial mode, 6 args:
        0x01, 0x2C, 0x2D,              //     Dot inversion mode
        0x01, 0x2C, 0x2D,              //     Line inversion mode
        ST7735_INVCTR, 1,              //  6: Display inversion ctrl, 1 arg:
        0x07,                          //     No inversion
        ST7735_PWCTR1, 3,              //  7: Power control, 3 args, no delay:
        0xA2,
        0x02,              //     -4.6V
        0x84,              //     AUTO mode
        ST7735_PWCTR2, 1,  //  8: Power control, 1 arg, no delay:
        0xC5,              //     VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
        ST7735_PWCTR3, 2,  //  9: Power control, 2 args, no delay:
        0x0A,              //     Opamp current small
        0x00,              //     Boost frequency
        ST7735_PWCTR4, 2,  // 10: Power control, 2 args, no delay:
        0x8A,              //     BCLK/2,
        0x2A,              //     opamp current small & medium low
        ST7735_PWCTR5, 2,  // 11: Power control, 2 args, no delay:
        0x8A, 0xEE,
        ST7735_VMCTR1, 1,  // 12: Power control, 1 arg, no delay:
        0x0E,
        ST77XX_INVOFF, 0,  // 13: Don't invert display, no args
        ST77XX_MADCTL, 1,  // 14: Mem access ctl (directions), 1 arg:
        0xC8,              //     row/col addr, bottom-top refresh
        ST77XX_COLMOD, 1,  // 15: set color mode, 1 arg, no delay:
        0x05,              //     16-bit color
        ST77XX_MADCTL, 1, 0xC8};

/*!
 @brief   Adafruit_SPITFT Send Command handles complete sending of commands and
 data
 @param   commandByte       The Command Byte
 @param   dataBytes         A pointer to the Data bytes to send
 @param   numDataBytes      The number of bytes we should send
 */
void sendCommand(uint8_t commandByte, const uint8_t *dataBytes,
                 uint8_t numDataBytes) {
    LCD_WR_REG(commandByte);  // Send the command byte
    for (int i = 0; i < numDataBytes; i++) {
        LCD_WR_DATA8(*(const unsigned char *)(dataBytes++));
    }
}

/**************************************************************************/
/*!
    @brief  Companion code to the initiliazation tables. Reads and issues
            a series of LCD commands stored in PROGMEM byte array.
    @param  addr  Flash memory array with commands and data to send
*/
/**************************************************************************/
void displayInit(const uint8_t *addr) {
    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = *(const unsigned char *)(addr++);  // Number of commands to follow
    while (numCommands--) {                          // For each command...
        cmd = *(const unsigned char *)(addr++);      // Read command
        numArgs = *(const unsigned char *)(addr++);  // Number of args to follow
        ms = numArgs & ST_CMD_DELAY;                 // If hibit set, delay follows args
        numArgs &= ~ST_CMD_DELAY;                    // Mask out delay bit
        sendCommand(cmd, addr, numArgs);
        addr += numArgs;

        if (ms) {
            ms = *(const unsigned char *)(addr++);  // Read post-command delay time (ms)
            if (ms == 255)
                ms = 500;  // If 255, delay for 500 ms
            HAL_Delay(ms);
        }
    }
}

/******************************************************************************
      函数说明：LCD串行数据写入函数
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(uint8_t dat) {
    uint8_t i;
    lcdIO.WritePin(lcdIO.CS_GPIO_Port, lcdIO.CS_GPIO_Pin, GPIO_PIN_RESET);
    for (i = 0; i < 8; i++) {
        lcdIO.WritePin(lcdIO.SCLK_GPIO_Port, lcdIO.SCLK_GPIO_Pin, GPIO_PIN_RESET);
        if (dat & 0x80) {
            lcdIO.WritePin(lcdIO.MOSI_GPIO_Port, lcdIO.MOSI_GPIO_Pin, GPIO_PIN_SET);
        } else {
            lcdIO.WritePin(lcdIO.MOSI_GPIO_Port, lcdIO.MOSI_GPIO_Pin, GPIO_PIN_RESET);
        }
        lcdIO.WritePin(lcdIO.SCLK_GPIO_Port, lcdIO.SCLK_GPIO_Pin, GPIO_PIN_SET);
        dat <<= 1;
    }
    lcdIO.WritePin(lcdIO.CS_GPIO_Port, lcdIO.CS_GPIO_Pin, GPIO_PIN_SET);
}

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(uint8_t dat) {
    LCD_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA16(uint16_t dat) {
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}

/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(uint8_t dat) {
    lcdIO.WritePin(lcdIO.DC_GPIO_Port, lcdIO.DC_GPIO_Pin, GPIO_PIN_RESET);  //写命令
    LCD_Writ_Bus(dat);
    lcdIO.WritePin(lcdIO.DC_GPIO_Port, lcdIO.DC_GPIO_Pin, GPIO_PIN_SET);  //写数据
}

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_SetAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    if (USE_HORIZONTAL == 0) {
        LCD_WR_REG(0x2a);  //列地址设置
        LCD_WR_DATA16(x1 + 52);
        LCD_WR_DATA16(x2 + 52);
        LCD_WR_REG(0x2b);  //行地址设置
        LCD_WR_DATA16(y1 + 40);
        LCD_WR_DATA16(y2 + 40);
        LCD_WR_REG(0x2c);  //储存器写
    } else if (USE_HORIZONTAL == 1) {
        LCD_WR_REG(0x2a);  //列地址设置
        LCD_WR_DATA16(x1 + 53);
        LCD_WR_DATA16(x2 + 53);
        LCD_WR_REG(0x2b);  //行地址设置
        LCD_WR_DATA16(y1 + 40);
        LCD_WR_DATA16(y2 + 40);
        LCD_WR_REG(0x2c);  //储存器写
    } else if (USE_HORIZONTAL == 2) {
        LCD_WR_REG(0x2a);  //列地址设置
        LCD_WR_DATA16(x1 + 40);
        LCD_WR_DATA16(x2 + 40);
        LCD_WR_REG(0x2b);  //行地址设置
        LCD_WR_DATA16(y1 + 53);
        LCD_WR_DATA16(y2 + 53);
        LCD_WR_REG(0x2c);  //储存器写
    } else {
        LCD_WR_REG(0x2a);  //列地址设置
        LCD_WR_DATA16(x1 + 40);
        LCD_WR_DATA16(x2 + 40);
        LCD_WR_REG(0x2b);  //行地址设置
        LCD_WR_DATA16(y1 + 52);
        lcdIO.WritePin(lcdIO.RES_GPIO_Port, lcdIO.RES_GPIO_Pin, GPIO_PIN_RESET);
        LCD_WR_DATA16(y2 + 52);
        LCD_WR_REG(0x2c);  //储存器写
    }
}

void LCD_RegisterIO(LCD_TypeDef *LCD_InitStruct) {
    lcdIO.WritePin = LCD_InitStruct->WritePin;
    lcdIO.SCLK_GPIO_Port = LCD_InitStruct->SCLK_GPIO_Port;
    lcdIO.MOSI_GPIO_Port = LCD_InitStruct->MOSI_GPIO_Port;
    lcdIO.RES_GPIO_Port = LCD_InitStruct->RES_GPIO_Port;
    lcdIO.DC_GPIO_Port = LCD_InitStruct->DC_GPIO_Port;
    lcdIO.CS_GPIO_Port = LCD_InitStruct->CS_GPIO_Port;
    lcdIO.BLK_GPIO_Port = LCD_InitStruct->BLK_GPIO_Port;
    lcdIO.SCLK_GPIO_Pin = LCD_InitStruct->SCLK_GPIO_Pin;
    lcdIO.MOSI_GPIO_Pin = LCD_InitStruct->MOSI_GPIO_Pin;
    lcdIO.RES_GPIO_Pin = LCD_InitStruct->RES_GPIO_Pin;
    lcdIO.DC_GPIO_Pin = LCD_InitStruct->DC_GPIO_Pin;
    lcdIO.CS_GPIO_Pin = LCD_InitStruct->CS_GPIO_Pin;
    lcdIO.BLK_GPIO_Pin = LCD_InitStruct->BLK_GPIO_Pin;
}

void LCD_Init() {
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

    lcdIO.WritePin(lcdIO.RES_GPIO_Port, lcdIO.RES_GPIO_Pin, GPIO_PIN_RESET);  //复位
    HAL_Delay(100);
    lcdIO.WritePin(lcdIO.RES_GPIO_Port, lcdIO.RES_GPIO_Pin, GPIO_PIN_SET);  //复位
    HAL_Delay(100);
    lcdIO.WritePin(lcdIO.BLK_GPIO_Port, lcdIO.BLK_GPIO_Pin, GPIO_PIN_SET);  //打开背光
    HAL_Delay(100);
    displayInit(generic_st7735);
}