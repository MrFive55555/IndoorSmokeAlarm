#include "Lcd.h"
#include "delay.h"
#include "Usart.h"
#include "LcdFont.h"
#include "DMA.h"
#define SPIT_FLAG_TIMEOUT 65535
/**
*经过测试：
*在文件和Keil均在GB2312编码下输入中文 编译器能通过且不会乱码 因此可以将编码值用于运算
*不过仅限在GB2312下 所以以后都用这种编码格式得了!!!
*配合YaHei Consolas Hybrid 无论中英文字体效果超棒！！！(默认字体不易区分中英文标点符号，且对中文支持不好)
*/
/**
*存在问题：1.硬件SPI无效（软件模拟SPI有效）已解决
*解决方案： 1.使用双向全双工模式SPI_Direction_2Lines_FullDuplex
*			2.接收查询RXNE也要处理 否则无法正常运行
*/
/******     1.配置LCD             ****/
void lcdInit(void)
{
	// 1.配置LCD引脚
	lcdGpioInit();
	lcdSpiInit();
	// 2.复位
	LCD_RST = 0;
	delay_ms(100);
	LCD_RST = 1;
	delay_ms(100);
	LCD_BLK = 1; // 打开背光 才能显示
	delay_ms(100);
	// 3.一系列初始化
	//************* Start Initial Sequence **********//
	lcdWriteCommand(0x11); // Sleep out
	delay_ms(120);		   // Delay 120ms
	//------------------------------------ST7735S Frame Rate-----------------------------------------//
	lcdWriteCommand(0xB1);
	lcdWriteByte(0x05);
	lcdWriteByte(0x3C);
	lcdWriteByte(0x3C);
	lcdWriteCommand(0xB2);
	lcdWriteByte(0x05);
	lcdWriteByte(0x3C);
	lcdWriteByte(0x3C);
	lcdWriteCommand(0xB3);
	lcdWriteByte(0x05);
	lcdWriteByte(0x3C);
	lcdWriteByte(0x3C);
	lcdWriteByte(0x05);
	lcdWriteByte(0x3C);
	lcdWriteByte(0x3C);
	//------------------------------------End ST7735S Frame Rate---------------------------------//
	lcdWriteCommand(0xB4); // Dot inversion
	lcdWriteByte(0x03);
	//------------------------------------ST7735S Power Sequence---------------------------------//
	lcdWriteCommand(0xC0);
	lcdWriteByte(0x28);
	lcdWriteByte(0x08);
	lcdWriteByte(0x04);
	lcdWriteCommand(0xC1);
	lcdWriteByte(0XC0);
	lcdWriteCommand(0xC2);
	lcdWriteByte(0x0D);
	lcdWriteByte(0x00);
	lcdWriteCommand(0xC3);
	lcdWriteByte(0x8D);
	lcdWriteByte(0x2A);
	lcdWriteCommand(0xC4);
	lcdWriteByte(0x8D);
	lcdWriteByte(0xEE);
	//---------------------------------End ST7735S Power Sequence-------------------------------------//
	lcdWriteCommand(0xC5); // VCOM
	lcdWriteByte(0x1A);
	lcdWriteCommand(0x36); // MX, MY, RGB mode
	if (USE_HORIZONTAL == 0)
		lcdWriteByte(0x00);
	else if (USE_HORIZONTAL == 1)
		lcdWriteByte(0xC0);
	else if (USE_HORIZONTAL == 2)
		lcdWriteByte(0x70);
	else
		lcdWriteByte(0xA0);
	//------------------------------------ST7735S Gamma Sequence---------------------------------//
	lcdWriteCommand(0xE0);
	lcdWriteByte(0x04);
	lcdWriteByte(0x22);
	lcdWriteByte(0x07);
	lcdWriteByte(0x0A);
	lcdWriteByte(0x2E);
	lcdWriteByte(0x30);
	lcdWriteByte(0x25);
	lcdWriteByte(0x2A);
	lcdWriteByte(0x28);
	lcdWriteByte(0x26);
	lcdWriteByte(0x2E);
	lcdWriteByte(0x3A);
	lcdWriteByte(0x00);
	lcdWriteByte(0x01);
	lcdWriteByte(0x03);
	lcdWriteByte(0x13);
	lcdWriteCommand(0xE1);
	lcdWriteByte(0x04);
	lcdWriteByte(0x16);
	lcdWriteByte(0x06);
	lcdWriteByte(0x0D);
	lcdWriteByte(0x2D);
	lcdWriteByte(0x26);
	lcdWriteByte(0x23);
	lcdWriteByte(0x27);
	lcdWriteByte(0x27);
	lcdWriteByte(0x25);
	lcdWriteByte(0x2D);
	lcdWriteByte(0x3B);
	lcdWriteByte(0x00);
	lcdWriteByte(0x01);
	lcdWriteByte(0x04);
	lcdWriteByte(0x13);
	//------------------------------------End ST7735S Gamma Sequence-----------------------------//
	lcdWriteCommand(0x3A); // 65k mode
	lcdWriteByte(0x05);
	lcdWriteCommand(0x29); // Display on
}
// 硬件SPI通信模式配置
void lcdSpiInit(void)
{
	//SPI2 相应复用引脚 PB12_13_14_15 CS SCK MISO MOSI
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	// 使能SPI2 GPIOB 复用时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* 1.配置 SPI 的 CS 引脚，普通 IO 即可 软件控制*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// CS片选信号 保持高电平未被选择通信
	LCD_CS = 1;
	/* 配置 SPI 的 SCK 引脚 MOSI引脚 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// SCK(SCL)低电平空闲 上升沿奇数沿采样
	/* 2.SPI 模式配置 ST7735S 数据手册P44 */
	// FLASH 芯片 支持 SPI 模式 0 及模式 3,据此设置 CPOL CPHA
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex ;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; // 模式0
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // 通信速率36M/2 = 18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	/* 使能 SPI */
	SPI_Cmd(SPI2, ENABLE);
}
//软件模拟SPI通信
//void lcdSpiInit(void){
//	//上升沿采样
//	GPIO_InitTypeDef GPIO_InitStructure;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	LCD_CS = 1; //未被选择
//	LCD_SCL = 0; //低电平空闲
//	LCD_SDA = 0;
//}
// 配置LCD独有的引脚 RST DC BLK
void lcdGpioInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// 使能GPIO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	LCD_RST = 0; // 复位引脚 最好与单片机的复位引脚连接
	LCD_DC = 1;	 // 默认写入的为数据
	LCD_BLK = 0; // 关闭背光
}
void spiWriteByte(u8 data)
{
	u16 spiTimeOut;
	spiTimeOut = SPIT_FLAG_TIMEOUT;
	/* 等待发送缓冲区为空，TXE 事件 */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
	{
		// 超时打印信息
		if ((spiTimeOut--) == 0)
		{
			lcdDebugInfo(0);
			return;
		}
	}
	/* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
	SPI_I2S_SendData(SPI2, data);
	spiTimeOut = SPIT_FLAG_TIMEOUT;
	//必须进行接收查询 才能使硬件SPI正常工作 （实际并没有MISO接口 设置单向发送也无效）
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
	{
		// 超时打印信息
		if ((spiTimeOut--) == 0)
		{
			lcdDebugInfo(1);
			return;
		}
	}
	//接收一些无意义的数据 因为没有MISO接口 不建议开启 浪费资源
	//printf("receive data:%x\n",SPI_I2S_ReceiveData(SPI2));
}
//软件模拟写入数据
//void spiWriteByte(u8 data){
//	u8 i; //高位先行
//	LCD_CS = 0; //开始信号
//	for(i=0;i<8;i++){
//		LCD_SCL = 0;
//		//不能这样直接采样 数据无法写入
//		//LCD_SDA = data&(0x80>>i);
//		//正确采样数据方式
//		if(data&(0x80>>i)){
//			LCD_SDA = 1;
//		}else{                             
//			LCD_SDA = 0;
//		}
//		LCD_SCL= 1; //采样
//	}
//	LCD_CS = 1; //停止信号
//}
// 写入字节
void lcdWriteByte(u8 data){
	LCD_CS = 0; //开始信号 (片选)
	spiWriteByte(data);
	LCD_CS = 1; //停止信号
}
// 写入双字节数据
void lcdWriteDoubleByte(u16 doubleData)
{
	lcdWriteByte(doubleData >> 8);
	lcdWriteByte(doubleData); // 抛弃高8位
}
// 写入指令
void lcdWriteCommand(u8 command)
{
	LCD_DC = 0; // 低电平选择写入指令
	lcdWriteByte(command);
	LCD_DC = 1;
}
// 起始坐标
void lcdAddressSet(u16 x1, u16 y1, u16 x2, u16 y2)
{
	if (USE_HORIZONTAL == 0)
	{
		lcdWriteCommand(0x2a); // 列地址
		lcdWriteDoubleByte(x1);
		lcdWriteDoubleByte(x2);
		lcdWriteCommand(0x2b); // 行地址
		lcdWriteDoubleByte(y1);
		lcdWriteDoubleByte(y2);
		lcdWriteCommand(0x2c); // 存储器写
	}
	else if (USE_HORIZONTAL == 1)
	{
		lcdWriteCommand(0x2a); // 列地址
		lcdWriteDoubleByte(x1);
		lcdWriteDoubleByte(x2);
		lcdWriteCommand(0x2b); // 行地址
		lcdWriteDoubleByte(y1);
		lcdWriteDoubleByte(y2);
		lcdWriteCommand(0x2c); // 存储器写
	}
	else if (USE_HORIZONTAL == 2)
	{
		lcdWriteCommand(0x2a); // 列地址
		lcdWriteDoubleByte(x1 + 1);
		lcdWriteDoubleByte(x2 + 1);
		lcdWriteCommand(0x2b); // 行地址
		lcdWriteDoubleByte(y1 + 2);
		lcdWriteDoubleByte(y2 + 2);
		lcdWriteCommand(0x2c); // 存储器写
	}
	else
	{
		lcdWriteCommand(0x2a); // 列地址
		lcdWriteDoubleByte(x1 + 1);
		lcdWriteDoubleByte(x2 + 1);
		lcdWriteCommand(0x2b); // 行地址
		lcdWriteDoubleByte(y1 + 2);
		lcdWriteDoubleByte(y2 + 2);
		lcdWriteCommand(0x2c); // 存储器写
	}
}
// 打印调试信息
void lcdDebugInfo(u8 errorCode)
{
	switch (errorCode)
	{
		case 0:
			printf("SPI wirtes bytes timeout!\n");
			break;
		case 1:
			printf("SPI recieves bytes timeout!\n");
			break;
	}
}
/******     2.LCD功能             ****/
// 指定区域填充颜色
void lcdFillColor(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	//MCU写入数据
	// 设置显示范围
	u16 i, j;
	lcdAddressSet(x1, y1, x2, y2);
	// 一行一行显示
	for (i = y1; i < y2; i++)
	{
		for (j = x1; j < x2; j++)
		{
			lcdWriteDoubleByte(color);
		}
	}
	/**
	*写入数据分析：
	1.SPI8位 DMA8位 此时写入16位的数据，只会写入8低位 
	而TFT颜色代码需要16位构成 因此实际颜色代码是两个低8位组合成的（与预期颜色不一致）
	TFT等待两个低8位 因此写入数据量只够显示一半的屏幕
	解决方案：SPI设置为16位,DMA设置为16位
	*/
	//DMA写入数据
//	u16 buffSize = (x1-x2)*(y1-y2); //写入数据量
//	lcdAddressSet(x1, y1, x2, y2);
//	/**寄存器配置 SPI2数据帧格式为16位
//	*只有当SPI禁止(SPE=0)时，才能写该位，否则出错。
//	*/
//	SPI_Cmd(SPI2,DISABLE);
//	SPI2->CR1|=(u16)1<<11; 	
//	SPI_Cmd(SPI2, ENABLE);
//	//DMA搬运数据流程
//	LCD_CS = 0;
//	DMA1Init(DMA1_Channel5,(u32)&color,(u32)&SPI2->DR,16); //配置SPI2 DMA 注意：后两个参数是地址类型强转为u32类型
//	SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE); //使能SPI2_Tx DMA
//	DMANormalEnable(DMA1_Channel5,buffSize); //使能一次传输
//	while(1)
//	{
//		//可以在循环里做其他事情 不影响数据的发送
//		if(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=RESET)
//		{
//			DMA_ClearFlag(DMA1_FLAG_TC5);
//			break;
//		}
//	}
//	LCD_CS = 1;
//	/**
//	*恢复为8位数据模式
//	*/
//	SPI_Cmd(SPI2,DISABLE);
//	SPI2->CR1&=~(u16)(1<<11);
//	SPI_Cmd(SPI2, ENABLE);
	
}
// 指定区域画点
void lcdDrawPoint(u16 x, u16 y, u16 color)
{
	lcdAddressSet(x, y, x, y);
	lcdWriteDoubleByte(color);
}
// 画线
void lcdDrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;//画线起点坐标
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步
	else if (delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//水平线
	else {incy=-1;delta_y=-delta_y;}//选取基本增量坐标轴 
	if(delta_x>delta_y)distance=delta_x;
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		lcdDrawPoint(uRow,uCol,color);//画点
		xerr+=delta_x;
		yerr+=delta_y;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}
//画矩形
void lcdDrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color){
	lcdDrawLine(x1,y1,x2,y1,color);
	lcdDrawLine(x1,y2,x2,y2,color);
	lcdDrawLine(x1,y1,x1,y2,color);
	lcdDrawLine(x2,y1,x2,y1,color);
}
//画圆
void lcdDrawCircle(u16 x0,u16 y0,u8 r,u16 color){
	int a,b;
	a=0;b=r;	  
	while(a<=b)
	{
		lcdDrawPoint(x0-b,y0-a,color);             //3           
		lcdDrawPoint(x0+b,y0-a,color);             //0           
		lcdDrawPoint(x0-a,y0+b,color);             //1                
		lcdDrawPoint(x0-a,y0-b,color);             //2             
		lcdDrawPoint(x0+b,y0+a,color);             //4               
		lcdDrawPoint(x0+a,y0-b,color);             //5
		lcdDrawPoint(x0+a,y0+b,color);             //6 
		lcdDrawPoint(x0-b,y0+a,color);             //7
		a++;
		if((a*a+b*b)>(r*r))//判断要画的点是否过远
		{
			b--;
		}
	}
}
//显示字符
void lcdShowChar(u16 x,u16 y,u8 index,u16 fc,u16 bc,u8 sizey)
{
	//显示的字符就是对像素点填充,如字体颜色填充fc,就可以显示一个字符了
	u8 sizex,fontHeight,word;
	u8 i,j;
	sizex=sizey/2; //计算x坐标尾
	fontHeight=(sizex/8+(sizex%8?1:0))*sizey; //计算字体写入高度
	index=index-' ';   //计算字库数组下标
	lcdAddressSet(x,y,x+sizex-1,y+sizey-1);  //设置写入地址区域
	for(i=0;i<fontHeight;i++)
	{ 
		switch(sizey){
			case 12:word=ascii_1206[index][i];break;
			case 16:word=ascii_1608[index][i];break;
			case 24:word=ascii_2412[index][i];break;
			case 32:word=ascii_3216[index][i];break;
			default:return;
		}
		for(j=0;j<8;j++)
		{
			//判断无字体颜色填充的像素点填充背景色
			if(word&(0x01<<j)) lcdWriteDoubleByte(fc); //高电平点亮LCD阳码
			else lcdWriteDoubleByte(bc);
		}
	}   	 	  
}

