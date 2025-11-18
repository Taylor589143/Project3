#include "line.h"
#include "Sensor.h"
#include "motor.h"
#include "delay.h"
#include "Serial.h"
#include "pid.h"
#include <stdio.h>

// 全局变量定义
unsigned char lukou_num = 0;
unsigned char last_line_status = 5; // 初始状态设为居中
unsigned int straight_count = 0;

// PID变量
static float pid_integral = 0;
static float pid_last_error = 0;

/**
  * @brief  循迹系统初始化
  */
void Track_Init(void)
{
    lukou_num = 0;
    last_line_status = 5;
    straight_count = 0;
    pid_integral = 0;
    pid_last_error = 0;
    
    printf("Track System Initialized\r\n");
    printf("Straight Speed: %d, Cross Delay: %dms\r\n", STRAIGHT_SPEED, CROSS_DELAY);
}

/**
  * @brief  处理十字路口
  */
void Handle_Crossroad(void)
{
    printf("=== Crossroad Detected! Passing... ===\r\n");
    
    // 直行通过十字路口
    motor(STRAIGHT_SPEED, STRAIGHT_SPEED);
    Delay_ms(CROSS_DELAY);
    
    // 更新路口计数
    lukou_num++;
    
    // 串口输出调试信息
    printf("Crossroad passed! Count: %d\r\n", lukou_num);
    
    // 重置PID参数和计数器
    pid_integral = 0;
    pid_last_error = 0;
    straight_count = 0;
}

/**
  * @brief  处理急弯
  */
void Handle_Sharp_Turn(void)
{
    Sensor_Read(); // 读取最新传感器数据
    
    // 检测急弯条件：只有最外侧传感器检测到黑线
    if((L2 == 1 && L1 == 0 && M == 0 && R1 == 0 && R2 == 0) ||  // 10000 - 左急弯
       (L2 == 0 && L1 == 0 && M == 0 && R1 == 0 && R2 == 1)) {  // 00001 - 右急弯
        
        printf("Sharp turn detected! Handling...\r\n");
        
        if(L2 == 1) { // 左急弯
            motor(25, 70);
            Delay_ms(SHARP_TURN_DELAY);
            printf("Sharp left turn completed\r\n");
        } else { // 右急弯
            motor(70, 25);
            Delay_ms(SHARP_TURN_DELAY);
            printf("Sharp right turn completed\r\n");
        }
        
        // 重置直行计数
        straight_count = 0;
    }
}

/**
  * @brief  直线循迹 (状态机方法)
  */
void Track_Straight_Line(void)
{
    unsigned char line_status = Get_Line_Status();
    
    // 首先检查特殊状况
    if(line_status == 12) { // 十字路口
        Handle_Crossroad();
        return;
    }
    
    if(line_status == 11) { // 停车线
        motor(0, 0);
        printf("Stop line detected! Car stopped.\r\n");
        Delay_ms(2000);
        return;
    }
    
    Handle_Sharp_Turn(); // 检查急弯
    
    // 状态机控制
    switch(line_status) {
        case 1: // 10000 - 严重偏左
            motor(30, 75);
            straight_count = 0;
            //printf("Status: Severe Left\r\n");
            break;
            
        case 2: // 11000 - 偏左
            motor(40, 70);
            straight_count = 0;
            //printf("Status: Left\r\n");
            break;
            
        case 3: // 01000 - 轻微偏左
            motor(45, 65);
            straight_count = 0;
            break;
            
        case 4: // 01100 - 正常偏左
            motor(50, 60);
            straight_count = 0;
            break;
            
        case 5: // 00100 - 居中
            motor(STRAIGHT_SPEED, STRAIGHT_SPEED);
            straight_count++;
            // 连续直行时稍微加速
            if(straight_count > 20) {
                motor(STRAIGHT_SPEED + 10, STRAIGHT_SPEED + 10);
            }
            break;
            
        case 6: // 00110 - 正常偏右
            motor(60, 50);
            straight_count = 0;
            break;
            
        case 7: // 00010 - 轻微偏右
            motor(65, 45);
            straight_count = 0;
            break;
            
        case 8: // 00011 - 偏右
            motor(70, 40);
            straight_count = 0;
            //printf("Status: Right\r\n");
            break;
            
        case 9: // 00001 - 严重偏右
            motor(75, 30);
            straight_count = 0;
            //printf("Status: Severe Right\r\n");
            break;
            
        case 10: // 00000 - 丢失路线
            printf("Line lost! Recovering...\r\n");
            // 基于历史状态的智能恢复
            if(last_line_status <= 4) {
                motor(35, 60); // 上次在左侧，向右找线
            } else if(last_line_status >= 6) {
                motor(60, 35); // 上次在右侧，向左找线
            } else {
                motor(45, 45); // 不确定时低速前进
            }
            straight_count = 0;
            break;
            
        default:
            motor(50, 50); // 默认速度
            break;
    }
    
    last_line_status = line_status;
}

