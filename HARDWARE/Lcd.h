#ifndef TFT_H
#define TFT_H
#include "sys.h"
#define USE_HORIZONTAL 1  //选择横竖屏模式

#if USE_HORIZONTAL==1
#define LCD_W 128
#define LCD_H 160
#else
#define LCD_W 160
#define LCD_H 128
#endif
//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE           	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 
#define BRRED 			 0XFC07 
#define GRAY  			 0X8430 
#define DARKBLUE      	 0X01CF
#define LIGHTBLUE      	 0X7D7C	
#define GRAYBLUE       	 0X5458 
#define LIGHTGREEN     	 0X841F 
#define LGRAY 			 0XC618 
#define LGRAYBLUE        0XA651 
#define LBBLUE           0X2B12
//引脚定义 SPI通信 虽然引脚是SCL和SDA (但不是IIC通信方式)
#define LCD_CS 	PBout(12) //CS
#define LCD_SCL PBout(13) //SCL = SCK
#define LCD_SDA PBout(15) //SDA = MOSI
#define LCD_RST PBout(7) 
#define LCD_DC	PBout(8) //data command flag 决定写入的数据是命令还是变量
#define LCD_BLK PBout(9)
//1.初始化函数
void lcdInit(void);
void lcdSpiInit(void);
void lcdGpioInit(void);
void lcdWriteByte(u8 data);
void spiWriteByte(u8 data);
void lcdWriteDoubleByte(u16 doubleData);
void lcdWriteCommand(u8 command);
void lcdAddressSet(u16 x1,u16 y1,u16 x2,u16 y2);
void lcdDebugInfo(u8 errorCode);
//2.功能函数
void lcdFillColor(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void lcdDrawPoint(u16 x, u16 y, u16 color);
void lcdDrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void lcdDrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color);
void lcdDrawCircle(u16 x0,u16 y0,u8 r,u16 color);
void lcdShowChar(u16 x,u16 y,u8 index,u16 fc,u16 bc,u8 sizey);
void lcdShowString(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 sizey);
void lcdShowChinese(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 size);
void lcdShowImage(u8 x,u8 y,u8 width,u8 height);
#endif
