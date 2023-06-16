#include "FreeRTOS_Demo.h"
#include "Led.h"
#include "Button.h"
#include "Lcd.h"
#include "Buzzer.h"
#include "Tool.h"
#include "Usart.h"
#include "EEPROM.h"
//ģ���ѹ��Ũ�ȼ���
#define MINIPPM 300
//#define MAXPPM (10000-MINIPPM) //ע������10000-MINIPPM��û�н������� ֻ���ı��滻 ����Ҫ��������
#define MAXPPM 10000
#define R1 470
#define R2 1000
#define MAX_THRESHOLD_VOLTAGE 1.6
#define MIN_THRESHOLD_VOLTAGE 0.1
//�¼�λ
#define ALARM_EVENT (0x01<<0)
//�ⲿ����
extern float thresholdVoltage;
extern float adcVoltage;
extern u16 mq2Con;
//�ڲ�����
static char *sThresholdVoltage = NULL;
//������ö��
typedef enum{
	BUTTON = 0,
	LCD_FLICKER,
	LED_ALARM,
	UPDATE_LCD
}TaskName;

/**
*����:
1.������ʱ������������Ч��ԭ�����ڿռ��С,�޸�configTOTAL_HEAP_SIZE�Ĵ�С
2.vTaskDelay�������³�����������������޸�SysTick_Handler(λ��stm32f10x_it.c)�жϺ���������ϵͳʱ��߶�ͳһ
*/
//��������
static void startTask(void* pvParameters);
static void buttonTask(void* pvParameters);
static void lcdFlickerTask(void *pvParameters);
static void ledAlarmTask(void *pvParameters);
static void updateLcdTask(void* pvParameters);
static void timerCheckCallback(void *pvParameters);
static void createTask(TaskName taskName);
//����ջ��С
#define START_TASK_STACK_SIZE 128 //128��x4 = 512bytes
#define BUTTON_TASK_STACK_SIZE 128
#define LCD_FLIKCER_TASK_STACK_SIZE 128
#define LED_ALARM_TASK_STACK_SIZE 128
#define UPDATE_LCD_TASK_STACK_SIZE 128
//���ȼ� �����ֳ�����
#define START_TASK_PRIORITY 1
#define BUTTON_TASK_PRIORITY 1 //���������ȼ�һ��� ��ʵʱ��ûʲôҪ��
#define LCD_FLICKER_TASK_PRIORITY 2 //��ʾ�����ȼ��� ��ʵʱ��Ҫ�󲻸�
#define LED_ALARM_TASK_PRIORITY 3 //�������ȼ���� ִ��ʱ����������������
#define UPDATE_LCD_TASK_PRIORITY 2
//������
static TaskHandle_t startTaskHandler = NULL; //void * ����
static TaskHandle_t buttonTaskHandler = NULL;
static TaskHandle_t lcdFlickerTaskHandler = NULL;
static TaskHandle_t ledAlarmTaskHandler = NULL;
static TaskHandle_t updateLcdTaskHandler = NULL;
static SemaphoreHandle_t binSemHandler  = NULL; //�ź������
static EventGroupHandle_t alarmEventHandler = NULL; //�¼����
static TimerHandle_t timerCheckHandler = NULL; //�����ʱ�����
//�û�����ջ�ռ� ����̬������
#if(configSUPPORT_STATIC_ALLOCATION==1)
static StackType_t  startTaskStack[128];//u32���� 512bytes
static StackType_t  idleTaskStack[configMINIMAL_STACK_SIZE];
static StackType_t  timerTaskStack[configTIMER_TASK_STACK_DEPTH];
//������ƿ� ����̬������
static StaticTask_t startTaskTCB; //StaticTask_t�ǽṹ��
static StaticTask_t idleTaskTCB;
static StaticTask_t timerTaskTCB;
//��̬������Ҫ��д��������
//�������� ��û������ִ��ʱ���ô˺��� ���ȼ����
void vApplicationGetIdleTaskMemory(
	StaticTask_t **ppxIdleTaskTCBBuffer,
	StackType_t **ppxIdleTaskStackBuffer,
	uint32_t *pulIdleTaskStackSize)
{
		*ppxIdleTaskTCBBuffer = &idleTaskTCB;
		*ppxIdleTaskStackBuffer = idleTaskStack;
		*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE; //128
}
//��ʱ������ configUSE_TIMERS = 1 ���������ʱ��
void vApplicationGetTimerTaskMemory(
	StaticTask_t **ppxTimerTaskTCBBuffer,
	StackType_t **ppxTimerTaskStackBuffer,
	uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer = &timerTaskTCB;
	*ppxTimerTaskStackBuffer = timerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; //configMINIMAL_STACK_SIZE*2
}
#endif

