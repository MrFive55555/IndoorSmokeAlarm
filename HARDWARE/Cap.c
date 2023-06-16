#include "Cap.h"
#include "Usart.h"
/**
 * 说明：
 * 1.PWM捕获模式 使用CH1和CH2 TIM4->PB6 PB7 只要配置一个CH 另一个硬件作相反的自动配置
*/
#define FCLK 72000000
// 保存计数值
u16 IC1Value,IC2Value,PSC,printCount=0;
float duty,freq;
TIM_ICInitTypeDef TIM_ICInitStructure;
void capInit(u16 arr, u16 psc)
{
	PSC = psc;
	//TIM4 输入捕获模式 使用CH1和CH2 分别存储周期/占空比 PB6和PB7
	// TIM4 输入捕获
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	// 1.使能时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);						// 使能定时器 4 时钟
	// 2.配置TIM4
	TIM_TimeBaseStructure.TIM_Period = arr;						// 设定计数器自动重装值
	TIM_TimeBaseStructure.TIM_Prescaler = psc;					// 预分频器
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		// TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // TIM 向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);				// 初始化 TIMx
	// 3.输入捕获配置
	TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;				// 选择输入端 IC1 映射到 TI1 上
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;		// 上升沿捕获
	TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; // 映射到 TI1 上
	TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;			// 配置输入分频,不分频
	TIM_ICInitStructure.TIM_ICFilter = 0x00;						// IC1F=0000 配置输入滤波器 不滤波
	TIM_PWMIConfig(TIM4, &TIM_ICInitStructure);						// 初始化 TIM4 输入捕获通道 1
	TIM_SelectInputTrigger(TIM4, TIM_TS_TI1FP1);				    //选择触发通道
	// 选择从模式: 复位模式
	// PWM 输入模式时, 从模式必须工作在复位模式，当捕获开始时, 计数器 CNT 会被复位(这样溢出中断不会发生)
	TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);
	TIM_SelectMasterSlaveMode(TIM4,TIM_MasterSlaveMode_Enable);
	// 4.中断配置 分组2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置 NVIC 中断分组 2
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;			  // TIM4 中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 先占优先级 2 级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		  // 从优先级 0 级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ 通道被使能
	NVIC_Init(&NVIC_InitStructure);							  // 初始化 NVIC
	TIM_ITConfig(TIM4,TIM_IT_CC1, ENABLE);	 				  // 中断
	TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);				  // 清除中断标志位
	TIM_Cmd(TIM4, ENABLE);									  // 使能定时器 4
}
void TIM4_IRQHandler(void){
	/* 清除中断标志位 */
	TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
	/* 获取输入捕获值 */
	IC1Value = TIM_GetCapture1(TIM4);
	IC2Value = TIM_GetCapture2(TIM4);
	//计算周期和占空比
	if((printCount++)<100){
		if(IC1Value!=0){ //!=0说明捕获了信号
			//占空比
			duty = (float)(IC2Value+1)*100/(IC1Value+1); //高电平周期比上整个周期
			freq = (float)FCLK/(PSC+1)/(IC1Value+1); //时钟频率分频之后再除以整个周期计数值 = 捕获信号频率
			printf("duty:%0.2f%%		freq:%0.2fHz\n",duty,freq);
		}else{
			duty = 0;
			freq = 0;
		}
	}else{
		TIM_ITConfig(TIM4,TIM_IT_CC1, DISABLE);
	}
}
