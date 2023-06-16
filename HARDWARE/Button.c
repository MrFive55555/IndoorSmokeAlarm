#include "Button.h"
void buttonInit(void){
    GPIO_InitTypeDef GPIO_InitStruct;
	//使能GPIOB
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	//GPIOB口的相关配置
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;//GPIO_Mode_IN_FLOATING; //浮空输入 由外部决定空闲电平状态
    GPIO_Init(GPIOB,&GPIO_InitStruct);
	/**
	*问题:浮空输入时，有时开机瞬间会导致电平不确定
	*所以输入时最好指定空闲时的上下拉电平
	*/
}
u8 scanButton(const u8 buttonMode){
	//Mode 0 不长按 反之长按
	static u8 longPress = 1;
	//问题：长按检测 解决方法：设置长按标志位
	if(longPress&&HAS_BUTTON_PRESS){
		if(buttonMode==0) longPress = !longPress;
		if(KEY_SET == 0) return KEY_SET_PRESS;
		else if(KEY_ADD == 0) return KEY_ADD_PRESS;
		else if(KEY_MIN == 0) return KEY_MIN_PRESS;
	}else if(!longPress&&NO_BUTTON_PRESS) longPress = !longPress;
	return NO_PRESS;
}
