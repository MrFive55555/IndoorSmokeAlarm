#include "ADC.h"
void adcInit(void)
{
    // 1.配置GPIO
    adcGpioInit();
    //2.配置ADC中断
    //adcNVICInit();
    // 3.配置ADC
	ADC_InitTypeDef ADC_InitStructure;
    //  打开 ADC 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    // ADC 模式配置
    // 只使用一个 ADC，属于独立模式
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    // 禁止扫描模式，多通道才要，单通道不需要
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    // 连续转换模式
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    // 不用外部触发转换，软件开启即可
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    // 转换结果右对齐
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    // 转换通道 1 个
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    // 初始化 ADC1
    ADC_Init(ADC1, &ADC_InitStructure);
    // 配置 ADC 时钟为 PCLK2 的 8 分频，即 9MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    // 配置 ADC 通道转换顺序和采样时间
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);
    // ADC 转换结束产生中断，在中断服务程序中读取转换值
    //ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
    // 开启 ADC ，并开始转换
    ADC_Cmd(ADC1, ENABLE);
    // 初始化 ADC 校准寄存器
    ADC_ResetCalibration(ADC1);
    // 等待校准寄存器初始化完成
    while (ADC_GetResetCalibrationStatus(ADC1));
    // ADC 开始校准
    ADC_StartCalibration(ADC1);
    // 等待校准完成
    while (ADC_GetCalibrationStatus(ADC1));
    // 由于没有采用外部触发，所以使用软件触发 ADC 转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}
void adcGpioInit(void)
{
    // 配置ADC模拟输入
    GPIO_InitTypeDef GPIO_InitStructure;
    // 使能GPIOA时钟 通道ADC1_IN1 PA1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; // 模拟输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/*void adcNVICInit(void){
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}*/

//ADC1 ADC2共用一个IRQHandler
/*void ADC1_2_IRQHandler(void)
{
    if(ADC_GetITStatus(ADC1,ADC_IT_EOC)==SET) {
        //读取 ADC 的转换值
		adcVoltage = (float)ADC_GetConversionValue(ADC1)/4096*3.3; //12位ADC 计算模拟电压
		//烟雾浓度转换 产品描述检测范围是300ppm起开始检测 模拟电压上升 证明浓度是一定满足条件的
		//实测烟雾检测不超过300ppm也会有电压，可能是传感器精度问题,或者浓度已达到检测最低要求
		//更加说明这个浓度只是作为一个参考浓度
		mq2Con = (float)ADC_GetConversionValue(ADC1)/4096*MAXPPM*(float)(R1+R2)/R2;
    }
    ADC_ClearITPendingBit(ADC1,ADC_IT_EOC);
}*/

/**
*烟雾浓度计算公式:
*MQ2检测范围300-10000ppm ppm:立方厘米/立方米 百万分之一
*mq2Con = (float)ADC_GetConversionValue(ADC1)/4096*10000
*因为进行降压，所以需要再升压计算:
*mq2Con = mq2Con/(R2/(R1+R2)),其中R1 = 470,R2 = 1000
*以上只是大致的计算，由于可检测的浓度范围有限，并且浓度的变化并不是线性关系，只能作个大致的参考
*实际使用建议用更专业的浓度检测设备
*/
