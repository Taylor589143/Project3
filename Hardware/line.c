#include "line.h"
#include "Sensor.h"
#include "Motor.h"
#include "Delay.h"
#include "Serial.h"
#include <stdio.h>

/* ==========================================================
 *  全局变量定义
 * ========================================================== */
unsigned char lukou_num        = 0;   // 十字路口计数
unsigned char last_line_status = 0;   // 简单记录“偏左/偏右/居中”，调试用
unsigned int  straight_count   = 0;   // 连续直线计数


//速度调节

int STRAIGHT_SPEED   = 100;    // 默认直线速度，可在菜单调整
int CORNER_SPEED_MIN = 42;     // 默认弯道最低速度
float SPEED_K        = 9.5f;   // 默认弯道降速系数



/* 上一帧线的位置，用于丢线后“朝上一次方向找线” */
static float last_position = 0.0f;

/* 全黑计数（>3 秒停车） */
static uint16_t all_black_cnt = 0;

/* 上一次是否在十字（防止每帧都 ++） */
static uint8_t last_is_cross = 0;

/* PID 内部状态（也可以只用 line_pid 里的 integral/last_error） */
static float pid_integral   = 0.0f;
static float pid_last_error = 0.0f;

/* 一个简单的停电机封装，代替示例里的 Motor_Stop() */
static void Motor_Stop(void)
{
    motor(0, 0);
}

/* ==========================================================
 *  通过权重计算线的位置
 *  @return 线的“相对位置”：
 *          正常 -5 ~ +5 左负右正，0 居中
 * ========================================================== */
static float Calculate_Line_Position(void)
{
    float   weight_sum = 0.0f;
    uint8_t active_cnt = 0;

    /* 传感器在黑线上为 1，在白底为 0
     * 从左到右依次是：L2, L1, M, R1, R2
     */
    if (L2) { weight_sum += WEIGHT_L2; active_cnt++; }
    if (L1) { weight_sum += WEIGHT_L1; active_cnt++; }
    if (M)  { weight_sum += WEIGHT_M;  active_cnt++; }
    if (R1) { weight_sum += WEIGHT_R1; active_cnt++; }
    if (R2) { weight_sum += WEIGHT_R2; active_cnt++; }

    /* 有探头在黑线上：返回加权平均位置 */
    if (active_cnt > 0)
    {
        last_position = weight_sum / active_cnt;   // -5 ~ +5 左负右正
        return last_position;
    }

    /* ======== 全白（丢线）：根据上一帧方向找线 ======== */
    if (last_position > 0.0f)
    {
        /* 上一次线在右边，往右找：比最右的权重再往右一点 */
        return WEIGHT_R2 + 1.0f;
    }
    else if (last_position < 0.0f)
    {
        /* 上一次线在左边，往左找 */
        return WEIGHT_L2 - 1.0f;
    }
    else
    {
        /* 实在不知道，先当居中 */
        return 0.0f;
    }
}

/* ==========================================================
 *  循迹系统初始化
 * ========================================================== */
void Track_Init(void)
{
    lukou_num        = 0;
    last_line_status = 0;
    straight_count   = 0;

    last_position    = 0.0f;
    all_black_cnt    = 0;
    last_is_cross    = 0;

    pid_integral     = 0.0f;
    pid_last_error   = 0.0f;

    PID_Reset(&line_pid);

    printf("Track System Initialized, STRAIGHT_SPEED=%d\r\n", STRAIGHT_SPEED);
}

/* ==========================================================
 *  十字路口处理：这里只负责“计数 + 重置 PID”
 *  ——速度怎么走，交给 PID 统一控制
 * ========================================================== */
void Handle_Crossroad(void)
{
    lukou_num++;
    PID_Reset(&line_pid);
    pid_integral   = 0.0f;
    pid_last_error = 0.0f;

    printf("Crossroad passed, count=%d\r\n", lukou_num);
}

/* ==========================================================
 *  急弯处理（示例接口，核心逻辑交给权重 + PID）
 *  这里不再做“固定延时大转弯”，避免冲出赛道
 * ========================================================== */
void Handle_Sharp_Turn(void)
{
    /* 新算法里，急弯情况会自然体现为“偏差很大”，
     * 通过 PID 和动态降速处理，不再需要额外强行 Delay。
     * 这个函数保留只是为了兼容原来的接口，有需要可以在这里加日志。
     */
}

/* ==========================================================
 *  高级循迹函数 - 权重 + PID，带十字计数 + 全黑保护
 * ========================================================== */