void taskInit(void){
	taskENTER_CRITICAL(); //�����ٽ��� ֹͣ�ж�
	#if(configSUPPORT_STATIC_ALLOCATION!=0)
	/*��̬��������  �û�ָ���ռ�*/
	xTaskCreateStatic(
	(TaskFunction_t )startTask,
	(const char*    )"startTask",
	(uint32_t       )START_TASK_STACK_SIZE, //��ջ�ռ��С
	(void*          )NULL,
	(UBaseType_t    )START_TASK_PRIORITY,
	(StackType_t*   )startTaskStack, //�û������ڴ�ռ�
	(StaticTask_t*  )&startTaskTCB
	);
	#else
	/*��̬��������  ϵͳ�Զ�����ռ�*/
	xTaskCreate((TaskFunction_t )startTask,
	(const char*    )"startTask",
	(uint16_t       )START_TASK_STACK_SIZE,
	(void*          )NULL,
	(UBaseType_t    )START_TASK_PRIORITY,
	(TaskHandle_t*  )&startTaskHandler);
	#endif
	//��ʼ���������
	vTaskStartScheduler();
	taskEXIT_CRITICAL(); //�˳��ٽ���
}
void startTask(void* pvParameters){
	//�ȴ����źţ��¼�
	binSemHandler = xSemaphoreCreateBinary(); //������ֵ�ź���
//	if(NULL!= binSemHandler) printf("binSemHandler�����ɹ���\r\n");
	alarmEventHandler = xEventGroupCreate(); //�����¼�
//	if(NULL!=alarmEventHandler) printf("alarmEventHandler�����ɹ���\r\n");
	timerCheckHandler = xTimerCreate( //���������ʱ��
		(const char *)"timerCheck",
		(TickType_t)1000, //1000��tick
		(UBaseType_t) pdTRUE, //����ģʽ
		(void *) 1, //����Ψһ����ID
		(TimerCallbackFunction_t) timerCheckCallback //�ص�����
	);
	if(NULL!=timerCheckHandler) {
		xTimerStart(timerCheckHandler,0); //���ȴ�
//		printf("timerCheckHandler�����ɹ���\r\n");
	}
	//�ٴ�������
	createTask(BUTTON);
	createTask(UPDATE_LCD);
	createTask(LED_ALARM);
	//ɾ��startTask
	vTaskDelete(NULL); //NULLɾ����ǰ����
	//vTaskDelete(startTaskHandler); //ָ�����ɾ������
}
//��������
static void createTask(TaskName taskName){
	if(taskName==LCD_FLICKER){
		xTaskCreate((TaskFunction_t )lcdFlickerTask,
		(const char*    )"lcdFlickerTask",
		(uint16_t       )LCD_FLIKCER_TASK_STACK_SIZE,
		(void*          )NULL,
		(UBaseType_t    )LCD_FLICKER_TASK_PRIORITY,
		(TaskHandle_t*  )&lcdFlickerTaskHandler);
	}else if(taskName==LED_ALARM){
		xTaskCreate((TaskFunction_t )ledAlarmTask,
		(const char*    )"ledAlarmTask",
		(uint16_t       )LED_ALARM_TASK_STACK_SIZE,
		(void*          )NULL,
		(UBaseType_t    )LED_ALARM_TASK_PRIORITY,
		(TaskHandle_t*  )&ledAlarmTaskHandler);
	}else if(taskName==BUTTON){
		xTaskCreate((TaskFunction_t )buttonTask,
		(const char*    )"buttonTask",
		(uint16_t       )BUTTON_TASK_STACK_SIZE,
		(void*          )NULL,
		(UBaseType_t    )BUTTON_TASK_PRIORITY,
		(TaskHandle_t*  )&buttonTaskHandler);
	}else if(taskName==UPDATE_LCD){
		xTaskCreate((TaskFunction_t )updateLcdTask,
		(const char*    )"updateLcdTask",
		(uint16_t       )UPDATE_LCD_TASK_STACK_SIZE,
		(void*          )NULL,
		(UBaseType_t    )UPDATE_LCD_TASK_PRIORITY,
		(TaskHandle_t*  )&updateLcdTaskHandler);
	}
}
//������ֵ��˸����
static void lcdFlickerTask(void *pvParameters){
//  float tempVoltage = 0.0;
	while(1){
//		if(sVoltage==NULL){
//			sVoltage = (char *)floatNumToStr(thresholdVoltage,2); 
//			tempVoltage = thresholdVoltage;  
//		}else if(tempVoltage<thresholdVoltage||tempVoltage>thresholdVoltage){ //��ֵ�ı�ʱ �ٸı�voltage��ֵ δ�䶯ʱ����һֱ�޸� ��ʡ��Դ 
//			//tempVoltage!=thresholdVoltagefloat ����������ò�Ҫ ��Ϊ���������ȴ������ ����ֱ������ж�
//			//���ͷ���һ���洢�ַ��������ĵ�ַ
//			freeRoom();
//			tempVoltage = thresholdVoltage;
//			sVoltage = (char*) floatNumToStr(tempVoltage,2);
//		}
		sThresholdVoltage = (char *)floatNumToStr(thresholdVoltage,2);
		//��һ��Ƶ���л���ʾ  ʵ��ѡ����˸Ч��
		lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16);
		vTaskDelay(300);
		lcdShowString(76,136,sThresholdVoltage,WHITE,GREEN,16);
		vTaskDelay(300);
		if(sThresholdVoltage!=NULL){
			//�ͷ��ڴ��ַ
			vPortFree(sThresholdVoltage);
			sThresholdVoltage = NULL;
		}
	}
}
/**
*���ϴ��ڵ����⣺��ΪfloatNumToStr���ص����ַ������׵�ַ 
�����ͷ��ַ�����ַʱ��voltage������׵�ַҲ����Ч�ˣ���Ϊ�������ʼ�վ��ǹ���һ��ָ��ĵ�ַ
��������Ҳ�����freeRoom()�����������sVoltage��ʧ��ַ�����ݴ���
�������:1.�޸ķ���ָ��Ϊ�ֲ�����
2.�ı�ҵ���߼������޸���ֵ����ʱ��Ӧ����ͣ�����������񣬲��У���������ֵ����������������������˵��ֱ�۸��á�

��ʾ����:
1.��λС��ʱ,�ڶ�λС�������9�����
2.�ڶ�������ʱ����������
���ڵڶ������⣬ԭ�������ͷ��ڴ��ַʱ����Ϊ��Ҫ��ʱ��˸Ч�������°������ü�ʱ������û�м�ʱ�ͷŵ�ַ
������Ƶ����malloc��free�ᵼ���ڴ淢���仯�����ܷ����ڴ�й¶��������Ƭ����Ҫ�����ڴ����
�������:
1.ʹ��freertos�Դ����ڴ����
2.��̬��������ת�ַ�����ַ
*/

