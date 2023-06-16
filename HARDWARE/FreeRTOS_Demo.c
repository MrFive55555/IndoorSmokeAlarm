#include "FreeRTOS_Demo.h"
#include "Led.h"
#include "Button.h"
#include "Lcd.h"
#include "Buzzer.h"
#include "Tool.h"
#include "Usart.h"
#include "EEPROM.h"
//模拟电压和浓度计算
#define MINIPPM 300
//#define MAXPPM (10000-MINIPPM) //注意这里10000-MINIPPM并没有进行运算 只是文本替换 故需要加上括号
#define MAXPPM 10000
#define R1 470
#define R2 1000
#define MAX_THRESHOLD_VOLTAGE 1.6
#define MIN_THRESHOLD_VOLTAGE 0.1
//事件位
#define ALARM_EVENT (0x01<<0)
//外部变量
extern float thresholdVoltage;
extern float adcVoltage;
extern u16 mq2Con;
//内部变量
static char *sThresholdVoltage = NULL;
//任务名枚举
typedef enum{
	BUTTON = 0,
	LCD_FLICKER,
	LED_ALARM,
	UPDATE_LCD
}TaskName;

/**
*问题:
1.多任务时，任务运行无效，原因在于空间过小,修改configTOTAL_HEAP_SIZE的大小
2.vTaskDelay函数导致程序卡死，解决方案是修改SysTick_Handler(位于stm32f10x_it.c)中断函数，配置系统时间尺度统一
*/
//函数声明
static void startTask(void* pvParameters);
static void buttonTask(void* pvParameters);
static void lcdFlickerTask(void *pvParameters);
static void ledAlarmTask(void *pvParameters);
static void updateLcdTask(void* pvParameters);
static void timerCheckCallback(void *pvParameters);
static void createTask(TaskName taskName);
//任务栈大小
#define START_TASK_STACK_SIZE 128 //128字x4 = 512bytes
#define BUTTON_TASK_STACK_SIZE 128
#define LCD_FLIKCER_TASK_STACK_SIZE 128
#define LED_ALARM_TASK_STACK_SIZE 128
#define UPDATE_LCD_TASK_STACK_SIZE 128
//优先级 与数字成正比
#define START_TASK_PRIORITY 1
#define BUTTON_TASK_PRIORITY 1 //按键的优先级一般低 对实时性没什么要求
#define LCD_FLICKER_TASK_PRIORITY 2 //显示的优先级低 对实时性要求不高
#define LED_ALARM_TASK_PRIORITY 3 //警报优先级最高 执行时阻塞其他所有任务
#define UPDATE_LCD_TASK_PRIORITY 2
//任务句柄
static TaskHandle_t startTaskHandler = NULL; //void * 类型
static TaskHandle_t buttonTaskHandler = NULL;
static TaskHandle_t lcdFlickerTaskHandler = NULL;
static TaskHandle_t ledAlarmTaskHandler = NULL;
static TaskHandle_t updateLcdTaskHandler = NULL;
static SemaphoreHandle_t binSemHandler  = NULL; //信号量句柄
static EventGroupHandle_t alarmEventHandler = NULL; //事件句柄
static TimerHandle_t timerCheckHandler = NULL; //软件定时器句柄
//用户分配栈空间 （静态创建）
#if(configSUPPORT_STATIC_ALLOCATION==1)
static StackType_t  startTaskStack[128];//u32类型 512bytes
static StackType_t  idleTaskStack[configMINIMAL_STACK_SIZE];
static StackType_t  timerTaskStack[configTIMER_TASK_STACK_DEPTH];
//任务控制块 （静态创建）
static StaticTask_t startTaskTCB; //StaticTask_t是结构体
static StaticTask_t idleTaskTCB;
static StaticTask_t timerTaskTCB;
//静态创建需要重写两个函数
//空闲任务 当没有任务执行时调用此函数 优先级最低
void vApplicationGetIdleTaskMemory(
	StaticTask_t **ppxIdleTaskTCBBuffer,
	StackType_t **ppxIdleTaskStackBuffer,
	uint32_t *pulIdleTaskStackSize)
{
		*ppxIdleTaskTCBBuffer = &idleTaskTCB;
		*ppxIdleTaskStackBuffer = idleTaskStack;
		*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE; //128
}
//定时器任务 configUSE_TIMERS = 1 启动软件定时器
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
	taskENTER_CRITICAL(); //进入临界区 停止中断
	#if(configSUPPORT_STATIC_ALLOCATION!=0)
	/*静态创建任务  用户指定空间*/
	xTaskCreateStatic(
	(TaskFunction_t )startTask,
	(const char*    )"startTask",
	(uint32_t       )START_TASK_STACK_SIZE, //堆栈空间大小
	(void*          )NULL,
	(UBaseType_t    )START_TASK_PRIORITY,
	(StackType_t*   )startTaskStack, //用户分配内存空间
	(StaticTask_t*  )&startTaskTCB
	);
	#else
	/*动态创建任务  系统自动分配空间*/
	xTaskCreate((TaskFunction_t )startTask,
	(const char*    )"startTask",
	(uint16_t       )START_TASK_STACK_SIZE,
	(void*          )NULL,
	(UBaseType_t    )START_TASK_PRIORITY,
	(TaskHandle_t*  )&startTaskHandler);
	#endif
	//开始任务调度器
	vTaskStartScheduler();
	taskEXIT_CRITICAL(); //退出临界区
}
void startTask(void* pvParameters){
	//先创建信号，事件
	binSemHandler = xSemaphoreCreateBinary(); //创建二值信号量
//	if(NULL!= binSemHandler) printf("binSemHandler创建成功！\r\n");
	alarmEventHandler = xEventGroupCreate(); //创建事件
//	if(NULL!=alarmEventHandler) printf("alarmEventHandler创建成功！\r\n");
	timerCheckHandler = xTimerCreate( //创建软件定时器
		(const char *)"timerCheck",
		(TickType_t)1000, //1000个tick
		(UBaseType_t) pdTRUE, //周期模式
		(void *) 1, //分配唯一索引ID
		(TimerCallbackFunction_t) timerCheckCallback //回调函数
	);
	if(NULL!=timerCheckHandler) {
		xTimerStart(timerCheckHandler,0); //不等待
//		printf("timerCheckHandler创建成功！\r\n");
	}
	//再创建任务
	createTask(BUTTON);
	createTask(UPDATE_LCD);
	createTask(LED_ALARM);
	//删除startTask
	vTaskDelete(NULL); //NULL删除当前任务
	//vTaskDelete(startTaskHandler); //指定句柄删除任务
}
//创建任务
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
//设置阈值闪烁任务
static void lcdFlickerTask(void *pvParameters){
//  float tempVoltage = 0.0;
	while(1){
//		if(sVoltage==NULL){
//			sVoltage = (char *)floatNumToStr(thresholdVoltage,2); 
//			tempVoltage = thresholdVoltage;  
//		}else if(tempVoltage<thresholdVoltage||tempVoltage>thresholdVoltage){ //当值改变时 再改变voltage的值 未变动时不用一直修改 节省资源 
//			//tempVoltage!=thresholdVoltagefloat 这种条件最好不要 因为浮点数精度存在误差 不能直接相等判断
//			//先释放上一个存储字符串变量的地址
//			freeRoom();
//			tempVoltage = thresholdVoltage;
//			sVoltage = (char*) floatNumToStr(tempVoltage,2);
//		}
		sThresholdVoltage = (char *)floatNumToStr(thresholdVoltage,2);
		//以一定频率切换显示  实现选中闪烁效果
		lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16);
		vTaskDelay(300);
		lcdShowString(76,136,sThresholdVoltage,WHITE,GREEN,16);
		vTaskDelay(300);
		if(sThresholdVoltage!=NULL){
			//释放内存地址
			vPortFree(sThresholdVoltage);
			sThresholdVoltage = NULL;
		}
	}
}
/**
*以上存在的问题：因为floatNumToStr返回的是字符串的首地址 
所以释放字符串地址时，voltage保存的首地址也就无效了，因为多个变量始终就是共用一个指针的地址
其他任务也会调用freeRoom()，导致这里的sVoltage丢失地址，数据错误
解决方案:1.修改返回指针为局部变量
2.改变业务逻辑，当修改阈值操作时，应该暂停其他所有任务，不行，看不到阈值有心理负担，对烟雾报警器来说，直观更好。

显示问题:
1.两位小数时,第二位小数会出现9的情况
2.第二次设置时，数据乱码
对于第二个问题，原因在于释放内存地址时，因为需要延时闪烁效果，导致按下设置键时，任务没有及时释放地址
分析：频繁的malloc和free会导致内存发生变化，可能发生内存泄露，产生碎片，需要进行内存管理
解决方案:
1.使用freertos自带的内存管理
2.静态分配数字转字符串地址
*/

