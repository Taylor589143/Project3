#include "stm32f10x.h"
#include "OLED_Font.h"

/* ==========================================================
 * OLED 驱动文件（I2C 软件模拟版）
 *
 * 硬件连接：
 *   SCL → PB8
 *   SDA → PB9
 *
 * 提供的主要接口：
 *   - OLED_Init()              初始化 OLED
 *   - OLED_Clear()             清屏
 *   - OLED_ShowChar()          显示单个字符（8x16）
 *   - OLED_ShowString()        显示字符串
 *   - OLED_ShowNum()           显示无符号十进制数
 *   - OLED_ShowSignedNum()     显示带符号十进制数
 *   - OLED_ShowHexNum()        显示十六进制数
 *   - OLED_ShowBinNum()        显示二进制数
 *   - OLED_ShowFloat()         显示浮点数（小数）
 *   - OLED_On() / OLED_Off()   开关 OLED 显示
 * ========================================================== */

/*--------- 底层 I2C 引脚控制 ---------*/
#define OLED_W_SCL(x)   GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)   GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

/*------------------------------------------------
 *  引脚初始化：PB8 / PB9 开漏输出，用作 I2C
 *------------------------------------------------*/
void OLED_I2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;   // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_8;         // SCL
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_9;         // SDA
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/*------------------------------------------------
 *  软件 I2C 起始 & 停止 & 发送字节
 *------------------------------------------------*/
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/**
 * @brief  通过 I2C 发送一个字节（不处理 ACK）
 */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA( (Byte & (0x80 >> i)) ? 1 : 0 );
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }
    /* 额外时钟：忽略从机应答 */
    OLED_W_SCL(1);
    OLED_W_SCL(0);
}

/*------------------------------------------------
 *  命令 / 数据 写入
 *------------------------------------------------*/
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);   // 从机地址（写）
    OLED_I2C_SendByte(0x00);   // 接下来是命令
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);   // 从机地址（写）
    OLED_I2C_SendByte(0x40);   // 接下来是数据
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/*------------------------------------------------
 *  光标 / 清屏
 *------------------------------------------------*/
/**
 * @brief 设置显存光标位置
 * @param Y 0~7（每个值是 8 像素高的一页）
 * @param X 0~127 像素列
 */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);                       // 设置页地址（Y）
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));       // X 高 4 位
    OLED_WriteCommand(0x00 | (X & 0x0F));              // X 低 4 位
}

/**
 * @brief 清屏（所有像素清 0）
 */
void OLED_Clear(void)
{
    uint8_t page, col;
    for (page = 0; page < 8; page++)
    {
        OLED_SetCursor(page, 0);
        for (col = 0; col < 128; col++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/*------------------------------------------------
 *  字符 / 字符串显示（8x16 字模）
 *------------------------------------------------*/
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;
    uint8_t y = (Line - 1) * 2;        // 每行 16 像素，高度 2 页
    uint8_t x = (Column - 1) * 8;      // 每列 8 像素宽

    /* 上半部分（8 点高） */
    OLED_SetCursor(y, x);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }

    /* 下半部分 */
    OLED_SetCursor(y + 1, x);
    for (i = 0; i < 8; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
    }
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i = 0;
    while (String[i] != '\0')
    {
        OLED_ShowChar(Line, Column + i, String[i]);
        i++;
    }
}

/*------------------------------------------------
 *  基础数学：整数次幂
 *------------------------------------------------*/
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--)
    {
        Result *= X;
    }
    return Result;
}

