#include "stm32f10x.h"
#include "Sensor.h"
#include "Delay.h"
#include "Serial.h"     

/* ==========================================================
 *  全局变量：当前 5 个红外的状态
 *
 *  约定：0 = 检测到白色（灯亮，输出低电平）
 *        1 = 检测到黑线（灯灭，输出高电平）
 * ========================================================== */

int L2 = 0;
int L1 = 0;
int M  = 0;
int R1 = 0;
int R2 = 0;

/*------------------------------------------------
 *  传感器 GPIO 初始化
 *------------------------------------------------*/
void Sensor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 打开 GPIOA 和 AFIO 时钟
     *    AFIO 用来关闭 JTAG，释放 PA15 作为普通 IO
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* 2. 禁用 JTAG，保留 SWD：这样 PA15 / PB3 / PB4 可以当普通 IO 用 */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* 3. 把 PA0/PA1/PA4/PA5/PA15 配置为上拉输入 */
    GPIO_InitStructure.GPIO_Pin   = L2_PIN | L1_PIN | M_PIN | R1_PIN | R2_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;       // 上拉输入：松开时为 1
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* 不需要串口的话可以把 printf 整行删掉 */
    printf("IR Sensors OK: PA0,PA1,PA4,PA5,PA15\r\n");
}

/*------------------------------------------------
 *  读取 5 个红外传感器的当前值
 *  调用后会更新全局变量 L2/L1/M/R1/R2
 *------------------------------------------------*/
void Sensor_Read(void)
{
    /* 注意：0 = 白色地面，1 = 黑色线条 */
    L2 = GPIO_ReadInputDataBit(L2_PORT, L2_PIN);
    L1 = GPIO_ReadInputDataBit(L1_PORT, L1_PIN);
    M  = GPIO_ReadInputDataBit(M_PORT,  M_PIN);
    R1 = GPIO_ReadInputDataBit(R1_PORT, R1_PIN);
    R2 = GPIO_ReadInputDataBit(R2_PORT, R2_PIN);
}

/*------------------------------------------------
 *  获取循线状态（主状态码）
 *
 *  返回值说明：
 *   1 : 10000 - 严重偏左
 *   2 : 11000 - 明显偏左
 *   3 : 01000 - 轻微偏左
 *   4 : 01100 - 正常偏左
 *   5 : 00100 - 居中
 *   6 : 00110 - 正常偏右
 *   7 : 00010 - 轻微偏右
 *   8 : 00011 - 明显偏右
 *   9 : 00001 - 严重偏右
 *  10 : 00000 - 全白（丢线）
 *  11 : 11111 - 全黑（停车线/大十字）
 *   0 : 其他组合（保底）
 *------------------------------------------------*/
uint8_t Get_Line_Status(void)
{
    /* 先刷新一次传感器数据，确保是最新的 */
    Sensor_Read();

    /* 先判全白和全黑两种极端情况 */
    if (L2 == 0 && L1 == 0 && M == 0 && R1 == 0 && R2 == 0)
    {
        return 10;     // 全白：丢线
    }
    if (L2 == 1 && L1 == 1 && M == 1 && R1 == 1 && R2 == 1)
    {
        return 11;     // 全黑：停车线或十字
    }

    /* 从左到右：L2, L1, M, R1, R2 */
    if (L2 == 1 && L1 == 0 && M == 0 && R1 == 0 && R2 == 0)
        return 1;      // 10000 - 严重偏左
    else if (L2 == 1 && L1 == 1 && M == 0 && R1 == 0 && R2 == 0)
        return 2;      // 11000 - 偏左
    else if (L2 == 0 && L1 == 1 && M == 0 && R1 == 0 && R2 == 0)
        return 3;      // 01000 - 轻微偏左
    else if (L2 == 0 && L1 == 1 && M == 1 && R1 == 0 && R2 == 0)
        return 4;      // 01100 - 正常偏左
    else if (L2 == 0 && L1 == 0 && M == 1 && R1 == 0 && R2 == 0)
        return 5;      // 00100 - 正中
    else if (L2 == 0 && L1 == 0 && M == 1 && R1 == 1 && R2 == 0)
        return 6;      // 00110 - 正常偏右
    else if (L2 == 0 && L1 == 0 && M == 0 && R1 == 1 && R2 == 0)
        return 7;      // 00010 - 轻微偏右
    else if (L2 == 0 && L1 == 0 && M == 0 && R1 == 1 && R2 == 1)
        return 8;      // 00011 - 偏右
    else if (L2 == 0 && L1 == 0 && M == 0 && R1 == 0 && R2 == 1)
        return 9;      // 00001 - 严重偏右
    else
        return 0;      // 其他组合：比如十字路口过渡状态
}

/*------------------------------------------------
 *  十字路口判定
 *
 *  简单策略：统计有多少个传感器踩在黑线上；
 *  若数量 >= 3，则认为是“十字、T 字”一类的宽线区域。
 *------------------------------------------------*/
uint8_t Detect_Crossroad(void)
{
    uint8_t sensor_count = 0;

    if (L2 == 1) sensor_count++;
    if (L1 == 1) sensor_count++;
    if (M  == 1) sensor_count++;
    if (R1 == 1) sensor_count++;
    if (R2 == 1) sensor_count++;

    if (sensor_count >= 3)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*------------------------------------------------
 *  串口调试输出（可选）
 *  - 打印当前五个传感器的值和循迹状态
 *------------------------------------------------*/
void Sensor_Debug_Output(void)
{
    uint8_t status;

    Sensor_Read();

    printf("IR: %d%d%d%d%d (L2,L1,M,R1,R2)\r\n",
           L2, L1, M, R1, R2);

    status = Get_Line_Status();

    switch (status)
    {
    case 1:  printf("Status: Severe Left\r\n");      break;
    case 2:  printf("Status: Left\r\n");             break;
    case 3:  printf("Status: Slight Left\r\n");      break;
    case 4:  printf("Status: Normal Left\r\n");      break;
    case 5:  printf("Status: Center\r\n");           break;
    case 6:  printf("Status: Normal Right\r\n");     break;
    case 7:  printf("Status: Slight Right\r\n");     break;
    case 8:  printf("Status: Right\r\n");            break;
    case 9:  printf("Status: Severe Right\r\n");     break;
    case 10: printf("Status: Line Lost\r\n");        break;
    case 11: printf("Status: All Black / Stop\r\n"); break;
    default: printf("Status: Unknown\r\n");          break;
    }

    if (Detect_Crossroad())
    {
        printf("Crossroad: YES\r\n");
    }
    else
    {
        printf("Crossroad: NO\r\n");
    }
}

/*------------------------------------------------
 *  打包原始 5 位数据
 *  bit4~bit0: L2,L1,M,R1,R2
 *------------------------------------------------*/
uint8_t Sensor_Get_Raw_Data(void)
{
    Sensor_Read();
    return (uint8_t)((L2 << 4) |
                     (L1 << 3) |
                     (M  << 2) |
                     (R1 << 1) |
                     (R2 << 0));
}

/*------------------------------------------------
 *  是否“至少有一个传感器踩在黑线上”
 *------------------------------------------------*/
uint8_t Sensor_Is_On_Line(void)
{
    Sensor_Read();

    if (L2 == 1 || L1 == 1 || M == 1 || R1 == 1 || R2 == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
