#include "stm32f10x.h"   // Device header
#include "Key.h"
#include "Menu.h"        // 里面有 extern uint8_t menu_state, edit_mode

/* ==========================================================
 * 按键驱动（独立实现版）
 *
 * 硬件连接（和你的引脚表一致）：
 *   UP    -> PC13  上拉输入
 *   DOWN  -> PC14  上拉输入
 *   OK    -> PC15  上拉输入
 *   BACK  -> PB0   上拉输入
 *
 * 提供接口：
 *   void    Key_Init(void);
 *   uint8_t Key_Scan(void);          // 每 10ms 调一次，返回键值
 *   uint8_t Key_ScanWithDelay(void); // 兼容老接口，等价于 Key_Scan
 *
 * 行为特性：
 *   1. 所有按键都有消抖 + 短按
 *   2. 在 menu_state == 2 && edit_mode == 1 时，
 *      UP / DOWN 支持“长按连续调节 PID 参数”
 * ========================================================== */

#define KEY_NUM              4     // 按键数量
#define KEY_DEBOUNCE_TICKS   5     // 消抖次数：5 次 * 10ms ≈ 50ms
#define KEY_LONG_START_TICKS 100   // 长按判定：100 次 * 10ms ≈ 1s
#define KEY_LONG_REPEAT_TICKS 10   // 长按连发间隔：10 次 * 10ms ≈ 100ms

/* 用索引表示 4 个按键：0=UP 1=DOWN 2=OK 3=BACK */
enum
{
    KEY_IDX_UP = 0,
    KEY_IDX_DOWN,
    KEY_IDX_OK,
    KEY_IDX_BACK
};

/* 映射：内部索引 -> 对外返回的按键值 */
static const uint8_t s_KeyCodeMap[KEY_NUM] =
{
    KEY_UP,      // idx 0
    KEY_DOWN,    // idx 1
    KEY_OK,      // idx 2
    KEY_BACK     // idx 3
};

/* 每个按键的滤波状态 */
typedef struct
{
    uint8_t  last_level;    // 上一次采样到的电平（1=松开，0=按下）
    uint8_t  stable_level;  // 已确认稳定的电平（经过消抖后的结果）
    uint8_t  debounce_cnt;  // 连续相同电平的计数（用于消抖）
    uint16_t hold_ticks;    // 在“稳定按下”状态下持续的时间计数
    uint8_t  long_mode;     // 是否已经进入“长按模式”（1=已进入）
} KeyFilter_t;

/* 4 个按键各自的状态 */
static KeyFilter_t s_Keys[KEY_NUM];

/*------------------------------------------------
 *  硬件初始化：配置 GPIO 为上拉输入
 *------------------------------------------------*/
void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 开启 GPIOB 和 GPIOC 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);

    /* PC13 / PC14 / PC15 : 上、下、确认 键 */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;   // 上拉输入：松开=1，按下=0
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* PB0 : 返回键 */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 初始化按键滤波状态：默认都处于“松开” */
    for (int i = 0; i < KEY_NUM; i++)
    {
        s_Keys[i].last_level   = 1;
        s_Keys[i].stable_level = 1;
        s_Keys[i].debounce_cnt = 0;
        s_Keys[i].hold_ticks   = 0;
        s_Keys[i].long_mode    = 0;
    }
}

/*------------------------------------------------
 *  读取 4 个按键当前的物理电平（不做消抖）
 *  返回到 levels[0..3] 中，1=松开，0=按下
 *------------------------------------------------*/
static void Key_ReadLevels(uint8_t levels[KEY_NUM])
{
    levels[KEY_IDX_UP]   = GPIO_ReadInputDataBit(KEY_UP_PORT,   KEY_UP_PIN);
    levels[KEY_IDX_DOWN] = GPIO_ReadInputDataBit(KEY_DOWN_PORT, KEY_DOWN_PIN);
    levels[KEY_IDX_OK]   = GPIO_ReadInputDataBit(KEY_OK_PORT,   KEY_OK_PIN);
    levels[KEY_IDX_BACK] = GPIO_ReadInputDataBit(KEY_BACK_PORT, KEY_BACK_PIN);
}


