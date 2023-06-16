#ifndef TOOL_H
#define TOOL_H
#include "sys.h"
u8* intNumToStr(u32 num);
u8 *floatNumToStr(float num,u8 decimalPlace);
float strToFloat(u8 *str,u8 numPlace);
u32 pow10(u8 place);
#endif
