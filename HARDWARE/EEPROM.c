#include "EEPROM.h"
#include "Usart.h"		
//代码参考：野火
uint16_t EEPROM_ADDRESS;
static __IO uint32_t  I2CTimeout = I2CT_LONG_TIMEOUT;   
static uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode);
//初始化数据线和时钟线
static void I2C_GPIO_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 
  EEPROM_I2C_APBxClock_FUN ( EEPROM_I2C_CLK, ENABLE );
  EEPROM_I2C_GPIO_APBxClock_FUN ( EEPROM_I2C_GPIO_CLK, ENABLE );
  GPIO_InitStructure.GPIO_Pin = EEPROM_I2C_SCL_PIN|EEPROM_I2C_SDA_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;	       // 开漏输出，高电平由外部决定
  GPIO_Init(EEPROM_I2C_SDA_PORT, &GPIO_InitStructure);	
}
//初始化I2C
static void I2C_Mode_Configu(void)
{
  I2C_InitTypeDef  I2C_InitStructure; 
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 =I2Cx_OWN_ADDRESS7; 
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable ;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_Speed;
  I2C_Init(EEPROM_I2Cx, &I2C_InitStructure);
  I2C_Cmd(EEPROM_I2Cx, ENABLE);   
}

//初始化eeprom
void I2C_EE_Init(void)
{

  I2C_GPIO_Config(); 
  I2C_Mode_Configu();
//设备地址选择
#ifdef EEPROM_Block0_ADDRESS
  EEPROM_ADDRESS = EEPROM_Block0_ADDRESS;
#endif

#ifdef EEPROM_Block1_ADDRESS  
  EEPROM_ADDRESS = EEPROM_Block1_ADDRESS;
#endif

#ifdef EEPROM_Block2_ADDRESS  
  EEPROM_ADDRESS = EEPROM_Block2_ADDRESS;
#endif

#ifdef EEPROM_Block3_ADDRESS  
  EEPROM_ADDRESS = EEPROM_Block3_ADDRESS;
#endif
}

//写入多字节
void I2C_EE_BufferWrite(u8* pBuffer, u8 WriteAddr, u16 NumByteToWrite)
{
  u8 NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0;

  Addr = WriteAddr % I2C_PageSize;
  count = I2C_PageSize - Addr;
  NumOfPage =  NumByteToWrite / I2C_PageSize;
  NumOfSingle = NumByteToWrite % I2C_PageSize;
 
  /* If WriteAddr is I2C_PageSize aligned  */
  if(Addr == 0) 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage == 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_EE_WaitEepromStandbyState();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else  
    {
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize); 
    	I2C_EE_WaitEepromStandbyState();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;
      }

      if(NumOfSingle!=0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }
  /* If WriteAddr is not I2C_PageSize aligned  */
  else 
  {
    /* If NumByteToWrite < I2C_PageSize */
    if(NumOfPage== 0) 
    {
      I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      I2C_EE_WaitEepromStandbyState();
    }
    /* If NumByteToWrite > I2C_PageSize */
    else
    {
      NumByteToWrite -= count;
      NumOfPage =  NumByteToWrite / I2C_PageSize;
      NumOfSingle = NumByteToWrite % I2C_PageSize;	
      if(count != 0)
      {  
        I2C_EE_PageWrite(pBuffer, WriteAddr, count);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr += count;
        pBuffer += count;
      } 
      while(NumOfPage--)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, I2C_PageSize);
        I2C_EE_WaitEepromStandbyState();
        WriteAddr +=  I2C_PageSize;
        pBuffer += I2C_PageSize;  
      }
      if(NumOfSingle != 0)
      {
        I2C_EE_PageWrite(pBuffer, WriteAddr, NumOfSingle); 
        I2C_EE_WaitEepromStandbyState();
      }
    }
  }  
}


//写入单字节
uint32_t I2C_EE_ByteWrite(u8* pBuffer, u8 WriteAddr) 
{
  /* Send STRAT condition */
  I2C_GenerateSTART(EEPROM_I2Cx, ENABLE);

  I2CTimeout = I2CT_FLAG_TIMEOUT;  
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(0);
  } 
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_Direction_Transmitter);
  
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(1);
  }  
  /* Send the EEPROM's internal address to write to */
  I2C_SendData(EEPROM_I2Cx, WriteAddr);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(2);
  } 
  
  /* Send the byte to be written */
  I2C_SendData(EEPROM_I2Cx, *pBuffer); 
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;  
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
  } 
  
  /* Send STOP condition */
  I2C_GenerateSTOP(EEPROM_I2Cx, ENABLE);
  
  return 1;
}


