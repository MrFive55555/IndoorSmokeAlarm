#include "Usart.h"
/**
 * 问题：
 * 1.发送中文乱码
*/
void usartInit(const u32 bound)
{
	// 初始化配置
	GPIO_InitTypeDef GPIO_InitStructure;   // GPIO
	USART_InitTypeDef USART_InitStructure; // 串口
	NVIC_InitTypeDef NVIC_InitStructure;   // 中断
	// 1.使能 串口 GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
	// 2.GPIO 端口模式设置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; // USART1_TX PA.9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		  // 复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure);				  // 初始化 GPIOA.9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;			  // USART1_RX PA.10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入
	GPIO_Init(GPIOA, &GPIO_InitStructure);				  // 初始化 GPIOA.10
	// 3.串口参数初始化
	USART_InitStructure.USART_BaudRate = bound;										// 波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;						// 字长为 8 位
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;								// 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式
	USART_Init(USART1, &USART_InitStructure);
	// 4.初始化 NVIC 分组2
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置 NVIC 中断分组 2
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 抢占优先级 3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  // 子优先级 3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  // IRQ 通道使能
	NVIC_Init(&NVIC_InitStructure);							  // 中断优先级初始化
	// 5.开启中断
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 开启中断
	// 6.使能串口
	USART_Cmd(USART1, ENABLE); // 使能串口
}
// 发送字符
void usartSendByte(USART_TypeDef *USARTx, const u8 data)
{
	// 发送数据
	USART_SendData(USARTx, data);
	// 等待发送数据寄存器为空
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET);
}
// 发送字符串
void usartSendString(USART_TypeDef *USARTx, const u8 *data)
{
	while(*data!='\0') usartSendByte(USARTx,*(data++));
	// 等待发送完成
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET);
}
// 中断函数 接收到数据触发中断   
void USART1_IRQHandler(void)
{
	u8 ucTemp;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		ucTemp = USART_ReceiveData(USART1);
		usartSendByte(USART1, ucTemp);
	}
}
///重定向 c 库函数 printf 到串口，重定向后可使用 printf 函数
int fputc(int ch, FILE *f)
{
	/* 发送一个字节数据到串口 */
	USART_SendData(USART1, (uint8_t) ch);
	/* 等待发送完毕 */
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
	return (ch);
}
///重定向 c 库函数 scanf 到串口，重写向后可使用 scanf、getchar 等函数
int fgetc(FILE *f)
{
	 /* 等待串口输入数据 */
	 while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
	 return (int)USART_ReceiveData(USART1);
}