//��ť����
static void buttonTask(void* pvParameters){
	u8 keyValue = 0;
	u8 setFlag = 0; //����ȷ�ϱ�־λ
	while(1){
		keyValue = scanButton(0); //��װ��ť��⣬��߳���������
		//��ʱ���� ͬʱ�������� Ϊ�����ȼ�����ִ��
		if(keyValue!=NO_PRESS){ 
			vTaskDelay(20); //�а������� ��ʱ���� ��ֹ���ִ��
			switch(keyValue){
				//�����ж��Ƿ���ͬһ����ť��������ɿ�����
				case KEY_SET_PRESS: //����
					if(!KEY_SET) {	
						if(setFlag){
							vTaskSuspend(lcdFlickerTaskHandler); //������˸����
							if(sThresholdVoltage!=NULL){
								lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16); //�ָ�ԭ��ʽ
							}else{
								sThresholdVoltage = (char *)floatNumToStr(thresholdVoltage,2);
								lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16); //�ָ�ԭ��ʽ
							}
							I2C_EE_BufferWrite((u8*)sThresholdVoltage,0x00,4); //д��4���ֽ� ��������
							vPortFree(sThresholdVoltage);
							sThresholdVoltage = NULL;
						}else{
							if(lcdFlickerTaskHandler!=NULL){
								vTaskResume(lcdFlickerTaskHandler);
							}else{
								createTask(LCD_FLICKER); //������ʾ��˸����
							}
						}
						setFlag = !setFlag;
					}
					break; 
				case KEY_ADD_PRESS:  //��
					if(!KEY_ADD&&setFlag) {
						thresholdVoltage+=0.1;
						if(thresholdVoltage>MAX_THRESHOLD_VOLTAGE) thresholdVoltage = MAX_THRESHOLD_VOLTAGE;
					}
					break; 
				case KEY_MIN_PRESS: //��
					if(!KEY_MIN&&setFlag) {
						thresholdVoltage-=0.1;
						if(thresholdVoltage<MIN_THRESHOLD_VOLTAGE) thresholdVoltage = MIN_THRESHOLD_VOLTAGE;
					}						
					break;
				default:break;
			}
		}
	}
	/**
	*ʵ����ֻ�Ӹ�104���� ����Ч��������
	*�����ʱ���ã���Ϊʹ�õ�rtos��������ʱҲ���������Դ�˷�
	*/
}
//����������
static void ledAlarmTask(void* pvParameters){
	EventBits_t r_event; /* ����һ���¼����ձ��� */
	while(1){
		//��λ�����ƺͷ�����
		LED1 = LED2 = buzzer = 0;
		r_event = xEventGroupWaitBits(
			alarmEventHandler,
		    ALARM_EVENT,
			pdTRUE, //�˳��Զ������־λ
			pdTRUE, //�߼��� �����������¼�������
			portMAX_DELAY); //һֱ�ȴ�
		//�жϷ���ֵ�Ƿ������¼���־
		if((r_event&ALARM_EVENT)!=ALARM_EVENT){
			//printf("�¼���������������ִ��ledAlarmTask����");
		}else{
			//printf("�¼�������������ʼ����ledAlarmTask����");
			//�������Ũ�ȵ�ѹһֱ������ֵ ��ô����һֱѭ��ִ��
			while(adcVoltage>thresholdVoltage){
				if(buzzer!=1) buzzer = 1;
				LED1 = 1;LED2 = 0;
				vTaskDelay(300);
				LED1 = 0;LED2 = 1;
				vTaskDelay(300);
			}
		}
	}
}

