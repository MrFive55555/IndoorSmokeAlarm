#ifndef BUTTON_H
#define BUTTON_H
#include "stm32f10x.h"
#define KEY_SET GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0)
#define KEY_ADD GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)
#define KEY_MIN GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_10)
#define HAS_BUTTON_PRESS (KEY_SET==0||KEY_ADD==0||KEY_MIN==0)
#define NO_BUTTON_PRESS KEY_SET==1&&KEY_ADD==1&&KEY_MIN==1
#define KEY_SET_PRESS 1 //KEY0
#define KEY_ADD_PRESS 2 //KEY1
#define KEY_MIN_PRESS 3 //KEY2
#define NO_PRESS 0
void buttonInit(void);
u8 scanButton(u8 buttonMode);
#endif
