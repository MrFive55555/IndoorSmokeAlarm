#include "Iwdg.h"
/**
*IWDG独立看门狗 使用内部RC时钟40Khz
*程序编写简单快捷，缺点不够精确，需要注意喂狗时机 
*/
void iwdgInit(u8 prer,u16 rlr){
	//1.取消寄存器写保护
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); 
	//2.设置IWDG预分频值 范围0-7
	IWDG_SetPrescaler(prer);
	//3.设置IWDG重装载值 范围0x000-0xfff
	IWDG_SetReload(rlr);
	//4.重载计数值喂狗 重载值就是rlr
	IWDG_ReloadCounter(); 
	//5.使能IWDG 使能后不能关闭 除非重启并不再打开
	IWDG_Enable(); 
	//溢出时间计算：Tout=((4×2^prer) ×rlr) /40
	//1s的配置方式 prer=4 rlr=625
	//3s prer=4 rlr=1875
}

void iwdgFeed(void){
	/**
	*喂狗就是重载计数值
	*如果不及时喂狗 狗子就会叫 单片机就会复位
	*/
	IWDG_ReloadCounter(); 
}