void Advanced_Tracking(void)
{
    /* 1. 读取传感器状态 */
    Sensor_Read();

    uint8_t sL2 = (L2 != 0);
    uint8_t sL1 = (L1 != 0);
    uint8_t sM  = (M  != 0);
    uint8_t sR1 = (R1 != 0);
    uint8_t sR2 = (R2 != 0);

    uint8_t sensor_sum = sL2 + sL1 + sM + sR1 + sR2;

    /* -------- 2. 全黑保护（>3 秒停车） --------
     * 说明：你的红外是“黑线=1，白底=0”；
     *      如果 5 个探头全为 1，说明整块都黑了，
     *      可能是停车线 / 完全跑飞，3 秒后直接停车保护。
     */
    uint8_t all_black = (sensor_sum == 5);

    if (all_black)
    {
        if (all_black_cnt < 0xFFFF)
        {
            all_black_cnt++;
        }
    }
    else
    {
        all_black_cnt = 0;
    }

    if (all_black_cnt > ALL_BLACK_LIMIT)
    {
        Motor_Stop();
        printf("All-black > 3s, stop for safety.\r\n");
        return;
    }

    /* -------- 3. 十字路口检测（>=3 个探头在黑线上） -------- */
    if (sensor_sum >= 3)
    {
        if (!last_is_cross)
        {
            /* 从“非十字”进入“十字”的瞬间，算通过一个十字 */
            Handle_Crossroad();
        }
        last_is_cross = 1;
    }
    else
    {
        last_is_cross = 0;
    }

    /* -------- 4. 权重 + PID 计算偏差 -------- */
    /* 计算线的位置（-5 ~ +5），再减去零点偏置 */
    float position = Calculate_Line_Position() - POSITION_OFFSET;

    /* 这里把 setpoint 和 current 对调，直接让 error = position
     * 即：希望 position -> 0，所以用 PID_Calculate(&pid, pos, 0)
     */
    float adjust = PID_Calculate(&line_pid, position, 0.0f);

    /* -------- 5. 根据偏差大小动态降速：弯道减速，直线全速 -------- */
    float abs_pos = position;
    if (abs_pos < 0.0f) abs_pos = -abs_pos;

    int base_speed = STRAIGHT_SPEED - (int)(SPEED_K * abs_pos);
    if (base_speed < CORNER_SPEED_MIN) base_speed = CORNER_SPEED_MIN;
    if (base_speed > STRAIGHT_SPEED)   base_speed = STRAIGHT_SPEED;

    /* -------- 6. 计算左右轮速度（左加右减） --------
     * position > 0 => 线在右边 => adjust > 0 =>
     *   left = base + adjust, right = base - adjust
     *   => 左轮快，车向右拐回到线附近。
     */
    int left_speed  = base_speed + (int)adjust;
    int right_speed = base_speed - (int)adjust;

    /* -------- 7. 限幅：只允许正转（>=0），不允许倒车 -------- */
    if (left_speed > MAX_SPEED)   left_speed = MAX_SPEED;
    if (right_speed > MAX_SPEED)  right_speed = MAX_SPEED;
    if (left_speed < MIN_SPEED)   left_speed = MIN_SPEED;
    if (right_speed < MIN_SPEED)  right_speed = MIN_SPEED;

    /* -------- 8. 设置电机速度 -------- */
    motor(left_speed, right_speed);

    /* -------- 9. 辅助状态记录（调试用） -------- */
    /* 简单把 position 映射成 -2/0/2 之类，用来观察“偏左/偏右” */
    if (position < -1.5f)
        last_line_status = 1;     // 偏左
    else if (position > 1.5f)
        last_line_status = 2;     // 偏右
    else
        last_line_status = 5;     // 大致居中

    /* 直线计数（绝对偏差很小就认为在直线段） */
    if (abs_pos < 1.0f)
    {
        straight_count++;
    }
    else
    {
        straight_count = 0;
    }
}

/* ==========================================================
 *  兼容接口：Track_Straight_Line / Track_With_PID
 *  都直接调用 Advanced_Tracking 即可
 * ========================================================== */
void Track_Straight_Line(void)
{
    Advanced_Tracking();
}

void Track_With_PID(int base_speed, float kp, float ki, float kd)
{
    (void)base_speed;
    (void)kp;
    (void)ki;
    (void)kd;

    Advanced_Tracking();
}

/* ==========================================================
 *  一些辅助 / 调试函数
 * ========================================================== */
unsigned char Get_Crossroad_Count(void)
{
    return lukou_num;
}

void Track_Reset(void)
{
    Track_Init();
    printf("Track System Reset\r\n");
}

void Track_Debug_Output(void)
{
    /* 这里只是简单示例，你也可以把 position 打印出来看 */
    printf("Track Debug - Cross=%d, StraightCnt=%d, LastStatus=%d\r\n",
           lukou_num, straight_count, last_line_status);
}










