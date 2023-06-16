#include "FreeRTOS_Demo.h"
#include "Led.h"
#include "Usart.h"
#include "lcd.h"
#include "delay.h"
#include "Buzzer.h"
#include "Button.h"
#include "ADC.h"
#include "Tool.h"
#include "EEPROM.h"
float adcVoltage = 0.00; //引用外部main.c中定义的变量 避免重复定义 节省内存开销
u16 mq2Con = 0;
float thresholdVoltage; //阈值电压
void UIInit(){
	u8 sThresholdVoltage[5];
	//背景色
	lcdFillColor(0,0,LCD_W,LCD_H,BLUE);
	//文字
	lcdShowString(8,2,"室内烟雾报警器",WHITE,BLUE,16);  //标题
	lcdShowString(8,22,"浓度:",WHITE,BLUE,16);
	lcdShowString(48,22,"0",WHITE,BLUE,16);
	lcdShowString(8,46,"电压:",WHITE,BLUE,16);
	lcdShowString(48,46,"0.00",WHITE,BLUE,16);
	//数据单位
	lcdShowString(86,22,"ppm",WHITE,BLUE,16);
	lcdShowChar(86,46,'V',WHITE,BLUE,16);
	//阈值显示
	lcdShowString(4,136,"当前阈值:",WHITE,BLUE,16); //刷新显示
	//恢复阈值数据过程：先从eeprom通过i2c协议读取数据，然后将字符串转换成浮点型，再将转换后的值赋值给阈值即可
	I2C_EE_BufferRead(sThresholdVoltage,0x00,4);
	sThresholdVoltage[4] = '\0';
	//printf("sThresholdVoltage is %s\r\n",sThresholdVoltage);
	thresholdVoltage = strToFloat(sThresholdVoltage,2);
	//printf("thresholdVoltage is %f\r\n",thresholdVoltage);
	//printf("recover datas successful!\r\n");
	lcdShowString(76,136,(char*)sThresholdVoltage,WHITE,BLUE,16);
	lcdShowChar(120,136,'V',WHITE,BLUE,16);
	//char *ch = "爱";
	//lcdShowString(20,120,intNumToStr(-ch[0]),WHITE,BLUE,16); //因为中文编码均为负数 所以转字符串需要转正
#if USE_IMAGE
	//图片
	lcdShowImage(13,39,100,100);
#endif
}
void allInit(void){
	delay_init();
	usartInit(115200);
	adcInit();
	buzzerInit();
	buttonInit();
	ledInit(); 
	lcdInit(); 
	I2C_EE_Init();
	UIInit();
	taskInit();
}
/**
*usartInit需要在eepromInit之前初始化，否则会程序卡死
因为eepromInit的函数使用到了串口通信，故需要先初始化串口通信
最终发现显示屏的RST引脚和I2C的SDA引脚冲突！！！！！唉，太不细心了。。。。
解决方案：先点亮屏幕，然后再配置I2C，虽然能用，可终究是不符合规范的。。。。
*/
int main(void){
	allInit();
	//开启任务后 不会执行以下循环
	while(1){
		//buzzer =1 ;
	}
}