//按钮任务
static void buttonTask(void* pvParameters){
	u8 keyValue = 0;
	u8 setFlag = 0; //设置确认标志位
	while(1){
		keyValue = scanButton(0); //封装按钮检测，提高程序利用率
		//延时消抖 同时阻塞任务 为低优先级任务执行
		if(keyValue!=NO_PRESS){ 
			vTaskDelay(20); //有按键按下 延时消抖 防止多次执行
			switch(keyValue){
				//二次判断是否按下同一个按钮，并解决松开消抖
				case KEY_SET_PRESS: //设置
					if(!KEY_SET) {	
						if(setFlag){
							vTaskSuspend(lcdFlickerTaskHandler); //挂起闪烁任务
							if(sThresholdVoltage!=NULL){
								lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16); //恢复原样式
							}else{
								sThresholdVoltage = (char *)floatNumToStr(thresholdVoltage,2);
								lcdShowString(76,136,sThresholdVoltage,WHITE,BLUE,16); //恢复原样式
							}
							I2C_EE_BufferWrite((u8*)sThresholdVoltage,0x00,4); //写入4个字节 保存数据
							vPortFree(sThresholdVoltage);
							sThresholdVoltage = NULL;
						}else{
							if(lcdFlickerTaskHandler!=NULL){
								vTaskResume(lcdFlickerTaskHandler);
							}else{
								createTask(LCD_FLICKER); //创建显示闪烁任务
							}
						}
						setFlag = !setFlag;
					}
					break; 
				case KEY_ADD_PRESS:  //加
					if(!KEY_ADD&&setFlag) {
						thresholdVoltage+=0.1;
						if(thresholdVoltage>MAX_THRESHOLD_VOLTAGE) thresholdVoltage = MAX_THRESHOLD_VOLTAGE;
					}
					break; 
				case KEY_MIN_PRESS: //减
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
	*实测测得只加个104电容 消抖效果并不好
	*软件延时更好，因为使用的rtos，所以延时也不会造成资源浪费
	*/
}
//警报灯任务
static void ledAlarmTask(void* pvParameters){
	EventBits_t r_event; /* 定义一个事件接收变量 */
	while(1){
		//复位警报灯和蜂鸣器
		LED1 = LED2 = buzzer = 0;
		r_event = xEventGroupWaitBits(
			alarmEventHandler,
		    ALARM_EVENT,
			pdTRUE, //退出自动清除标志位
			pdTRUE, //逻辑与 （满足所有事件条件）
			portMAX_DELAY); //一直等待
		//判断返回值是否满足事件标志
		if((r_event&ALARM_EVENT)!=ALARM_EVENT){
			//printf("事件不满足条件，不执行ledAlarmTask任务");
		}else{
			//printf("事件满足条件，开始就绪ledAlarmTask任务");
			//如果烟雾浓度电压一直大于阈值 那么任务一直循环执行
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
*以上存在一个问题：如果不一直等待 那么即使事件不发生 也始终会执行重复的复位代码
解决方案:一直等待 且将复位放到第一句执行 这样事件不发生时，就只执行一次复位了
*/
/**
新问题:因为checkTask 1s钟才执行一次 导致触发事件也大概需要1s钟才发出 所以会导致alarmTask的警报灯任务执行不连贯
解决方案:循环执行任务主体 不是很好的解决方案 因为触发事件还是会一直发出去，但是因为循环的原因，执行不到这个事件(也没必要执行，只是程序不够优雅)
暂时就这样吧 至少退出循环后 任务还是会因为收不到触发事件而阻塞
*/

//刷新屏幕显示
static void updateLcdTask(void* pvParameters){
	BaseType_t xReturn  = pdPASS;
	char *sMq2con = NULL,*sAdcVoltage = NULL;
	while(1){
		xReturn = xSemaphoreTake(binSemHandler,portMAX_DELAY); //一直等待信号量释放 然后获取释放后的信号量
//		if(pdPASS == xReturn) printf("获取二值信号量成功，执行updateLcdTask任务\r\n");
//		else printf("获取二值信号量失败，不执行updateLcdTask任务\r\n");
		//标志位获取ADC的值 避免adc中断频繁触发影响任务的执行 且1s检测一次相适应
		if(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)!=RESET) { //EOC转换结束标志位 
			//清除标记
			ADC_ClearFlag(ADC1,ADC_FLAG_EOC);
			//数据处理
			adcVoltage = (float)ADC_GetConversionValue(ADC1)/4096*3.3; //12位ADC 计算模拟电压
			mq2Con = (float)ADC_GetConversionValue(ADC1)/4096*MAXPPM*(float)((R1+R2)/R2); //换算电压为浓度
			if(adcVoltage>thresholdVoltage) xEventGroupSetBits(alarmEventHandler,ALARM_EVENT);	//超过阈值 触发警报事件
			printf("[{\"con\":%d,\"voltage\":%.2f,\"threshold\":%.2f}]",mq2Con,adcVoltage,thresholdVoltage);	//发送数据到串口 json格式
			//数据显示
			lcdFillColor(56,22,80,38,BLUE); //清空数字位未满时的多余位置
			sMq2con = (char *)intNumToStr(mq2Con);
			lcdShowString(48,22,sMq2con,WHITE,BLUE,16); //浓度
			vPortFree(sMq2con); //释放内存
			sMq2con = NULL;
			sAdcVoltage =(char*)floatNumToStr(adcVoltage,2);
			lcdShowString(48,46,sAdcVoltage,WHITE,BLUE,16); //电压
			vPortFree(sAdcVoltage);
			sAdcVoltage = NULL;
		}
	}
}

//信号量+软件延时 进一步提升了实时性和同步性 分析见初版
static void timerCheckCallback(void *pvParameters){
	//1s钟释放一次信号量用于刷新数据 优先级最大(31)
	BaseType_t xReturn  = pdPASS;
	xReturn  = xSemaphoreGive(binSemHandler);
//	if(xReturn!=pdPASS) printf("释放二值信号量失败！\r\n");
//	else printf("释放二值信号量成功!\r\n");
}
/**
*问题:定时器无效
解决方案:创建的先后顺序问题，因为先创建的任务优先级比较高，
所以创建完毕之后，低优先级任务无法再继续执行
导致没有成功启动定时器
*/

//检测变量变化情况 并做出相应的变化
/**
*时间分析:
不使用信号量时，假设到延时函数(vTaskDelay)之前的执行时间需要0.2s，
那么整个任务执行时间就是1+0.2 = 1.2s
如果使用信号量 那么信号量的释的时间几乎可以忽略不计
所以延时之前的任务可以通过信号量通知时开始就绪，
当延时调用时，任务就可以开始执行，
这样任务是在上个任务延时执行，提高了同步率和实时性，不必等执行完毕再延时
*/
/*static void checkTask(void* pvParameters){
	u16 mq2ConTemp  = 0;
	float adcVoltageTemp = 0.0;
	while(1){
		if(adcVoltage>thresholdVoltage){
			//创建相应的任务
			if(ledAlarmTaskHandler==NULL){
				createTask(LED_ALARM);
			}else if(ledAlarmTaskHandler){
				//任务继续
				vTaskResume(ledAlarmTaskHandler); 
			}
		}
		if(mq2Con!=mq2ConTemp&&(adcVoltage<adcVoltageTemp||adcVoltage>adcVoltageTemp)){
			mq2ConTemp = mq2Con;
			adcVoltageTemp = adcVoltage;
			//显示相应的变化
			lcdFillColor(56,22,80,38,BLUE); //清空数字位未满时的多余位置
			lcdShowString(48,22,(char *)intNumToStr(mq2ConTemp),WHITE,BLUE,16); //浓度
			freeRoom();
			lcdShowString(48,46,(char *)floatNumToStr(adcVoltageTemp,2),WHITE,BLUE,16); //电压
			freeRoom();
			//发送数据到串口 json格式
			printf("[{\"con\":%d,\"voltage\":%.2f}]",mq2ConTemp,adcVoltageTemp);
		}
		//每1s钟检测一次 不需要那么快 太快导致数据更新频繁 显示效果差 且浓度是持续性上升的
		vTaskDelay(1000);
		//二值信号量 到延迟1s后 就将信号释放 然后获取信号 用于刷新显示
	}
}*/
