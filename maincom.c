#include "sys.h"
#include "delay.h"
#include "usart.h"        //gjw 181215 
#include "led.h" 		      //gjw 181215  	 
#include "lcd.h"          //gjw 181215 
#include "key.h"          //gjw 181215 
#include "usmart.h"       //gjw 181215 
#include "malloc.h"       //gjw 181215 
#include "sdio_sdcard.h"  //gjw 181215 
#include "w25qxx.h"       //gjw 181215 
#include "ff.h"           //gjw 181215 
#include "exfuns.h"       //gjw 181215 
#include "text.h"	        //gjw 181215 
#include "touch.h"		    //gjw 181215 
#include "common.h"  	 
#include "usart2.h" 
#include "usart3.h"
#include "gps.h"	 	 
#include "string.h"	


char *inputdata;   //wifi
char *gpsdata;

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN]; 					//����1,���ͻ�����
nmea_msg gpsx; 											//GPS��Ϣ
__align(4) u8 dtbuf[50];   								//��ӡ������
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode�ַ��� 

u8 GPS_DATA_BUF[512];  //20181224,cxw,check
	  
//��ʾGPS��λ��Ϣ 
void Gps_Msg_Show(void)
{
 	float tp;		   
	POINT_COLOR=BLUE;  	 
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//�õ������ַ���
 	LCD_ShowString(30,200,200,16,16,dtbuf);	 
	
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//�õ�γ���ַ���
 	LCD_ShowString(30,220,200,16,16,dtbuf);	 
	
	tp=gpsx.altitude;	   
 	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//�õ��߶��ַ���
 	LCD_ShowString(30,240,200,16,16,dtbuf);	

	tp=gpsx.speed;	   
 	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//�õ��ٶ��ַ���	 
 	LCD_ShowString(30,260,200,16,16,dtbuf);	 				    
	
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//��ʾUTC����
	LCD_ShowString(30,280,200,16,16,dtbuf);	
	
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//��ʾUTCʱ��
  LCD_ShowString(30,300,200,16,16,dtbuf);		  

}


/************************* MAIN *******************************************************/