uint8_t Key_Scan(void)
{
    uint8_t levels[KEY_NUM];
    uint8_t key_event = 0;  // 本次要上报给外部的按键值

    /* 1. 读取当前 4 个按键的物理电平 */
    Key_ReadLevels(levels);

    /* 2. 依次处理每一个按键 */
    for (int i = 0; i < KEY_NUM; i++)
    {
        KeyFilter_t *k = &s_Keys[i];
        uint8_t level   = levels[i];

        /* ---------- 2.1 处理电平变化与消抖 ---------- */

        if (level != k->last_level)
        {
            /* 本次采样与上一次不同：可能是按下/松开/抖动 */
            k->last_level   = level;
            k->debounce_cnt = 0;     // 重新开始消抖计数
            k->hold_ticks   = 0;     // 时间计数清零
            // long_mode 先不改，只有进入稳定按下时才会重新判断
        }
        else
        {
            /* 电平和上次一致，开始累计“稳定次数” */
            if (k->debounce_cnt < 0xFF)
            {
                k->debounce_cnt++;
            }

            /* 当连续相同电平达到 KEY_DEBOUNCE_TICKS 次，认为电平稳定 */
            if (k->debounce_cnt == KEY_DEBOUNCE_TICKS)
            {
                /* 只在“稳定电平”发生改变时处理按下/松开事件 */
                if (level != k->stable_level)
                {
                    k->stable_level = level;

                    if (level == 0)
                    {
                        /* --- 稳定按下（按键刚被按下）--- */
                        k->hold_ticks = 0;
                        k->long_mode  = 0;
                        /* 在这里我们不立即上报事件，
                           等松开时决定是不是“短按”，
                           或者在 hold 阶段进入长按模式。 */
                    }
                    else
                    {
                        /* --- 稳定松开（按键刚被释放）--- */
                        if (!k->long_mode)
                        {
                            /* 在未进入 long_mode 的情况下，
                               按下再松开视为“短按”事件。 */
                            if (key_event == 0)
                            {
                                key_event = s_KeyCodeMap[i];
                            }
                        }

                        /* 不管是不是长按过，松开后都重置这些量 */
                        k->hold_ticks = 0;
                        k->long_mode  = 0;
                    }
                }
            }

            /* ---------- 2.2 在“稳定按下”状态下处理长按逻辑 ---------- */

            if (k->stable_level == 0 && k->debounce_cnt >= KEY_DEBOUNCE_TICKS)
            {
                /* 按住期间，每次扫描累加一次计数：
                 * 你的主循环是 Delay_ms(10)，所以这里每次加 1 ≈ 10ms
                 */
                if (k->hold_ticks < 0xFFFF)
                {
                    k->hold_ticks++;
                }

                /* 未进入长按模式时，先等待达到“长按起点” */
                if (!k->long_mode)
                {
                    if (k->hold_ticks >= KEY_LONG_START_TICKS)
                    {
                        k->long_mode  = 1;
                        k->hold_ticks = 0;   // 重新开始用于“连发周期”的计数

                        /* 第一次进入长按模式时，只对 UP/DOWN，并且在 PID 参数编辑界面下触发 */
                        if (menu_state == 2 && edit_mode == 1 &&
                            (i == KEY_IDX_UP || i == KEY_IDX_DOWN))
                        {
                            if (key_event == 0)
                            {
                                key_event = s_KeyCodeMap[i];
                            }
                        }
                    }
                }
                else
                {
                    /* 已经是长按模式：每隔 KEY_LONG_REPEAT_TICKS 再触发一次 */
                    if (k->hold_ticks >= KEY_LONG_REPEAT_TICKS)
                    {
                        k->hold_ticks = 0;

                        if (menu_state == 2 && edit_mode == 1 &&
                            (i == KEY_IDX_UP || i == KEY_IDX_DOWN))
                        {
                            if (key_event == 0)
                            {
                                key_event = s_KeyCodeMap[i];
                            }
                        }
                    }
                }
            }
        }
    }

    return key_event;
}

/*------------------------------------------------
 *  兼容接口：如果工程里还有地方调用 Key_ScanWithDelay，
 *  可以直接用它转调 Key_Scan，避免到处改代码。
 *------------------------------------------------*/
uint8_t Key_ScanWithDelay(void)
{
    return Key_Scan();
}
