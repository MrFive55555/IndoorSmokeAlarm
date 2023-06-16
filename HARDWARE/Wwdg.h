#ifndef WWDG_H
#define WWDG_H
#include "sys.h"
void wwdgInit(u8 tr,u8 wr,u32 fprer);
void WWDG_IRQHandler(void);
#endif
