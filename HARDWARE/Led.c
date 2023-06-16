#include "Led.h"
void ledInit(void){
    //初始化与LED相关联的GPIO
    GPIO_InitTypeDef GPIO_InitStruct;
	//使能IO口 这一点一定不要忘了!
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA,&GPIO_InitStruct);
}