/**
  * @brief  PID循迹控制
  * @param  base_speed: 基础速度
  * @param  kp: 比例系数
  * @param  ki: 积分系数  
  * @param  kd: 微分系数
  */
void Track_With_PID(int base_speed, float kp, float ki, float kd)
{
    unsigned char line_status = Get_Line_Status();
    int error = 0;
    
    // 首先检查特殊状况
    if(line_status == 12) { // 十字路口
        Handle_Crossroad();
        return;
    }
    
    if(line_status == 11) { // 停车线
        motor(0, 0);
        printf("Stop line detected! Car stopped.\r\n");
        Delay_ms(2000);
        return;
    }
    
    Handle_Sharp_Turn(); // 检查急弯
    
    // 根据循线状态计算偏差
    switch(line_status) {
        case 1:  error = -4; break;  // 严重偏左
        case 2:  error = -3; break;  // 偏左
        case 3:  error = -2; break;  // 轻微偏左
        case 4:  error = -1; break;  // 正常偏左
        case 5:  error = 0;  break;  // 居中
        case 6:  error = 1;  break;  // 正常偏右
        case 7:  error = 2;  break;  // 轻微偏右
        case 8:  error = 3;  break;  // 偏右
        case 9:  error = 4;  break;  // 严重偏右
        case 10: // 丢失路线
            // 使用历史偏差
            error = (last_line_status <= 5) ? -3 : 3;
            break;
        default: error = 0; break;
    }
    
    // PID计算
    pid_integral += error;
    
    // 积分限幅
    if(pid_integral > 100) pid_integral = 100;
    if(pid_integral < -100) pid_integral = -100;
    
    float differential = error - pid_last_error;
    
    float adjust = error * kp + pid_integral * ki + differential * kd;
    
    // 针对18mm赛道调整控制力度
    int left_speed = base_speed + adjust;
    int right_speed = base_speed - adjust;
    
    pid_last_error = error;
    
    // 速度限制
    if(left_speed > 100) left_speed = 100;
    if(right_speed > 100) right_speed = 100;
    if(left_speed < 30) left_speed = 30;
    if(right_speed < 30) right_speed = 30;
    
    motor(left_speed, right_speed);
    last_line_status = line_status;
    
    // 调试输出（可注释掉以减少串口负载）
    // printf("PID - Error: %d, Adjust: %.1f, L:%d, R:%d\r\n", 
    //        error, adjust, left_speed, right_speed);
}

/**
  * @brief  高级循迹函数 (推荐使用)
  * @说明：结合状态机和PID，适合18mm赛道
  *       小偏差使用PID精细调整，大偏差使用状态机快速响应
  */
void Advanced_Tracking(void)
{
    unsigned char line_status = Get_Line_Status();
    
    // 优先处理特殊状况
    if(line_status == 12) { // 十字路口
        Handle_Crossroad();
        return;
    }
    
    if(line_status == 11) { // 停车线
        motor(0, 0);
        printf("Stop line detected! Car stopped.\r\n");
        Delay_ms(2000);
        return;
    }
    
    Handle_Sharp_Turn(); // 检查急弯
    
    // 对于小偏差使用PID，大偏差使用状态机
    if(line_status >= 3 && line_status <= 7) {
        // 使用PID进行精细调整 (适合小偏差)
        Track_With_PID(STRAIGHT_SPEED, 3.0, 0.05, 0.8);
    } else {
        // 使用状态机进行大范围调整 (适合大偏差和特殊情况)
        Track_Straight_Line();
    }
}

/**
  * @brief  获取当前路口计数
  */
unsigned char Get_Crossroad_Count(void)
{
    return lukou_num;
}

/**
  * @brief  重置循迹系统
  */
void Track_Reset(void)
{
    Track_Init();
    printf("Track System Reset\r\n");
}

/**
  * @brief  循迹状态调试输出
  */
void Track_Debug_Output(void)
{
    unsigned char line_status = Get_Line_Status();
    
    printf("Track Status: %d, Crossroads: %d, Straight Count: %d\r\n", 
           line_status, lukou_num, straight_count);
}