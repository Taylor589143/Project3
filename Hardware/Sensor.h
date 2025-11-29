#ifndef __SENSOR_H
#define __SENSOR_H

#include "stm32f10x.h"

/* ==========================================================
 *  红外循迹传感器引脚定义（从左到右）
 *
 *   L2 : PA0  - 最左
 *   L1 : PA1  - 左
 *   M  : PA4  - 中
 *   R1 : PA5  - 右
 *   R2 : PA15 - 最右（注意：使用 PA15，避免和编码器 PA8/PA9 冲突）
 * ========================================================== */

#define L2_PIN   GPIO_Pin_0
#define L2_PORT  GPIOA

#define L1_PIN   GPIO_Pin_1
#define L1_PORT  GPIOA

#define M_PIN    GPIO_Pin_4
#define M_PORT   GPIOA

#define R1_PIN   GPIO_Pin_5
#define R1_PORT  GPIOA

#define R2_PIN   GPIO_Pin_15
#define R2_PORT  GPIOA

#define SENSOR_NUM 5

/* 方便调试用的状态结构体（目前没强制用，用不上可以不管） */
typedef struct
{
    uint8_t L2;
    uint8_t L1;
    uint8_t M;
    uint8_t R1;
    uint8_t R2;
    uint8_t raw;    // 打包后的 5 位原始数据
} Sensor_State;


extern int L2, L1, M, R1, R2;

/* ------------ 传感器基础接口 ------------ */
void Sensor_Init(void);      // 初始化 GPIO 等
void Sensor_Read(void);      // 刷新 L2/L1/M/R1/R2 值

/* ------------ 巡线状态接口（主控逻辑会用到） ------------ */
/**
 * @brief 获取循线状态
 * @return 状态码：
 *   1~9  : 不同偏差方向（严重左、轻微右……）
 *   10   : 全白（丢线）
 *   11   : 全黑（停车线 / 十字路口）
 *   0    : 其他组合（未知）
 *
 * 说明：函数内部会主动调用 Sensor_Read() 刷新一次传感器。
 */
uint8_t Get_Line_Status(void);

/**
 * @brief 检测是否为十字路口（辅助用）
 * @return 1 = 判定为十字路口，0 = 非十字
 */
uint8_t Detect_Crossroad(void);

/* ------------ 调试 / 辅助接口（可选用） ------------ */

/**
 * @brief 通过串口打印当前传感器状态和循迹状态
 */
void Sensor_Debug_Output(void);

/**
 * @brief 获取原始 5 位数据
 * @return bit4~bit0 分别对应 L2,L1,M,R1,R2
 */
uint8_t Sensor_Get_Raw_Data(void);

/**
 * @brief 判断当前是否至少有一个传感器踩在黑线上
 * @return 1 = 在线上，0 = 全白
 */
uint8_t Sensor_Is_On_Line(void);

#endif
