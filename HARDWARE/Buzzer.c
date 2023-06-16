#include "Buzzer.h"
void buzzerInit(){
	/**
	*蜂鸣器模块用的是S8550PNP型三极管，E极接入VCC，而STM32IO口的高电平只有3.3V接入B极，
	*当VCC接入5V时，Ve大于Vb，Veb恒大于0.7v，
	*三极管始终处于导通状态，因此失去控制，一直angang响
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	//使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE); //使能 PA 端口时钟
	//PA2 推挽输出 50MHZ速度
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	buzzer = 0;
	//GPIO_ResetBits(GPIOB,GPIO_Pin_7);
	/**
	*问题:TFT无法显示
	*依次排查 PB8脱焊了！！！
	*/
}
