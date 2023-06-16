#include "Time.h"
#include "Led.h"
#include "buzzer.h"
//使用main函数定义的变量
extern float adcVoltage;
//通用定时器3的初始化
void time3Init(u16 arr,u16 psc){
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	//1.使能定时器3时钟 APB1时钟线上 36MHZ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	//2.定时器 TIM3 初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler = psc; //设置时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM 向上计数
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //初始化 TIM3                                                  
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //允许更新中断
	//3.定时器中断配置
	//中断优先级 NVIC 设置 分组2(放主函数了 只需要调用一次)
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置 NVIC 中断分组 2
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn; //TIM3 中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //抢占优先级 0 级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3; //从优先级 3 级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ 通道被使能
	NVIC_Init(&NVIC_InitStructure); //初始化 NVIC 寄存器
	TIM_Cmd(TIM3, ENABLE); //使能 TIM3
	/**
	* 溢出时间计算公式：
	* Tout= ((arr+1)*(psc+1))/Tclk;
	*/
	/**
	*计算步骤分解：
	*1.每个计数值周期：Tm = 1/(APB1*(分频)/(PSC+1))  PSC范围(0-65535) 
	*因为计数到65536才溢出 所以要PSC+1才是整个计数周期
	*2.总计数周期: T = Tm*(ARR+1) = (ARR+1)/(APB1*(分频)/(PSC+1))  //ARR自动重载计数值(最大值)
	*							  = (ARR+1)*(PSC+1)/(APB1*(分频))
	*/
}

//TIM3中断函数
void TIM3_IRQHandler(void){
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) //检查 TIM3 更新中断发生与否
	{
		
		//printf和lcdshowString均会使程序卡死
		/**
		*原因可能在于执行时间过长
		1.将led的转换放在中断执行
		*/
		if(adcVoltage > 0.8){
			buzzer = 0;
			if(LED1==0){
				LED1 = 1;
				LED2 = 0;
			}else {
				LED1 = 1;
				LED2 = 0;
			}
		}else{
			buzzer = 1;
			LED1 = 0;
			LED2 = 0;
		}
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update); //清除 TIM3 更新中断标志
}
