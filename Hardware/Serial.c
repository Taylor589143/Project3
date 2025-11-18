#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "PID.h"

extern int16_t target_speed;   // 目标速度（外部变量）

/* ==========================================================
 * 串口模块 Serial.c
 * 功能：与上位机进行数据通信
 * - 发送电机速度与目标值
 * - 接收上位机控制命令
 * 
 * ★保持原有功能完全一致，仅优化结构与注释。
 * ========================================================== */


/**
 * @brief 串口初始化函数（USART1, 115200bps）
 * 
 * 配置说明：
 *  - PA9  → TX（推挽复用输出）
 *  - PA10 → RX（上拉输入）
 *  - 无硬件流控，8位数据，1位停止，无校验
 *  - 开启接收中断（USART_IT_RXNE）
 */
void Serial_Init(void)
{
    // ★新增注释：开启USART1与GPIOA时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);  // ★修改：合并两句时钟配置

    GPIO_InitTypeDef GPIO_InitStructure;

    // 配置 TX (PA9)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 RX (PA10)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 串口参数配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    // 开启接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // NVIC 配置（中断优先级）
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_Init(&NVIC_InitStructure);

    // 使能串口
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief USART1 中断处理函数
 * 
 * 功能：解析上位机发来的速度控制指令。
 * 支持命令格式：@speed%数值
 * 示例：
 *     @speed%100   → 设置目标速度为100
 */
void USART1_IRQHandler(void)
{
    // 静态变量：用于存储一条完整命令
    static char cmd_buffer[32];       // 命令接收缓冲
    static uint8_t cmd_index = 0;     // 缓冲写入位置
    static uint8_t receiving_cmd = 0; // 是否正在接收命令

    // 检查中断标志
    if (USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        char received_char = USART_ReceiveData(USART1); // 读取接收到的字符

        // 命令起始符检测
        if (received_char == '@')
        {
            receiving_cmd = 1;
            cmd_index = 0;
            cmd_buffer[cmd_index++] = received_char;
        }
        else if (receiving_cmd)
        {
            // ★修改：合并判断逻辑，增强健壮性
            if (received_char == '\r' || received_char == '\n')
            {
                if (cmd_index > 0)
                {
                    cmd_buffer[cmd_index] = '\0'; // 封闭字符串

                    // 匹配 "@speed%" 指令
                    if (strncmp(cmd_buffer, "@speed%", 7) == 0)
                    {
                        char *num_str = cmd_buffer + 7; // 提取数值部分
                        if (*num_str != '\0')
                        {
                            target_speed = atoi(num_str);  // 更新目标速度
                            Speed_PID_Reset();             // ★修改：保持快速响应逻辑
                        }
                    }
                }
                receiving_cmd = 0;
                cmd_index = 0;
            }
            else if (cmd_index < sizeof(cmd_buffer) - 1)
            {
                cmd_buffer[cmd_index++] = received_char;
            }
            else
            {
                // 缓冲区溢出保护
                receiving_cmd = 0;
                cmd_index = 0;
            }
        }

        // 清除中断标志位
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}


/**
 * @brief printf重定向函数
 * 
 * 说明：将标准输出（printf）映射到USART1发送端
 */
int fputc(int ch, FILE *f)
{
    USART_SendData(USART1, (uint8_t)ch);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    return ch;
}


/**
 * @brief 发送调试数据到上位机
 * 
 * 格式：<当前速度,目标速度>\n
 * 例如：120,100\n
 */
void USART_Send_Data(int16_t speed1, int16_t target_speed)
{
    printf("%d,%d\n", speed1, target_speed);
}
