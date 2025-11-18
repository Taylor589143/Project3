#include "key.h"
#include "delay.h"

/**
  * @brief  按键初始化
  */
void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = KEY_UP_PIN | KEY_DOWN_PIN | KEY_OK_PIN | KEY_BACK_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(KEY_PORT, &GPIO_InitStructure);
}

/**
  * @brief  按键扫描
  * @retval 按键值
  */
uint8_t Key_Scan(void)
{
    static uint8_t key_up = 1;
    
    if(key_up && (GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN)==0 || 
                  GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN)==0 ||
                  GPIO_ReadInputDataBit(KEY_PORT, KEY_OK_PIN)==0 ||
                  GPIO_ReadInputDataBit(KEY_PORT, KEY_BACK_PIN)==0))
    {
        delay_ms(10);
        key_up = 0;
        if(GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN)==0) return KEY_UP;
        else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN)==0) return KEY_DOWN;
        else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_OK_PIN)==0) return KEY_OK;
        else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_BACK_PIN)==0) return KEY_BACK;
    }
    else if(GPIO_ReadInputDataBit(KEY_PORT, KEY_UP_PIN)==1 && 
            GPIO_ReadInputDataBit(KEY_PORT, KEY_DOWN_PIN)==1 &&
            GPIO_ReadInputDataBit(KEY_PORT, KEY_OK_PIN)==1 &&
            GPIO_ReadInputDataBit(KEY_PORT, KEY_BACK_PIN)==1)
    {
        key_up = 1;
    }
    return 0;
}

/**
  * @brief  带延时的按键扫描（支持长按）
  * @retval 按键值
  */
uint8_t Key_ScanWithDelay(void)
{
    uint8_t key = Key_Scan();
    static uint32_t press_time = 0;
    
    if(key)
    {
        press_time++;
        if(press_time > 100) // 长按检测
        {
            press_time = 90; // 防止溢出
            return key;
        }
    }
    else
    {
        press_time = 0;
    }
    return 0;
}