#ifndef DMA_H
#define DMA_H
#include "sys.h"
void DMA1Init(DMA_Channel_TypeDef *DMA_CHx, u32 MemoryBaseAddr, u32 PeripheralBaseAddr,u8 dataSize);
void DMANormalEnable(DMA_Channel_TypeDef*DMA_CHx,u16 buffSize);
#endif
