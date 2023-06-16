#ifndef USART_H
#define USART_H
#include "stm32f10x.h"
#include "stdio.h"
void usartInit(const u32 bound);
void usartSendByte(USART_TypeDef *USARTx, const u8 data);
void usartSendString(USART_TypeDef *USARTx,const u8 *data);
void USART1_IRQHandler(void);
int fputc(int ch, FILE *f);
int fgetc(FILE *f);
#endif
