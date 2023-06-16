#include "Lcd.h"
#include "delay.h"
#include "Usart.h"
#include "LcdFont.h"
#include "DMA.h"
#define SPIT_FLAG_TIMEOUT 65535
/**
*�������ԣ�
*���ļ���Keil����GB2312�������������� ��������ͨ���Ҳ������� ��˿��Խ�����ֵ��������
*����������GB2312�� �����Ժ������ֱ����ʽ����!!!
*���YaHei Consolas Hybrid ������Ӣ������Ч������������(Ĭ�����岻��������Ӣ�ı����ţ��Ҷ�����֧�ֲ���)
*/
/**
*�������⣺1.Ӳ��SPI��Ч�����ģ��SPI��Ч���ѽ��
*��������� 1.ʹ��˫��ȫ˫��ģʽSPI_Direction_2Lines_FullDuplex
*			2.���ղ�ѯRXNEҲҪ���� �����޷���������
*/
/******     1.����LCD             ****/
void lcdInit(void)
{
	// 1.����LCD����
	lcdGpioInit();
	lcdSpiInit();
	// 2.��λ
	LCD_RST = 0;
	delay_ms(100);
	LCD_RST = 1;
	delay_ms(100);
	LCD_BLK = 1; // �򿪱��� ������ʾ
	delay_ms(100);
	// 3.һϵ�г�ʼ��
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
// Ӳ��SPIͨ��ģʽ����
void lcdSpiInit(void)
{
	//SPI2 ��Ӧ�������� PB12_13_14_15 CS SCK MISO MOSI
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	// ʹ��SPI2 GPIOB ����ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	/* 1.���� SPI �� CS ���ţ���ͨ IO ���� �������*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// CSƬѡ�ź� ���ָߵ�ƽδ��ѡ��ͨ��
	LCD_CS = 1;
	/* ���� SPI �� SCK ���� MOSI���� */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	// SCK(SCL)�͵�ƽ���� �����������ز���
	/* 2.SPI ģʽ���� ST7735S �����ֲ�P44 */
	// FLASH оƬ ֧�� SPI ģʽ 0 ��ģʽ 3,�ݴ����� CPOL CPHA
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex ;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; // ģʽ0
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; // ͨ������36M/2 = 18M
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
	/* ʹ�� SPI */
	SPI_Cmd(SPI2, ENABLE);
}
//���ģ��SPIͨ��
//void lcdSpiInit(void){
//	//�����ز���
//	GPIO_InitTypeDef GPIO_InitStructure;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//	GPIO_Init(GPIOB, &GPIO_InitStructure);
//	LCD_CS = 1; //δ��ѡ��
//	LCD_SCL = 0; //�͵�ƽ����
//	LCD_SDA = 0;
//}
// ����LCD���е����� RST DC BLK
void lcdGpioInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// ʹ��GPIO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	LCD_RST = 0; // ��λ���� ����뵥Ƭ���ĸ�λ��������
	LCD_DC = 1;	 // Ĭ��д���Ϊ����
	LCD_BLK = 0; // �رձ���
}
void spiWriteByte(u8 data)
{
	u16 spiTimeOut;
	spiTimeOut = SPIT_FLAG_TIMEOUT;
	/* �ȴ����ͻ�����Ϊ�գ�TXE �¼� */
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
	{
		// ��ʱ��ӡ��Ϣ
		if ((spiTimeOut--) == 0)
		{
			lcdDebugInfo(0);
			return;
		}
	}
	/* д�����ݼĴ�������Ҫд�������д�뷢�ͻ����� */
	SPI_I2S_SendData(SPI2, data);
	spiTimeOut = SPIT_FLAG_TIMEOUT;
	//������н��ղ�ѯ ����ʹӲ��SPI�������� ��ʵ�ʲ�û��MISO�ӿ� ���õ�����Ҳ��Ч��
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET)
	{
		// ��ʱ��ӡ��Ϣ
		if ((spiTimeOut--) == 0)
		{
			lcdDebugInfo(1);
			return;
		}
	}
	//����һЩ����������� ��Ϊû��MISO�ӿ� �����鿪�� �˷���Դ
	//printf("receive data:%x\n",SPI_I2S_ReceiveData(SPI2));
}
//���ģ��д������
//void spiWriteByte(u8 data){
//	u8 i; //��λ����
//	LCD_CS = 0; //��ʼ�ź�
//	for(i=0;i<8;i++){
//		LCD_SCL = 0;
//		//��������ֱ�Ӳ��� �����޷�д��
//		//LCD_SDA = data&(0x80>>i);
//		//��ȷ�������ݷ�ʽ
//		if(data&(0x80>>i)){
//			LCD_SDA = 1;
//		}else{                             
//			LCD_SDA = 0;
//		}
//		LCD_SCL= 1; //����
//	}
//	LCD_CS = 1; //ֹͣ�ź�
//}
// д���ֽ�
void lcdWriteByte(u8 data){
	LCD_CS = 0; //��ʼ�ź� (Ƭѡ)
	spiWriteByte(data);
	LCD_CS = 1; //ֹͣ�ź�
}
// д��˫�ֽ�����
void lcdWriteDoubleByte(u16 doubleData)
{
	lcdWriteByte(doubleData >> 8);
	lcdWriteByte(doubleData); // ������8λ
}
// д��ָ��
void lcdWriteCommand(u8 command)
{
	LCD_DC = 0; // �͵�ƽѡ��д��ָ��
	lcdWriteByte(command);
	LCD_DC = 1;
}
// ��ʼ����
void lcdAddressSet(u16 x1, u16 y1, u16 x2, u16 y2)
{
	if (USE_HORIZONTAL == 0)
	{
		lcdWriteCommand(0x2a); // �е�ַ
		lcdWriteDoubleByte(x1);
		lcdWriteDoubleByte(x2);
		lcdWriteCommand(0x2b); // �е�ַ
		lcdWriteDoubleByte(y1);
		lcdWriteDoubleByte(y2);
		lcdWriteCommand(0x2c); // �洢��д
	}
	else if (USE_HORIZONTAL == 1)
	{
		lcdWriteCommand(0x2a); // �е�ַ
		lcdWriteDoubleByte(x1);
		lcdWriteDoubleByte(x2);
		lcdWriteCommand(0x2b); // �е�ַ
		lcdWriteDoubleByte(y1);
		lcdWriteDoubleByte(y2);
		lcdWriteCommand(0x2c); // �洢��д
	}
	else if (USE_HORIZONTAL == 2)
	{
		lcdWriteCommand(0x2a); // �е�ַ
		lcdWriteDoubleByte(x1 + 1);
		lcdWriteDoubleByte(x2 + 1);
		lcdWriteCommand(0x2b); // �е�ַ
		lcdWriteDoubleByte(y1 + 2);
		lcdWriteDoubleByte(y2 + 2);
		lcdWriteCommand(0x2c); // �洢��д
	}
	else
	{
		lcdWriteCommand(0x2a); // �е�ַ
		lcdWriteDoubleByte(x1 + 1);
		lcdWriteDoubleByte(x2 + 1);
		lcdWriteCommand(0x2b); // �е�ַ
		lcdWriteDoubleByte(y1 + 2);
		lcdWriteDoubleByte(y2 + 2);
		lcdWriteCommand(0x2c); // �洢��д
	}
}
// ��ӡ������Ϣ
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
/******     2.LCD����             ****/
// ָ�����������ɫ
void lcdFillColor(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	//MCUд������
	// ������ʾ��Χ
	u16 i, j;
	lcdAddressSet(x1, y1, x2, y2);
	// һ��һ����ʾ
	for (i = y1; i < y2; i++)
	{
		for (j = x1; j < x2; j++)
		{
			lcdWriteDoubleByte(color);
		}
	}
	/**
	*д�����ݷ�����
	1.SPI8λ DMA8λ ��ʱд��16λ�����ݣ�ֻ��д��8��λ 
	��TFT��ɫ������Ҫ16λ���� ���ʵ����ɫ������������8λ��ϳɵģ���Ԥ����ɫ��һ�£�
	TFT�ȴ�������8λ ���д��������ֻ����ʾһ�����Ļ
	���������SPI����Ϊ16λ,DMA����Ϊ16λ
	*/
	//DMAд������
//	u16 buffSize = (x1-x2)*(y1-y2); //д��������
//	lcdAddressSet(x1, y1, x2, y2);
//	/**�Ĵ������� SPI2����֡��ʽΪ16λ
//	*ֻ�е�SPI��ֹ(SPE=0)ʱ������д��λ���������
//	*/
//	SPI_Cmd(SPI2,DISABLE);
//	SPI2->CR1|=(u16)1<<11; 	
//	SPI_Cmd(SPI2, ENABLE);
//	//DMA������������
//	LCD_CS = 0;
//	DMA1Init(DMA1_Channel5,(u32)&color,(u32)&SPI2->DR,16); //����SPI2 DMA ע�⣺�����������ǵ�ַ����ǿתΪu32����
//	SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE); //ʹ��SPI2_Tx DMA
//	DMANormalEnable(DMA1_Channel5,buffSize); //ʹ��һ�δ���
//	while(1)
//	{
//		//������ѭ�������������� ��Ӱ�����ݵķ���
//		if(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=RESET)
//		{
//			DMA_ClearFlag(DMA1_FLAG_TC5);
//			break;
//		}
//	}
//	LCD_CS = 1;
//	/**
//	*�ָ�Ϊ8λ����ģʽ
//	*/
//	SPI_Cmd(SPI2,DISABLE);
//	SPI2->CR1&=~(u16)(1<<11);
//	SPI_Cmd(SPI2, ENABLE);
	
}
// ָ�����򻭵�
void lcdDrawPoint(u16 x, u16 y, u16 color)
{
	lcdAddressSet(x, y, x, y);
	lcdWriteDoubleByte(color);
}
// ����
void lcdDrawLine(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //������������
	delta_y=y2-y1;
	uRow=x1;//�����������
	uCol=y1;
	if(delta_x>0)incx=1; //���õ���
	else if (delta_x==0)incx=0;//��ֱ�� 
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if (delta_y==0)incy=0;//ˮƽ��
	else {incy=-1;delta_y=-delta_y;}//ѡȡ�������������� 
	if(delta_x>delta_y)distance=delta_x;
	else distance=delta_y;
	for(t=0;t<distance+1;t++)
	{
		lcdDrawPoint(uRow,uCol,color);//����
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
//������
void lcdDrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2, u16 color){
	lcdDrawLine(x1,y1,x2,y1,color);
	lcdDrawLine(x1,y2,x2,y2,color);
	lcdDrawLine(x1,y1,x1,y2,color);
	lcdDrawLine(x2,y1,x2,y1,color);
}
//��Բ
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
		if((a*a+b*b)>(r*r))//�ж�Ҫ���ĵ��Ƿ��Զ
		{
			b--;
		}
	}
}
//��ʾ�ַ�
void lcdShowChar(u16 x,u16 y,u8 index,u16 fc,u16 bc,u8 sizey)
{
	//��ʾ���ַ����Ƕ����ص����,��������ɫ���fc,�Ϳ�����ʾһ���ַ���
	u8 sizex,fontHeight,word;
	u8 i,j;
	sizex=sizey/2; //����x����β
	fontHeight=(sizex/8+(sizex%8?1:0))*sizey; //��������д��߶�
	index=index-' ';   //�����ֿ������±�
	lcdAddressSet(x,y,x+sizex-1,y+sizey-1);  //����д���ַ����
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
			//�ж���������ɫ�������ص���䱳��ɫ
			if(word&(0x01<<j)) lcdWriteDoubleByte(fc); //�ߵ�ƽ����LCD����
			else lcdWriteDoubleByte(bc);
		}
	}   	 	  
}

