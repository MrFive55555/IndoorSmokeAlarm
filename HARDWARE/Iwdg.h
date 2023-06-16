#ifndef IWGD_H
#define IWGD_H
#include "sys.h"
void iwdgInit(u8 prer,u16 rlr);
void iwdgFeed(void);
#endif
