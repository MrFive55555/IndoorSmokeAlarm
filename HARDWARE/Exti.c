#include "Exti.h"
#include "delay.h"
#include "Button.h"
#include "buzzer.h"
#include "Iwdg.h"
void extiInit(void){
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	//初始化GPIO 配置相应的触发前的输入模式
	buttonInit();
	//1.使能AFIO时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE); 
	//2.GPIOA.5 中断线以及中断初始化配置
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA,GPIO_PinSource5);
	EXTI_InitStructure.EXTI_Line=EXTI_Line5;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发
	EXTI_Init(&EXTI_InitStructure); //初始化 EXTI 寄存器
	//3.中断配置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置 NVIC 中断分组 2
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn; //使能按键外部中断通道 GPIO5-9共用一个中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02; //抢占优先级 2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00; //子优先级 0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);//初始化 NVIC
}

//4.中断处理函数
/*void EXTI9_5_IRQHandler(void)
{
	delay_ms(10);//消抖
	if(KEY2==1&&EXTI_GetITStatus(EXTI_Line5)!=RESET) //按键 KEY2
	{
		buzzer = ~buzzer;
		
	}
	EXTI_ClearITPendingBit(EXTI_Line5); //清除 LINE5 上的中断标志位 
}*/
void EXTI9_5_IRQHandler(void)
{
//	delay_ms(10);//消抖
//	if(KEY2==1&&EXTI_GetITStatus(EXTI_Line5)!=RESET) //按键 KEY2
//	{
//		//喂狗
//		iwdgFeed();
//	}
//	EXTI_ClearITPendingBit(EXTI_Line5); //清除 LINE5 上的中断标志位 
}