//��ʾ�ַ���
void lcdShowString(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 sizey)
{   
	char temp[3];
	temp[2] = '\0';
	while(*s!='\0')
	{   
		//�������0��ʾӢ�� ��֮����
		if(*s>0){
			lcdShowChar(x,y,*s++,fc,bc,sizey);
			x+=sizey/2; //�ƶ���һ��λ��
		}else{
			//���������������ֽڱ��� ���Դ洢�����ֽ����ݺ�����ʾ
			for(int i = 0; i < 2;i++){
				temp[i] = *s++;
			}
			lcdShowChinese(x,y,temp,fc,bc,sizey);
			x+=sizey;
		}
	}  
}
/**
*��ʾ˼·��indexָ�������ֿ������±�
1.start -> length
2.��Ϊ�����Ƿ����� x��y��ռ��һ����

�Ľ�˼·����֮ǰ������ı���ɻ�ȡ���ڴ˻����Ͻ��������������⸽�����ֿ���
�ڵ����ֿ�ʱ���ȱȶԱ����Ƿ�һ�£��Ӷ��ж�����һ���ԣ�ʵ������Ӣ���ַ�ֻ��Ҫ����Ӣ���ַ�������ʾ��Ч��
*/
//��ʾ����
/**
*�����������´���ɹ�������
*�㷨���иĽ��ĵط�
*/
void lcdShowChinese(u16 x,u16 y,const char *s,u16 fc,u16 bc,u8 size){
	//�㷨ʱ�临�Ӷ�Ϊ:n+ n*n = n(1+n)
	u8 i,j,k,word,keepIndex = 0,index = 0,pass = 0;
	for(i = 0;s[i]!='\0';){
		//�����Ϊ����  ������Ҫת������������
		if(-s[i]==chinese16[index][i%2+32]) pass++; //i%2+32�����±�
		i++;
		if(i%2==0){ //ÿ���������ʾһ���ַ�
			//���������ƥ��ͽ�����ʾ
			if(pass==2){
				lcdAddressSet(x,y,x+size - 1,y+size-1);  //��������ʽһ����ʾ��ַ(sizeΪ16 ���ֽ�)
				//����ʽ ��λ��ǰ ����(�ߵ�ƽ��Ч)
				for(j=0;j<size*2;j++){
					switch(size){ //ѡ������
						case 16:word=chinese16[index][j];break;
						default:return;
					}
					for(k=0;k<8;k++) //�����ɫ
					{
						//�ж���������ɫ�������ص���䱳��ɫ
						if(word&(0x01<<k)) lcdWriteDoubleByte(fc); //�ߵ�ƽ����LCD����
						else lcdWriteDoubleByte(bc); //�͵�ƽ��䱳��ɫ
					}
				}
				//x+=size; ���ϵ�showString�� �������Ǹ���
				keepIndex = i; //������һ���ȶ��ַ�λ��
				index = 0; //��һ���ַ������´��ֿ��н��бȶ�
			}else{
				index++; //�л�����һ���ַ�
				if(index>MAX_CHINESE_INDEX) break; //�����ֿ����鶼��������˻�û�ҵ� ���˳�ѭ��
			}
			i = keepIndex; 
			pass = 0;
		}
	}
}
//��ʾͼƬ
#if USE_IMAGE
void lcdShowImage(u8 x,u8 y,u8 width,u8 height){
	const unsigned char *p = gImage_nvdi;
	int i,j,k; 
	unsigned char picH,picL;
	//��ʾ3*3 = 9 9��ͼƬ
	for(k=0;k<1;k++)
	{
	   	for(j=0;j<1;j++)
		{	
			lcdAddressSet(x,y,x+width-1,y+height-1);		//��������
		    for(i=0;i<width*height;i++)
			 {	
			 	picL=*(p+i*2);	//���ݵ�λ��ǰ
				picH=*(p+i*2+1);				
				lcdWriteDoubleByte(picH<<8|picL);  						
			 }	
		 }
	}		
}
#endif