//按页写入
uint32_t I2C_EE_PageWrite(u8* pBuffer, u8 WriteAddr, u8 NumByteToWrite)
{
  I2CTimeout = I2CT_LONG_TIMEOUT;

  while(I2C_GetFlagStatus(EEPROM_I2Cx, I2C_FLAG_BUSY))   
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(4);
  } 
  
  /* Send START condition */
  I2C_GenerateSTART(EEPROM_I2Cx, ENABLE);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_MODE_SELECT))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(5);
  } 
  
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_Direction_Transmitter);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))  
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(6);
  } 
  
  /* Send the EEPROM's internal address to write to */    
  I2C_SendData(EEPROM_I2Cx, WriteAddr);  

  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(! I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(7);
  } 

  /* While there is data to be written */
  while(NumByteToWrite--)  
  {
    /* Send the current byte */
    I2C_SendData(EEPROM_I2Cx, *pBuffer); 

    /* Point to the next byte to be written */
    pBuffer++; 
  
    I2CTimeout = I2CT_FLAG_TIMEOUT;

    /* Test on EV8 and clear it */
    while (!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
      if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(8);
    } 
  }

  /* Send STOP condition */
  I2C_GenerateSTOP(EEPROM_I2Cx, ENABLE);
  
  return 1;
}

//读取多字节
uint32_t I2C_EE_BufferRead(u8* pBuffer, u8 ReadAddr, u16 NumByteToRead)
{  
  
  I2CTimeout = I2CT_LONG_TIMEOUT;
  
  //*((u8 *)0x4001080c) |=0x80; 
  while(I2C_GetFlagStatus(EEPROM_I2Cx, I2C_FLAG_BUSY))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(9);
   }
  
  /* Send START condition */
  I2C_GenerateSTART(EEPROM_I2Cx, ENABLE);
  //*((u8 *)0x4001080c) &=~0x80;
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(10);
   }
  
  /* Send EEPROM address for write */
  I2C_Send7bitAddress(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_Direction_Transmitter);

  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(11);
   }
    
  /* Clear EV6 by setting again the PE bit */
  I2C_Cmd(EEPROM_I2Cx, ENABLE);

  /* Send the EEPROM's internal address to write to */
  I2C_SendData(EEPROM_I2Cx, ReadAddr);  

   
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV8 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(12);
   }
    
  /* Send STRAT condition a second time */  
  I2C_GenerateSTART(EEPROM_I2Cx, ENABLE);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV5 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_MODE_SELECT))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(13);
   }
    
  /* Send EEPROM address for read */
  I2C_Send7bitAddress(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_Direction_Receiver);
  
  I2CTimeout = I2CT_FLAG_TIMEOUT;
  /* Test on EV6 and clear it */
  while(!I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
  {
    if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(14);
   }
  
  /* While there is data to be read */
  while(NumByteToRead)  
  {
    if(NumByteToRead == 1)
    {
      /* Disable Acknowledgement */
      I2C_AcknowledgeConfig(EEPROM_I2Cx, DISABLE);
      
      /* Send STOP Condition */
      I2C_GenerateSTOP(EEPROM_I2Cx, ENABLE);
    }

    /* Test on EV7 and clear it */    
    I2CTimeout = I2CT_LONG_TIMEOUT;
    
		while(I2C_CheckEvent(EEPROM_I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED)==0)  
		{
			if((I2CTimeout--) == 0) return I2C_TIMEOUT_UserCallback(3);
		} 
    {      
      /* Read a byte from the EEPROM */
      *pBuffer = I2C_ReceiveData(EEPROM_I2Cx);

      /* Point to the next location where the byte read will be saved */
      pBuffer++; 
      
      /* Decrement the read bytes counter */
      NumByteToRead--;        
    }   
  }

  /* Enable Acknowledgement to be ready for another reception */
  I2C_AcknowledgeConfig(EEPROM_I2Cx, ENABLE);
  
    return 1;
}

//等待eeprom空闲
void I2C_EE_WaitEepromStandbyState(void)      
{
  vu16 SR1_Tmp = 0;
  do
  {
    /* Send START condition */
    I2C_GenerateSTART(EEPROM_I2Cx, ENABLE);
    /* Read I2C1 SR1 register */
    SR1_Tmp = I2C_ReadRegister(EEPROM_I2Cx, I2C_Register_SR1);
    /* Send EEPROM address for write */
    I2C_Send7bitAddress(EEPROM_I2Cx, EEPROM_ADDRESS, I2C_Direction_Transmitter);
  }while(!(I2C_ReadRegister(EEPROM_I2Cx, I2C_Register_SR1) & 0x0002));
  /* Clear AF flag */
  I2C_ClearFlag(EEPROM_I2Cx, I2C_FLAG_AF);
  /* STOP condition */    
  I2C_GenerateSTOP(EEPROM_I2Cx, ENABLE); 
}

//超时反馈
static uint32_t I2C_TIMEOUT_UserCallback(uint8_t errorCode)
{
  /* Block communication and all processes */
  EEPROM_ERROR("I2C had waited more time!errorCode = %d",errorCode);
  return 0;
}