int main(void)
{	
	u8 fontok=0; 	
	u16 i,rxlen;
	u16 lenx;
	u8 key=0XFF;
	u8 upload=0;		
	u8 netpro=0;	        //����ģʽ;  //gjw25
	u8 timex=0; 
	u8 ipbufserver[16]; 	//IP����   //gjw181225-
	u8 *p;
	u16 t=999;		        //���ٵ�һ�λ�ȡ����״̬
	u8 res=0;
	u16 rlen=0;
	u8 constate=0;	      //����״̬
	p=mymalloc(SRAMIN,32);

	
	delay_init();	    	     //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	    //���ڳ�ʼ��Ϊ115200
 	usmart_dev.init(72);		  //��ʼ��USMART		
 	LED_Init();		  			    //��ʼ����LED���ӵ�Ӳ���ӿ�          
	LCD_Init();			   		    //��ʼ��LCD            
	usart3_init(115200);		  //��ʼ������3 
 	my_mem_init(SRAMIN);		  //��ʼ���ڲ��ڴ�� 
	delay_init();	    	      //��ʱ������ʼ��	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	    //���ڳ�ʼ��Ϊ115200
 	usmart_dev.init(72);		  //��ʼ��USMART		
	KEY_Init();					      //��ʼ������
	usart2_init(38400);		    //��ʼ������2 
	fontok=font_init();			//����ֿ��Ƿ�OK
		
	POINT_COLOR=RED;	
	Show_Str_Mid(0,30,"�������� GPS+WIFI ����",16,240);   //gjw 181215
	while(atk_8266_send_cmd("AT","OK",20))         //���WIFIģ���Ƿ�����
	{
		atk_8266_quit_trans();//�˳�͸��
		atk_8266_send_cmd("AT+CIPMODE=0","OK",200);  //�ر�͸��ģʽ	

	} 
		
	atk_8266_at_response(1);                     //���ATK-ESP8266ģ�鷢�͹���������,��ʱ�ϴ�������//gjw 181215+
	atk_8266_send_cmd("AT+CWMODE=1","OK",50);		 //����WIFI STAģʽ
	atk_8266_send_cmd("AT+RST","OK",20);		     //DHCP�������ر�(��APģʽ��Ч) 
	delay_ms(1000);                              //��ʱ3S�ȴ������ɹ�
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	
	//�������ӵ���WIFI��������/���ܷ�ʽ/����,�⼸��������Ҫ�������Լ���·�������ý����޸�!!
	sprintf((char*)p,"AT+CWJAP=\"%s\",\"%s\"",wifista_ssid,wifista_password);//�������߲���:ssid,����
	while(atk_8266_send_cmd(p,"WIFI GOT IP",300));				                 	 //����Ŀ��·����,���һ��IP
	atk_8266_send_cmd("AT+CIPMUX=0","OK",20);                                //0�������ӣ�1�������� //gjw181225 check+
	sprintf((char*)p,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipbuf,(u8*)portnum);    //����Ŀ��TCP����
	atk_8266_send_cmd(p,"OK",200);                //gjw181225+
	atk_8266_send_cmd("AT+CIPMODE=1","OK",200);   //����ģʽΪ��͸��
	atk_8266_get_wanip(ipbufserver);              //������ģʽ,��ȡWAN IP
	sprintf((char*)p,"IP��ַ:%s �˿�:%s",ipbufserver,(u8*)portnum);
		
	Show_Str(30,65,200,12,p,12,0);				        //��ʾIP��ַ�Ͷ˿�	
//	Show_Str(30,80,200,12,"״̬:",12,0); 		      //����״̬
	Show_Str(30,80,200,12,"ģʽ:",12,0); 	    	//����״̬
	Show_Str(30,100,200,12,"��������:",12,0); 	  //��������
//	Show_Str(30+30,80,200,12,"���ӳɹ�",12,0);  //����״̬
	atk_8266_wificonf_show(30,135,"������·�������߲���Ϊ:",(u8*)wifista_ssid,(u8*)wifista_encryption,(u8*)wifista_password);
	POINT_COLOR=BLUE;
	Show_Str(30+30,80,200,12,(u8*)ATK_ESP8266_WORKMODE_TBL[1],12,0); 		//����״̬	
	USART3_RX_STA=0;		
	
	if(SkyTra_Cfg_Rate(5)!=0)	//���ö�λ��Ϣ�����ٶ�Ϊ5Hz,˳���ж�GPSģ���Ƿ���λ. 
	{
		do
		{
		  usart2_init(9600);			//��ʼ������3������Ϊ9600
		  SkyTra_Cfg_Prt(3);			//��������ģ��Ĳ�����Ϊ38400
		  usart2_init(38400);			//��ʼ������3������Ϊ38400
		  key=SkyTra_Cfg_Tp(100000);	//������Ϊ100ms
		}while(SkyTra_Cfg_Rate(5)!=0&&key!=0);//����SkyTraF8-BD�ĸ�������Ϊ5Hz
		
	}
	while(1) 
	{	
		//wifi beigin gjw
		atk_8266_quit_trans();
		atk_8266_send_cmd("AT+CIPSEND","OK",20); //��ʼ͸�� 
		//wifi end gjw
		

		delay_ms(1);
		if(USART2_RX_STA&0X8000)		             //���յ�һ��������
		{
			rxlen=USART2_RX_STA&0X7FFF;	           //�õ����ݳ���
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART2_RX_BUF[i];	   
 			USART2_RX_STA=0;		          	       //������һ�ν���
			USART1_TX_BUF[i]=0;			               //�Զ���ӽ�����                      //USART1
			GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//�����ַ���                    //USART1
			GPS_Analysis(&gpsx,(u8*)GPS_DATA_BUF); //�����ַ���  
			POINT_COLOR=BLUE;
			Gps_Msg_Show();				                 //��ʾ��Ϣ	
		
			if(upload)printf("\r\n%s\r\n",USART1_TX_BUF);//���ͽ��յ������ݵ�����1   ///USART1				
			sprintf((char *)p,"##%d,%05d,%05d,%02d,%04d,%02d,%02d,%02d,%02d,%02d##\r\n ",1,
				                                                                  gpsx.longitude,			
				                                                                  gpsx.latitude, 
			                                                                    gpsx.altitude,
				                                                                  gpsx.utc.year,gpsx.utc.month,gpsx.utc.date,
			                                                                    gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//�õ������ַ���	
			//	longitude;		  //���� 
			//  latitude;				//γ�� 	
			//  altitude;			 	//�߶�
			//  speed;					//�ٶ�
						
			
//		sprintf((char *)p,"##%d,%05d,%05d,%02d,%04d,%02d,%02d,%02d,%02d,%02d##\r\n ",1,117999,39888,54,2018,1,2,12,13,45);	//�õ������ַ���	

		Show_Str(30,115,200,12,p,12,0);	
		u3_printf("%s",p);
		delay_ms(100);
		timex=100;

		//wifi begin gjw
		if(timex)timex--;
		if(timex==1)LCD_Fill(30+54,100,239,112,WHITE);
		t++;		
		delay_ms(10);
			
		if(USART3_RX_STA&0X8000)		  //���յ�һ��������
		{ 
			rlen=USART3_RX_STA&0X7FFF;	//�õ����ν��յ������ݳ���
			USART3_RX_BUF[rlen]=0;		  //��ӽ����� 
			printf("%s",USART3_RX_BUF);	//���͵�����
			USART3_RX_STA=0;
			if(constate!='+')t=1000;		//״̬Ϊ��δ����,������������״̬
			else t=0;                   //״̬Ϊ�Ѿ�������,10����ټ��
		}  
		if((t%20)==0)LED0=!LED0;
		atk_8266_at_response(1);
	 }
	  myfree(SRAMIN,p);		//�ͷ��ڴ�    wifi gjw
  }

}
