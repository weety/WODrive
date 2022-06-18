#include <st7789.h>

LCD_TypeDef lcdIO;

static const uint8_t
    generic_st7789[] = {                  // Init commands for 7789 screens
        10,                               //  9 commands in list:
        ST77XX_SWRESET, ST_CMD_DELAY,     //  1: Software reset, no args, w/delay
        150,                              //     ~150 ms delay
        ST77XX_SLPOUT, ST_CMD_DELAY,      //  2: Out of sleep mode, no args, w/delay
        10,                               //      10 ms delay
        ST77XX_COLMOD, 1 + ST_CMD_DELAY,  //  3: Set color mode, 1 arg + delay:
        0x55,                             //     16-bit color
        10,                               //     10 ms delay
        ST77XX_MADCTL, 1,                 //  4: Mem access ctrl (directions), 1 arg:
        0x08,                             //     Row/col addr, bottom-top refresh
        ST77XX_CASET, 4,                  //  5: Column addr set, 4 args, no delay:
        0x00,
        0,  //     XSTART = 0
        0,
        240,              //     XEND = 240
        ST77XX_RASET, 4,  //  6: Row addr set, 4 args, no delay:
        0x00,
        0,  //     YSTART = 0
        320 >> 8,
        320 & 0xFF,                  //     YEND = 320
        ST77XX_INVON, ST_CMD_DELAY,  //  7: hack
        10,
        ST77XX_NORON, ST_CMD_DELAY,   //  8: Normal display on, no args, w/delay
        10,                           //     10 ms delay
        ST77XX_DISPON, ST_CMD_DELAY,  //  9: Main screen turn on, no args, delay
        10,                           //    10 ms delay
        ST77XX_MADCTL, 1, 0x60 /*0xA0*/};

/**
 * @brief   源自Adafruit_SPITFT的发送整组命令的函数
 * 
 * @param commandByte   命令条数
 * @param dataBytes     数据的长度、是否延时
 * @param numDataBytes  数据、延时长度
 */
void sendCommand(uint8_t commandByte, const uint8_t *dataBytes,
                 uint8_t numDataBytes) {
    LCD_WR_REG(commandByte);  // Send the command byte
    for (int i = 0; i < numDataBytes; i++) {
        LCD_WR_DATA8(*(const unsigned char *)(dataBytes++));
    }
}

/**
 * @brief   源自Adafruit_SPITFT的屏幕初始化函数
 * 
 * @param addr  初始化命令列表
 */
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

/**
 * @brief   LCD串行数据写入函数
 * 
 * @param dat   要写入的串行数据
 */
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

/**
 * @brief   LCD写入8位数据
 * 
 * @param dat   写入的数据
 */
void LCD_WR_DATA8(uint8_t dat) {
    LCD_Writ_Bus(dat);
}

/**
 * @brief   LCD写入16位数据
 * 
 * @param dat   写入的数据
 */
void LCD_WR_DATA16(uint16_t dat) {
    LCD_Writ_Bus(dat >> 8);
    LCD_Writ_Bus(dat);
}

/**
 * @brief   LCD写入命令
 * 
 * @param dat   写入的命令
 */
void LCD_WR_REG(uint8_t dat) {
    lcdIO.WritePin(lcdIO.DC_GPIO_Port, lcdIO.DC_GPIO_Pin, GPIO_PIN_RESET);  //写命令
    LCD_Writ_Bus(dat);
    lcdIO.WritePin(lcdIO.DC_GPIO_Port, lcdIO.DC_GPIO_Pin, GPIO_PIN_SET);  //写数据
}

/**
 * @brief   设置起始和结束地址
 * 
 * @param x1    列起始地址
 * @param y1    行起始地址
 * @param x2    列结束地址
 * @param y2    行结束地址
 */
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

/**
 * @brief   LCD的GPIO注册函数
 * 
 * @param LCD_InitStruct    LCD相关IO接口
 */
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

/**
 * @brief   LCD初始化函数
 * 
 */
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
    displayInit(generic_st7789);
}