/*------------------------------------------------
 *  各种格式数字显示
 *------------------------------------------------*/
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        uint32_t temp = Number / OLED_Pow(10, Length - i - 1);
        OLED_ShowChar(Line, Column + i, (temp % 10) + '0');
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint32_t Number1;
    uint8_t i;

    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = (uint32_t)(-Number);
    }

    for (i = 0; i < Length; i++)
    {
        uint32_t temp = Number1 / OLED_Pow(10, Length - i - 1);
        OLED_ShowChar(Line, Column + 1 + i, (temp % 10) + '0');
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        uint8_t single = (Number / OLED_Pow(16, Length - i - 1)) % 16;
        if (single < 10)
        {
            OLED_ShowChar(Line, Column + i, single + '0');
        }
        else
        {
            OLED_ShowChar(Line, Column + i, single - 10 + 'A');
        }
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        uint32_t temp = Number / OLED_Pow(2, Length - i - 1);
        OLED_ShowChar(Line, Column + i, (temp % 2) + '0');
    }
}

/*------------------------------------------------
 *  浮点数显示（独立实现版本）
 *------------------------------------------------*/
/**
 * @brief  显示浮点数，例如：-12.34
 * @param  Line       行号 1~4
 * @param  Column     列号 1~16（整数部分起始列）
 * @param  Number     要显示的浮点数
 * @param  IntLength  整数部分显示宽度（不含符号）
 * @param  FracLength 小数部分显示位数
 */
void OLED_ShowFloat(uint8_t Line, uint8_t Column,
                    float Number, uint8_t IntLength, uint8_t FracLength)
{
    uint8_t  negative = 0;
    uint32_t scale    = OLED_Pow(10, FracLength);
    uint32_t scaled   = 0;
    uint32_t intPart  = 0;
    uint32_t fracPart = 0;

    /* 处理负号 */
    if (Number < 0.0f)
    {
        negative = 1;
        Number   = -Number;
    }

    /* 放大后四舍五入成整数，减少浮点误差 */
    scaled   = (uint32_t)(Number * scale + 0.5f);
    intPart  = scaled / scale;
    fracPart = scaled % scale;

    if (negative)
    {
        OLED_ShowChar(Line, Column, '-');
        Column++;
    }

    /* 显示整数部分 */
    OLED_ShowNum(Line, Column, intPart, IntLength);

    /* 显示小数点 */
    OLED_ShowChar(Line, Column + IntLength, '.');

    /* 显示小数部分，不足位数自动补零 */
    OLED_ShowNum(Line, Column + IntLength + 1, fracPart, FracLength);
}

/*------------------------------------------------
 *  OLED 显示开关
 *------------------------------------------------*/
void OLED_Off(void)
{
    OLED_WriteCommand(0xAE);   // 关闭显示
}

void OLED_On(void)
{
    OLED_WriteCommand(0xAF);   // 打开显示
}

/*------------------------------------------------
 *  OLED 初始化
 *------------------------------------------------*/
void OLED_Init(void)
{
    uint32_t i, j;

    /* 上电简单延时，保证电压稳定 */
    for (i = 0; i < 1000; i++)
    {
        for (j = 0; j < 1000; j++);
    }

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE);    // 关闭显示

    OLED_WriteCommand(0xD5);    // 设置显示时钟分频/振荡频率
    OLED_WriteCommand(0x80);

    OLED_WriteCommand(0xA8);    // 设置多路复用率
    OLED_WriteCommand(0x3F);    // 1/64

    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);

    OLED_WriteCommand(0x40);    // 设置显示起始行

    OLED_WriteCommand(0xA1);    // 段重映射，0xA1 正常，0xA0 左右反置
    OLED_WriteCommand(0xC8);    // 行扫描方向，0xC8 正常，0xC0 上下反置

    OLED_WriteCommand(0xDA);    // 设置 COM 引脚硬件配置
    OLED_WriteCommand(0x12);

    OLED_WriteCommand(0x81);    // 对比度设置
    OLED_WriteCommand(0xCF);

    OLED_WriteCommand(0xD9);    // 预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    // VCOMH 取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);    // 整屏显示开/关，0xA4 按显存显示，0xA5 整屏亮

    OLED_WriteCommand(0xA6);    // 正常 / 反相显示，0xA6 正常，0xA7 反相

    OLED_WriteCommand(0x8D);    // 充电泵设置
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);    // 开启显示

    OLED_Clear();               // 上电后先清屏
}
