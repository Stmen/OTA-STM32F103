#include "BSP_OTA.h"
				
#define BSP_Write_Addr 0x08003C00	

#define SendNum_len			10
u16 sendNum[SendNum_len];

u32 iapbuf[512];  
iapfun jump2app; 
u16 AppLenth,oldCount,USART_RX_CNT = 0;
u16 OTAFLAG;

/**
  * @brief  ���桢ִ�й̼�
  * @retval 0  �������ݷǹ̼�
  */
u8 Update_Firmware(void)
{
		if(((*(vu32*)(BSP_Write_Addr+4))&0xFF000000)==0x08000000)//�ж��Ƿ�Ϊ0X08XXXXXX.																			
		{	 
			printf("��ʼִ��FLASH�û�����!!\r\n");
			iap_load_app(BSP_Write_Addr);			//ִ��FLASH APP����	
		}else 
		{
			printf("��FLASHӦ�ó���,�޷�ִ��!\r\n");
		}	
		 
		return 0;
}

/**
  * @brief  Count_SendNum
	* @note   ������մ���,���ݹ̼�SIZE���
  * @param  size:�����������
  * @retval None	  
  */
void Count_ReceiveNum(void)
{
		u8 i = 0;
		
		while (AppLenth)
		{
			if (AppLenth >= Single_SendSize)	
			{
				sendNum[i] = Single_SendSize/Single_DataSize;	// ����NRF���ʹ���
				AppLenth -= Single_SendSize;
			}				
			else 
			{	
				if (AppLenth%Single_DataSize != 0) sendNum[i] = AppLenth/Single_DataSize + 1;		// ����NRF���ʹ���
				else  sendNum[i] = AppLenth/Single_DataSize;
				AppLenth = 0;
			}
			printf("��%d�ν��մ���Ϊ%d\r\n",i+1,sendNum[i]);
			i++;		
		}
}

/**
  * @brief  Receive_Firmware
	* @note:	���չ̼�,��д�뵽flash
  * @param  
  * @retval 1 ���³ɹ� or 0 ����ʧ��  
  */
u8 Receive_Firmware(void)
{
		u16 i = 0;
		u16 x = 0;
		USART_RX_CNT = 0;
		u32 Addr =	0x08003C00; 
		
	
		while (sendNum[i])
		{
			while(USART_RX_CNT <= (sendNum[i]-1))
			{	
					x = 0;
					//HAL_Delay(1);
					while( NRF24L01_Rx(USART_RX_CNT) && x < 1000 )
					{
						HAL_Delay(1); //printf("1");
						if(++x >= 950) 
						{
							printf("1s�ڼ�ⲻ�����ݣ�ȡ������\r\n");
							Change_OTAFlag(0xffff);
							return 0;
						}
					}
				//	printf(" %3d %3d %3d\r\n",SPI_RX_BUF[0] + SPI_RX_BUF[1]*256,USART_RX_CNT + i*256,SPI_RX_BUF[4]); 		// ����ʹ��
					if (SPI_RX_BUF[0] + SPI_RX_BUF[1]*256 == i*256 + USART_RX_CNT)	USART_RX_CNT++; 					
			}
			USART_RX_CNT = 0;		// ��ʼ��SRAM��������׼����һ�ֽ���	

			if(!i)
			{
				if(((*(vu32*)(0X20001000+4))&0xFF000000)==0x08000000)  //�ж��Ƿ�Ϊ0X08XXXXXX. 
				{	 
					iap_write_appbin(Addr,OTA_RX,sendNum[i]*Single_DataSize);		//����FLASH����   
					printf("\tд��flash�ɹ���\r\n");	
				}
				else 
				{
					printf("\t�ǹ̼�������д��flash��\r\n");	
				}
			}		
			else  			
			{
				iap_write_appbin(Addr,OTA_RX,sendNum[i]*Single_DataSize);		//����FLASH����   
				printf("\thello,д��flash�ɹ���\r\n");				
			}
			printf("\t%d %x\r\n",i,Addr);
			Addr += sendNum[i]*Single_DataSize;		// ��ַƫ��   
			
			i++;
		}
		
		Change_OTAFlag(0xd0d0);
		Update_Firmware();		// ���¹̼�
		return 1;
}

