#include "Pwm.h"
/**
*说明：CNT>CCR时有效 此时输出低电平 因为输出极性为低
*因为没有等于 只是大于 所以设置占空比时应注意加1
*使用复用GPIO时 一定都要使能GPIO和AFIO时钟!
*
*/
void pwmInit(u16 arr,u16 psc){
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	//1.使能时钟 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //使能定时器 3 时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE); //使能 GPIOB 和复用时钟
	//2.重映射 部分映射到PB5作为CH2
	GPIO_PinRemapConfig(GPIO_PartialRemap_TIM3, ENABLE); //重映射 TIM3_CH2->PB5
	//3.配置GPIO作为PWM输出口
	//设置该引脚为复用输出功能,输出 TIM3 CH2 的 PWM 脉冲波形 GPIOB.5
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //TIM_CH2
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; //复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure); //初始化 GPIO
	//4.配置定时器3
	//初始化 TIM3
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在自动重装载周期值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM 向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //初始化 TIMx
	//5.初始化 TIM3 Channel2 PWM 模式
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2; //选择 PWM 模式 2 
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low; //输出极性低
	TIM_OC2Init(TIM3, &TIM_OCInitStructure); //初始化外设 TIM3 OC2
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable); //使能预装载寄存器
	TIM_Cmd(TIM3, ENABLE); //使能 TIM3
}
