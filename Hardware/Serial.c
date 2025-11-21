#include "stm32f10x.h"
#include "Serial.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "PID.h"

/* 来自其他模块的目标速度，由串口命令修改 */
extern int16_t target_speed;

/*==================== 硬件相关宏定义 ====================*/
/* 串口：这里使用 USART1，如需改成 USART2 等，只改这些宏即可 */
#define SERIAL_USART              USART1
#define SERIAL_GPIO_PORT          GPIOA
#define SERIAL_TX_PIN             GPIO_Pin_9      /* PA9  → TX  */
#define SERIAL_RX_PIN             GPIO_Pin_10     /* PA10 → RX  */

#define SERIAL_GPIO_CLK           RCC_APB2Periph_GPIOA
#define SERIAL_USART_CLK          RCC_APB2Periph_USART1

/* 波特率设置 */
#define SERIAL_BAUDRATE           115200u

/* 串口命令接收缓冲区大小（字节） */
#define SERIAL_CMD_BUF_SIZE       32

/*========================================================
 * 函数名：Serial_Init
 * 作用  ：初始化串口 USART1，用于与上位机通信
 *
 * 硬件连接：
 *   PA9  → USART1_TX  （复用推挽输出）
 *   PA10 → USART1_RX  （上拉输入 / 浮空输入均可）
 *
 * 串口配置：
 *   波特率：115200
 *   数据位：8 位
 *   停止位：1 位
 *   校  验：无
 *   流  控：无
 *   中  断：开启接收中断 USART_IT_RXNE
 *=======================================================*/
void Serial_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    /* 1. 打开 GPIOA 和 USART1 的时钟 */
    RCC_APB2PeriphClockCmd(SERIAL_GPIO_CLK | SERIAL_USART_CLK, ENABLE);

    /* 2. 配置 TX 引脚：PA9 复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = SERIAL_TX_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(SERIAL_GPIO_PORT, &GPIO_InitStructure);

    /* 3. 配置 RX 引脚：PA10 上拉输入（也可以改成浮空输入） */
    GPIO_InitStructure.GPIO_Pin  = SERIAL_RX_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(SERIAL_GPIO_PORT, &GPIO_InitStructure);

    /* 4. 配置串口参数 */
    USART_InitStructure.USART_BaudRate            = SERIAL_BAUDRATE;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(SERIAL_USART, &USART_InitStructure);

    /* 5. 使能接收中断：当收到一个字节时触发 */
    USART_ITConfig(SERIAL_USART, USART_IT_RXNE, ENABLE);

    /* 6. 配置 NVIC：设置 USART1 中断的优先级并使能 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);       // 优先级分组（全工程中统一即可）

    NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;   // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;   // 子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* 7. 最后打开串口 */
    USART_Cmd(SERIAL_USART, ENABLE);
}

/*========================================================
 * 函数名：USART1_IRQHandler
 * 作用  ：串口接收中断服务函数
 *
 * 协议约定（简单文本协议）：
 *   以 '@' 开头，以 '\r' 或 '\n' 结尾。
 *   当前只支持一种命令：
 *      @speed%123
 *   含义：将 target_speed 设置为 123
 *
 * 解析思路：
 *   1. 收到 '@' 时，开始一帧新命令，清空缓冲。
 *   2. 中间收到的字符依次存入缓冲。
 *   3. 收到 '\r' 或 '\n' 时，一帧命令结束，进行解析。
 *=======================================================*/
void USART1_IRQHandler(void)
{
    /* 用 static 变量保存状态，这样每次中断进入时不会丢失上一次的数据 */
    static char    rx_buffer[SERIAL_CMD_BUF_SIZE];  // 接收缓冲区
    static uint8_t rx_index   = 0;                  // 当前写入的位置
    static uint8_t rx_inFrame = 0;                  // 标记是否处在一帧命令数据中

    /* 判断是否为“接收寄存器非空”中断 */
    if (USART_GetITStatus(SERIAL_USART, USART_IT_RXNE) != RESET)
    {
        /* 读出收到的一个字节（读之后标志会自动清零） */
        uint8_t ch = (uint8_t)USART_ReceiveData(SERIAL_USART);

        if (ch == '@')
        {
            /* 1. 收到命令起始符：开始新的命令帧 */
            rx_inFrame        = 1;
            rx_index          = 0;
            rx_buffer[rx_index++] = (char)ch;
        }
        else if (rx_inFrame)
        {
            /* 已经处于一帧命令内部 */

            if (ch == '\r' || ch == '\n')
            {
                /* 2. 收到结尾符：命令结束，准备解析 */
                if (rx_index < SERIAL_CMD_BUF_SIZE)
                {
                    rx_buffer[rx_index] = '\0';     // 补上字符串结束符

                    /* === 解析命令 === */

                    /* 判断前缀是否为 "@speed%" */
                    if (strncmp(rx_buffer, "@speed%", 7) == 0)
                    {
                        /* 数字部分从第 7 个字符开始 */
                        char *num_str = rx_buffer + 7;

                        if (*num_str != '\0')      // 确保后面不是空字符串
                        {
                            int value = atoi(num_str);      // 把字符串转换成整数
                            target_speed = (int16_t)value;  // 更新目标速度

                            /* 目标速度改变后，重置速度 PID 状态，避免积分“穿越” */
                            Speed_PID_Reset();
                        }
                    }
                    /* 如果后续要添加其他命令，可以在这里再加 else if(...) */
                }

                /* 不管解析成功与否，这一帧都结束了，准备下一帧 */
                rx_inFrame = 0;
                rx_index   = 0;
            }
            else
            {
                /* 3. 普通字符：写入缓冲区（注意不要越界） */
                if (rx_index < (SERIAL_CMD_BUF_SIZE - 1))
                {
                    rx_buffer[rx_index++] = (char)ch;
                }
                else
                {
                    /* 缓冲区满了，认为这一帧无效，直接丢弃并重置状态 */
                    rx_inFrame = 0;
                    rx_index   = 0;
                }
            }
        }

        /* 手动清除中断挂起标志（更保险） */
        USART_ClearITPendingBit(SERIAL_USART, USART_IT_RXNE);
    }
}

/*========================================================
 * printf 重定向
 *
 * 说明：
 *   有了这个函数以后，在工程里直接使用 printf()
 *   输出的数据就会通过 USART1 发到上位机，方便调试。
 *=======================================================*/
int fputc(int ch, FILE *f)
{
    USART_SendData(SERIAL_USART, (uint8_t)ch);
    /* 等待发送完成，防止数据丢失 */
    while (USART_GetFlagStatus(SERIAL_USART, USART_FLAG_TC) == RESET);
    return ch;
}

/*========================================================
 * 函数名：USART_Send_Data
 * 作用  ：给上位机发送当前速度和目标速度，用于画曲线
 *
 * 参数：
 *   speed_now    —— 当前实际速度
 *   target_speed —— 当前目标速度
 *
 * 数据格式：
 *   发送形如 "120,100\n" 的字符串
 *   上位机按逗号分隔，前一个数画实测曲线，后一个画目标曲线。
 *=======================================================*/
void USART_Send_Data(int16_t speed_now, int16_t target_speed)
{
    printf("%d,%d\n", speed_now, target_speed);
}