/**
  * @brief  OTA_RxMode
  * @param  void
  * @retval None	  
  */
void OTA_RxMode(void)
{
		NRF24L01_Init();
		NRF24L01_RX_Mode();
}

/**
  * @brief  APPд��FLASH
  * @param  appxaddr:Ӧ�ó������ʼ��ַ
  * @param 	appbuf:  Ӧ�ó���CODE.
  * @param	appsize: Ӧ�ó����С(�ֽ�).
  * @retval None	  
  */
void iap_write_appbin(u32 appxaddr,u8 *appbuf,u32 appsize)
{
	u16 t;
	u16 i=0;
	u32 temp;
	u32 fwaddr=appxaddr;//��ǰд��ĵ�ַ
	u8 *dfu=appbuf;

	for(t=0;t<appsize;t+=4)
	{			
		temp=(u32)dfu[3]<<24;
		temp+=(u32)dfu[2]<<16;
		temp+=(u32)dfu[1]<<8;
		temp+=(u32)dfu[0];	  
		dfu+=4;		//ƫ��4���ֽ�
		iapbuf[i++]=temp;	 	
		if(i==512)
		{
			i=0;
			BSP_FLASHWrite(fwaddr,iapbuf,512);
			fwaddr+=2048; //ƫ��2048  32=4*8.����Ҫ����4.
		}
	}
	if(i)BSP_FLASHWrite(fwaddr,iapbuf,i);//������һЩ�����ֽ�д��ȥ.  
}

/**
  * @brief  ��ת��Ӧ�ó����
  * @param  appxaddr:�û�������ʼ��ַ.
  * @retval None	  
  */
void iap_load_app(u32 appxaddr)
{
	if(((*(vu32*)appxaddr)&0x2FFE0000)==0x20000000)	//���ջ����ַ�Ƿ�Ϸ�.
	{ 
		jump2app=(iapfun)*(vu32*)(appxaddr+4);		//�û��������ڶ�����Ϊ����ʼ��ַ(��λ��ַ)					
		__set_MSP(*(vu32*) appxaddr);  	//��ʼ��APP��ջָ��(�û��������ĵ�һ�������ڴ��ջ����ַ)
		jump2app();									//��ת��APP. 
	}
}	
  
/**
  * @brief  ��ת��Ӧ�ó����
  * @param  appxaddr:�û�������ʼ��ַ.
  * @retval None	  
  */
void check_RunStatu(void)
{	
		OTAFLAG = BSP_FLASHReadWord(OTA_Sign);	// ��ȡ����״̬��־λ
		
		printf("OTAFLAG =  %x\r\n",OTAFLAG);
		switch (OTAFLAG)
		{
			case 0xFFFF:	printf("\r\nû�й̼����ȴ����չ̼�\r\n");break;
			case 0xd0d0:	printf("\r\nִ�й̼�������������\r\n");
										Update_Firmware();
										OTAFLAG = 0xffff;	// ��鲻���̼�
										break;
			case 0xd1d1:	printf("\r\n�����̼����ȴ����ݰ�\r\n");break;
			default : OTAFLAG = 0xffff;	Change_OTAFlag(OTAFLAG);break;
		}
}

/**
  * @brief  ����OTA��־λ	
  * @param  tmp�� FFFF û�й̼�   D0D0 ִ�й̼�
  * @retval None	  
  */
void Change_OTAFlag(u16 tmp)
{
		OTAFLAG = tmp;
		BSP_FLASHWriteWord(OTA_Sign,OTAFLAG);  // 		
}

