#include "oled.h"
#include "i2c.h"

static const uint8_t OLED_ASCII_Font[][16] =
{
#include "font_ascii_8x16.h"
};

static uint8_t OLED_GRAM[OLED_WIDTH][OLED_HEIGHT / 8] = {0};

static void OLED_SendByte(uint8_t data, uint8_t mode)
{
    uint8_t buf[2] = {mode, data};
    HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDR, buf, 2, 10);
}

/**
 * @brief  设置光标位置
 * @param  x: 列地址(0~127)
 * @param  page: 页地址(0~7)
 * @retval 无
 */
static void OLED_SetCursor(uint8_t x, uint8_t page)
{
    OLED_SendByte(0xB0 + page, OLED_CMD);     // 设置页
    OLED_SendByte((x & 0x0F), OLED_CMD);     // 设置列低4位
    OLED_SendByte(0x10 | (x >> 4), OLED_CMD); // 设置列高4位
}

/************************ 外部接口函数 ************************/
void OLED_Init(void)
{
    HAL_Delay(100); // 上电延时

    // SSD1306标准初始化序列（精简版）
    OLED_SendByte(0xAE, OLED_CMD); // 关闭显示
    OLED_SendByte(0xD5, OLED_CMD);
    OLED_SendByte(0x80, OLED_CMD); // 时钟分频
    OLED_SendByte(0xA8, OLED_CMD);
    OLED_SendByte(0x3F, OLED_CMD); // 路数1/64
    OLED_SendByte(0xD3, OLED_CMD);
    OLED_SendByte(0x00, OLED_CMD); // 显示偏移
    OLED_SendByte(0x40, OLED_CMD); // 起始行
    OLED_SendByte(0x8D, OLED_CMD);
    OLED_SendByte(0x14, OLED_CMD); // 电荷泵使能
    OLED_SendByte(0x20, OLED_CMD);
    OLED_SendByte(0x00, OLED_CMD); // 水平地址模式
    OLED_SendByte(0xA1, OLED_CMD); // 段重映射
    OLED_SendByte(0xC8, OLED_CMD); // 扫描方向
    OLED_SendByte(0xDA, OLED_CMD);
    OLED_SendByte(0x12, OLED_CMD); // 引脚配置
    OLED_SendByte(0x81, OLED_CMD);
    OLED_SendByte(0xCF, OLED_CMD); // 对比度
    OLED_SendByte(0xD9, OLED_CMD);
    OLED_SendByte(0xF1, OLED_CMD); // 预充电周期
    OLED_SendByte(0xDB, OLED_CMD);
    OLED_SendByte(0x30, OLED_CMD); // VCOMH
    OLED_SendByte(0xA4, OLED_CMD); // 全局显示
    OLED_SendByte(0xA6, OLED_CMD); // 正常显示
    OLED_SendByte(0xAF, OLED_CMD); // 开启显示

    OLED_Clear();
}

void OLED_Clear(void)
{
    // 清空GRAM
    for (uint8_t i = 0; i < OLED_WIDTH; i++)
        for (uint8_t j = 0; j < (OLED_HEIGHT / 8); j++)
            OLED_GRAM[i][j] = 0x00;
    OLED_Refresh();
}

void OLED_DrawPoint(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return; // 越界保护
    OLED_GRAM[x][y / 8] |= (1 << (y % 8));
}

void OLED_ShowChar(uint8_t x, uint8_t y, char chr)
{
    uint8_t c = chr - ' ';  // 计算字符在字库中的索引
    if (c > 94) return;     // 超出可打印字符范围

    uint8_t page = y / 8;   // 计算起始页
    uint8_t i;

    // 8*16字体：8列宽度，占用2个页(16行)
    for (i = 0; i < 8; i++)
    {
        OLED_GRAM[x + i][page]   = OLED_ASCII_Font[c][i];  // 上8行
        OLED_GRAM[x + i][page + 1] = OLED_ASCII_Font[c][i + 8]; // 下8行
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *str)
{
    while (*str != '\0')
    {
        OLED_ShowChar(x, y, *str);
        x += 8;          // 每个字符占8列
        if (x > 120)     // 自动换行
        {
            x = 0;
            y += 16;
        }
        str++;
    }
    OLED_Refresh(); // 必须刷新屏幕
}

void OLED_Refresh(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        OLED_SetCursor(0, page);
        for (uint8_t col = 0; col < OLED_WIDTH; col++)
        {
            OLED_SendByte(OLED_GRAM[col][page], OLED_DATA);
        }
    }
}
