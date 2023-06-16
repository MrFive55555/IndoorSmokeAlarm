#include "Wwdg.h"
#include "Led.h"
u8 WWDG_CNT=0x7f; //默认最大值
void wwdgInit(u8 tr,u8 wr,u32 fprer){
	//WWDG参数配置
	//1.使能WWDG时钟 使用APB1 36MHZ
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG, ENABLE); 
	//2.初始化 WWDG_CNT
	WWDG_CNT=tr&WWDG_CNT;
	//3.对APB1 36MHZ进行分频
	WWDG_SetPrescaler(fprer); 
	//4.设置上窗口值 范围0x40-0x7F
	WWDG_SetWindowValue(wr);
	//5.使能WWDG 并设置计数器初始值
	WWDG_Enable(WWDG_CNT); //使能看门狗,设置 counter 
	
	//WWDG中断配置
	NVIC_InitTypeDef NVIC_InitStructure;
	//所有中断是共用一个中断优先组的 所以当你配置其他组时 中断的优先级也相应的视作其他组的配置
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置 NVIC 中断分组 2
	NVIC_InitStructure.NVIC_IRQChannel = WWDG_IRQn; //WWDG 中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //抢占 2 子优先级 3 组 2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //抢占 2,子优先级 3,组 2
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	WWDG_ClearFlag(); //清除提前唤醒中断标志位
	WWDG_EnableIT(); //开启窗口看门狗中断
	
	/**
	*WWDG超时时间计算公式：
	*Twwdg=(4096×2^WDGTB×(T[6:0]-0x3f)) /Fpclk1;
	*/
}
void WWDG_IRQHandler(void)
{
	//喂狗 重置计数值
	//WWDG_Enable(WWDG_CNT); //当禁掉此句后,窗口看门狗将产生复位
	WWDG_ClearFlag(); //清除提前唤醒中断标志位
	LED1=!LED1; //LED 状态翻转
}