/**
*���ϴ���һ�����⣺�����һֱ�ȴ� ��ô��ʹ�¼������� Ҳʼ�ջ�ִ���ظ��ĸ�λ����
�������:һֱ�ȴ� �ҽ���λ�ŵ���һ��ִ�� �����¼�������ʱ����ִֻ��һ�θ�λ��
*/
/**
������:��ΪcheckTask 1s�Ӳ�ִ��һ�� ���´����¼�Ҳ�����Ҫ1s�Ӳŷ��� ���Իᵼ��alarmTask�ľ���������ִ�в�����
�������:ѭ��ִ���������� ���ǺܺõĽ������ ��Ϊ�����¼����ǻ�һֱ����ȥ��������Ϊѭ����ԭ��ִ�в�������¼�(Ҳû��Ҫִ�У�ֻ�ǳ��򲻹�����)
��ʱ�������� �����˳�ѭ���� �����ǻ���Ϊ�ղ��������¼�������
*/

//ˢ����Ļ��ʾ
static void updateLcdTask(void* pvParameters){
	BaseType_t xReturn  = pdPASS;
	char *sMq2con = NULL,*sAdcVoltage = NULL;
	while(1){
		xReturn = xSemaphoreTake(binSemHandler,portMAX_DELAY); //һֱ�ȴ��ź����ͷ� Ȼ���ȡ�ͷź���ź���
//		if(pdPASS == xReturn) printf("��ȡ��ֵ�ź����ɹ���ִ��updateLcdTask����\r\n");
//		else printf("��ȡ��ֵ�ź���ʧ�ܣ���ִ��updateLcdTask����\r\n");
		//��־λ��ȡADC��ֵ ����adc�ж�Ƶ������Ӱ�������ִ�� ��1s���һ������Ӧ
		if(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)!=RESET) { //EOCת��������־λ 
			//������
			ADC_ClearFlag(ADC1,ADC_FLAG_EOC);
			//���ݴ���
			adcVoltage = (float)ADC_GetConversionValue(ADC1)/4096*3.3; //12λADC ����ģ���ѹ
			mq2Con = (float)ADC_GetConversionValue(ADC1)/4096*MAXPPM*(float)((R1+R2)/R2); //�����ѹΪŨ��
			if(adcVoltage>thresholdVoltage) xEventGroupSetBits(alarmEventHandler,ALARM_EVENT);	//������ֵ ���������¼�
			printf("[{\"con\":%d,\"voltage\":%.2f,\"threshold\":%.2f}]",mq2Con,adcVoltage,thresholdVoltage);	//�������ݵ����� json��ʽ
			//������ʾ
			lcdFillColor(56,22,80,38,BLUE); //�������λδ��ʱ�Ķ���λ��
			sMq2con = (char *)intNumToStr(mq2Con);
			lcdShowString(48,22,sMq2con,WHITE,BLUE,16); //Ũ��
			vPortFree(sMq2con); //�ͷ��ڴ�
			sMq2con = NULL;
			sAdcVoltage =(char*)floatNumToStr(adcVoltage,2);
			lcdShowString(48,46,sAdcVoltage,WHITE,BLUE,16); //��ѹ
			vPortFree(sAdcVoltage);
			sAdcVoltage = NULL;
		}
	}
}

