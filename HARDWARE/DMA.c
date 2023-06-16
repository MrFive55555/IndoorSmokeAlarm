#include "DMA.h"
/**
 * DMA减轻CPU负担 可以实现类似多线程的操作
*/
void DMA1Init(DMA_Channel_TypeDef *DMA_CHx, u32 MemoryBaseAddr, u32 PeripheralBaseAddr, u8 dataSize)
{
	DMA_InitTypeDef DMA_InitStructure; // DMA配置
	u32 perDataSize,MemDataSize;
	//设定发送数据长度
	switch(dataSize){
		case 8:
			perDataSize = DMA_PeripheralDataSize_Byte;
			MemDataSize = DMA_MemoryDataSize_Byte;
			break;
		case 16:
			perDataSize = DMA_PeripheralDataSize_HalfWord;
			MemDataSize = DMA_MemoryDataSize_HalfWord;
			break;
		case 32:
			perDataSize = DMA_PeripheralDataSize_Word;
			MemDataSize = DMA_MemoryDataSize_Word;
			break;
	}
    // DMA_DeInit(DMA_CHx); // 将 DMA 的通道 1 寄存器重设为缺省值
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE); // 开启 DMA 时钟
	DMA_InitStructure.DMA_MemoryBaseAddr = MemoryBaseAddr; // 内存地址 (要传输的变量的指针)
    DMA_InitStructure.DMA_PeripheralBaseAddr = PeripheralBaseAddr; // 设置 DMA 外设地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 方向：从内存到外设
    DMA_InitStructure.DMA_BufferSize = 0; // 传输数据量
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable; // 内存地址不自增
    DMA_InitStructure.DMA_PeripheralDataSize = perDataSize; // 外设数据单位
    DMA_InitStructure.DMA_MemoryDataSize = MemDataSize; // 内存数据单位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // DMA 模式：一次
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low; // 优先级：低
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // 禁止内存到内存的传输
    DMA_Init(DMA_CHx,&DMA_InitStructure); // 配置 DMA 通道
	//DMA_Cmd(DMA_CHx, ENABLE); //使能通道
}
//开启一次 DMA 传输
void DMANormalEnable(DMA_Channel_TypeDef*DMA_CHx,u16 buffSize)
{ 	//先关闭再使能 即开启一次DMA传输
    DMA_Cmd(DMA_CHx, DISABLE); //关闭通道 
    DMA_SetCurrDataCounter(DMA_CHx,buffSize); //设置 DMA 缓存的大小
    DMA_Cmd(DMA_CHx, ENABLE); //使能通道
}
