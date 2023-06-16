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
float adcVoltage = 0.00; //�����ⲿmain.c�ж���ı��� �����ظ����� ��ʡ�ڴ濪��
u16 mq2Con = 0;
float thresholdVoltage; //��ֵ��ѹ
void UIInit(){
	u8 sThresholdVoltage[5];
	//����ɫ
	lcdFillColor(0,0,LCD_W,LCD_H,BLUE);
	//����
	lcdShowString(8,2,"������������",WHITE,BLUE,16);  //����
	lcdShowString(8,22,"Ũ��:",WHITE,BLUE,16);
	lcdShowString(48,22,"0",WHITE,BLUE,16);
	lcdShowString(8,46,"��ѹ:",WHITE,BLUE,16);
	lcdShowString(48,46,"0.00",WHITE,BLUE,16);
	//���ݵ�λ
	lcdShowString(86,22,"ppm",WHITE,BLUE,16);
	lcdShowChar(86,46,'V',WHITE,BLUE,16);
	//��ֵ��ʾ
	lcdShowString(4,136,"��ǰ��ֵ:",WHITE,BLUE,16); //ˢ����ʾ
	//�ָ���ֵ���ݹ��̣��ȴ�eepromͨ��i2cЭ���ȡ���ݣ�Ȼ���ַ���ת���ɸ����ͣ��ٽ�ת�����ֵ��ֵ����ֵ����
	I2C_EE_BufferRead(sThresholdVoltage,0x00,4);
	sThresholdVoltage[4] = '\0';
	//printf("sThresholdVoltage is %s\r\n",sThresholdVoltage);
	thresholdVoltage = strToFloat(sThresholdVoltage,2);
	//printf("thresholdVoltage is %f\r\n",thresholdVoltage);
	//printf("recover datas successful!\r\n");
	lcdShowString(76,136,(char*)sThresholdVoltage,WHITE,BLUE,16);
	lcdShowChar(120,136,'V',WHITE,BLUE,16);
	//char *ch = "��";
	//lcdShowString(20,120,intNumToStr(-ch[0]),WHITE,BLUE,16); //��Ϊ���ı����Ϊ���� ����ת�ַ�����Ҫת��
#if USE_IMAGE
	//ͼƬ
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
*usartInit��Ҫ��eepromInit֮ǰ��ʼ��������������
��ΪeepromInit�ĺ���ʹ�õ��˴���ͨ�ţ�����Ҫ�ȳ�ʼ������ͨ��
���շ�����ʾ����RST���ź�I2C��SDA���ų�ͻ��������������̫��ϸ���ˡ�������
����������ȵ�����Ļ��Ȼ��������I2C����Ȼ���ã����վ��ǲ����Ϲ淶�ġ�������
*/
int main(void){
	allInit();
	//��������� ����ִ������ѭ��
	while(1){
		//buzzer =1 ;
	}
}