//�ź���+�����ʱ ��һ��������ʵʱ�Ժ�ͬ���� ����������
static void timerCheckCallback(void *pvParameters){
	//1s���ͷ�һ���ź�������ˢ������ ���ȼ����(31)
	BaseType_t xReturn  = pdPASS;
	xReturn  = xSemaphoreGive(binSemHandler);
//	if(xReturn!=pdPASS) printf("�ͷŶ�ֵ�ź���ʧ�ܣ�\r\n");
//	else printf("�ͷŶ�ֵ�ź����ɹ�!\r\n");
}
/**
*����:��ʱ����Ч
�������:�������Ⱥ�˳�����⣬��Ϊ�ȴ������������ȼ��Ƚϸߣ�
���Դ������֮�󣬵����ȼ������޷��ټ���ִ��
����û�гɹ�������ʱ��
*/

//�������仯��� ��������Ӧ�ı仯
/**
*ʱ�����:
��ʹ���ź���ʱ�����赽��ʱ����(vTaskDelay)֮ǰ��ִ��ʱ����Ҫ0.2s��
��ô��������ִ��ʱ�����1+0.2 = 1.2s
���ʹ���ź��� ��ô�ź������͵�ʱ�伸�����Ժ��Բ���
������ʱ֮ǰ���������ͨ���ź���֪ͨʱ��ʼ������
����ʱ����ʱ������Ϳ��Կ�ʼִ�У�
�������������ϸ�������ʱִ�У������ͬ���ʺ�ʵʱ�ԣ����ص�ִ���������ʱ
*/
/*static void checkTask(void* pvParameters){
	u16 mq2ConTemp  = 0;
	float adcVoltageTemp = 0.0;
	while(1){
		if(adcVoltage>thresholdVoltage){
			//������Ӧ������
			if(ledAlarmTaskHandler==NULL){
				createTask(LED_ALARM);
			}else if(ledAlarmTaskHandler){
				//�������
				vTaskResume(ledAlarmTaskHandler); 
			}
		}
		if(mq2Con!=mq2ConTemp&&(adcVoltage<adcVoltageTemp||adcVoltage>adcVoltageTemp)){
			mq2ConTemp = mq2Con;
			adcVoltageTemp = adcVoltage;
			//��ʾ��Ӧ�ı仯
			lcdFillColor(56,22,80,38,BLUE); //�������λδ��ʱ�Ķ���λ��
			lcdShowString(48,22,(char *)intNumToStr(mq2ConTemp),WHITE,BLUE,16); //Ũ��
			freeRoom();
			lcdShowString(48,46,(char *)floatNumToStr(adcVoltageTemp,2),WHITE,BLUE,16); //��ѹ
			freeRoom();
			//�������ݵ����� json��ʽ
			printf("[{\"con\":%d,\"voltage\":%.2f}]",mq2ConTemp,adcVoltageTemp);
		}
		//ÿ1s�Ӽ��һ�� ����Ҫ��ô�� ̫�쵼�����ݸ���Ƶ�� ��ʾЧ���� ��Ũ���ǳ�����������
		vTaskDelay(1000);
		//��ֵ�ź��� ���ӳ�1s�� �ͽ��ź��ͷ� Ȼ���ȡ�ź� ����ˢ����ʾ
	}
}*/