//显示字符串
void lcdShowString(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 sizey)
{   
	char temp[3];
	temp[2] = '\0';
	while(*s!='\0')
	{   
		//编码大于0显示英文 反之中文
		if(*s>0){
			lcdShowChar(x,y,*s++,fc,bc,sizey);
			x+=sizey/2; //移动下一个位置
		}else{
			//中文是连续两个字节编码 所以存储两个字节数据后再显示
			for(int i = 0; i < 2;i++){
				temp[i] = *s++;
			}
			lcdShowChinese(x,y,temp,fc,bc,sizey);
			x+=sizey;
		}
	}  
}
/**
*显示思路：index指定中文字库数组下标
1.start -> length
2.因为中文是方块字 x和y轴占用一样大’

改进思路：由之前测得中文编码可获取，在此基础上将中文字体编码额外附加在字库中
在调用字库时，先比对编码是否一致，从而判断字体一致性，实现类似英文字符只需要输入英文字符即可显示的效果
*/
//显示中文
/**
*经过测试以下代码成功！！！
*算法还有改进的地方
*/
void lcdShowChinese(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 size){
	//算法时间复杂度为:n+ n*n = n(1+n)
	u8 i,j,k,word,keepIndex = 0,index = 0,pass = 0;
	for(i = 0;s[i]!='\0';){
		//编码均为负数  所以需要转成正数再运算
		if(-s[i]==chinese16[index][i%2+32]) pass++; //i%2+32编码下标
		i++;
		if(i%2==0){ //每两个编码表示一个字符
			//两个编码均匹配就进行显示
			if(pass==2){
				lcdAddressSet(x,y,x+size - 1,y+size-1);  //设置逐行式一行显示地址(size为16 两字节)
				//逐行式 低位在前 阴码(高电平有效)
				for(j=0;j<size*2;j++){
					switch(size){ //选择字形
						case 16:word=chinese16[index][j];break;
						default:return;
					}
					for(k=0;k<8;k++) //填充颜色
					{
						//判断无字体颜色填充的像素点填充背景色
						if(word&(0x01<<k)) lcdWriteDoubleByte(fc); //高电平点亮LCD阴码
						else lcdWriteDoubleByte(bc); //低电平填充背景色
					}
				}
				//x+=size; 整合到showString中 坐标在那更新
				keepIndex = i; //保存下一个比对字符位置
				index = 0; //下一个字符再重新从字库中进行比对
			}else{
				index++; //切换到下一个字符
				if(index>MAX_CHINESE_INDEX) break; //整个字库数组都遍历完毕了还没找到 就退出循环
			}
			i = keepIndex; 
			pass = 0;
		}
	}
}
//显示图片
#if USE_IMAGE
void lcdShowImage(u8 x,u8 y,u8 width,u8 height){
	const unsigned char *p = gImage_nvdi;
	int i,j,k; 
	unsigned char picH,picL;
	//显示3*3 = 9 9张图片
	for(k=0;k<1;k++)
	{
	   	for(j=0;j<1;j++)
		{	
			lcdAddressSet(x,y,x+width-1,y+height-1);		//坐标设置
		    for(i=0;i<width*height;i++)
			 {	
			 	picL=*(p+i*2);	//数据低位在前
				picH=*(p+i*2+1);				
				lcdWriteDoubleByte(picH<<8|picL);  						
			 }	
		 }
	}		
}
#endif
