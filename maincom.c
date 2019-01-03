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

u8 USART1_TX_BUF[USART2_MAX_RECV_LEN]; 					//串口1,发送缓存区
nmea_msg gpsx; 											//GPS信息
__align(4) u8 dtbuf[50];   								//打印缓存器
const u8*fixmode_tbl[4]={"Fail","Fail"," 2D "," 3D "};	//fix mode字符串 

u8 GPS_DATA_BUF[512];  //20181224,cxw,check
	  
//显示GPS定位信息 
void Gps_Msg_Show(void)
{
 	float tp;		   
	POINT_COLOR=BLUE;  	 
	tp=gpsx.longitude;	   
	sprintf((char *)dtbuf,"Longitude:%.5f %1c   ",tp/=100000,gpsx.ewhemi);	//得到经度字符串
 	LCD_ShowString(30,200,200,16,16,dtbuf);	 
	
	tp=gpsx.latitude;	   
	sprintf((char *)dtbuf,"Latitude:%.5f %1c   ",tp/=100000,gpsx.nshemi);	//得到纬度字符串
 	LCD_ShowString(30,220,200,16,16,dtbuf);	 
	
	tp=gpsx.altitude;	   
 	sprintf((char *)dtbuf,"Altitude:%.1fm     ",tp/=10);	    			//得到高度字符串
 	LCD_ShowString(30,240,200,16,16,dtbuf);	

	tp=gpsx.speed;	   
 	sprintf((char *)dtbuf,"Speed:%.3fkm/h     ",tp/=1000);		    		//得到速度字符串	 
 	LCD_ShowString(30,260,200,16,16,dtbuf);	 				    
	
	sprintf((char *)dtbuf,"UTC Date:%04d/%02d/%02d   ",gpsx.utc.year,gpsx.utc.month,gpsx.utc.date);	//显示UTC日期
	LCD_ShowString(30,280,200,16,16,dtbuf);	
	
	sprintf((char *)dtbuf,"UTC Time:%02d:%02d:%02d   ",gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//显示UTC时间
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
	u8 netpro=0;	        //网络模式;  //gjw25
	u8 timex=0; 
	u8 ipbufserver[16]; 	//IP缓存   //gjw181225-
	u8 *p;
	u16 t=999;		        //加速第一次获取链接状态
	u8 res=0;
	u16 rlen=0;
	u8 constate=0;	      //连接状态
	p=mymalloc(SRAMIN,32);

	
	delay_init();	    	     //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	    //串口初始化为115200
 	usmart_dev.init(72);		  //初始化USMART		
 	LED_Init();		  			    //初始化与LED连接的硬件接口          
	LCD_Init();			   		    //初始化LCD            
	usart3_init(115200);		  //初始化串口3 
 	my_mem_init(SRAMIN);		  //初始化内部内存池 
	delay_init();	    	      //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	    //串口初始化为115200
 	usmart_dev.init(72);		  //初始化USMART		
	KEY_Init();					      //初始化按键
	usart2_init(38400);		    //初始化串口2 
	fontok=font_init();			//检查字库是否OK
		
	POINT_COLOR=RED;	
	Show_Str_Mid(0,30,"数据中心 GPS+WIFI 测试",16,240);   //gjw 181215
	while(atk_8266_send_cmd("AT","OK",20))         //检查WIFI模块是否在线
	{
		atk_8266_quit_trans();//退出透传
		atk_8266_send_cmd("AT+CIPMODE=0","OK",200);  //关闭透传模式	

	} 
		
	atk_8266_at_response(1);                     //检查ATK-ESP8266模块发送过来的数据,及时上传给电脑//gjw 181215+
	atk_8266_send_cmd("AT+CWMODE=1","OK",50);		 //设置WIFI STA模式
	atk_8266_send_cmd("AT+RST","OK",20);		     //DHCP服务器关闭(仅AP模式有效) 
	delay_ms(1000);                              //延时3S等待重启成功
	delay_ms(1000);
	delay_ms(1000);
	delay_ms(1000);
	
	//设置连接到的WIFI网络名称/加密方式/密码,这几个参数需要根据您自己的路由器设置进行修改!!
	sprintf((char*)p,"AT+CWJAP=\"%s\",\"%s\"",wifista_ssid,wifista_password);//设置无线参数:ssid,密码
	while(atk_8266_send_cmd(p,"WIFI GOT IP",300));				                 	 //连接目标路由器,并且获得IP
	atk_8266_send_cmd("AT+CIPMUX=0","OK",20);                                //0：单连接，1：多连接 //gjw181225 check+
	sprintf((char*)p,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipbuf,(u8*)portnum);    //配置目标TCP服务
	atk_8266_send_cmd(p,"OK",200);                //gjw181225+
	atk_8266_send_cmd("AT+CIPMODE=1","OK",200);   //传输模式为：透传
	atk_8266_get_wanip(ipbufserver);              //服务器模式,获取WAN IP
	sprintf((char*)p,"IP地址:%s 端口:%s",ipbufserver,(u8*)portnum);
		
	Show_Str(30,65,200,12,p,12,0);				        //显示IP地址和端口	
//	Show_Str(30,80,200,12,"状态:",12,0); 		      //连接状态
	Show_Str(30,80,200,12,"模式:",12,0); 	    	//连接状态
	Show_Str(30,100,200,12,"发送数据:",12,0); 	  //发送数据
//	Show_Str(30+30,80,200,12,"连接成功",12,0);  //连接状态
	atk_8266_wificonf_show(30,135,"请设置路由器无线参数为:",(u8*)wifista_ssid,(u8*)wifista_encryption,(u8*)wifista_password);
	POINT_COLOR=BLUE;
	Show_Str(30+30,80,200,12,(u8*)ATK_ESP8266_WORKMODE_TBL[1],12,0); 		//连接状态	
	USART3_RX_STA=0;		
	
	if(SkyTra_Cfg_Rate(5)!=0)	//设置定位信息更新速度为5Hz,顺便判断GPS模块是否在位. 
	{
		do
		{
		  usart2_init(9600);			//初始化串口3波特率为9600
		  SkyTra_Cfg_Prt(3);			//重新设置模块的波特率为38400
		  usart2_init(38400);			//初始化串口3波特率为38400
		  key=SkyTra_Cfg_Tp(100000);	//脉冲宽度为100ms
		}while(SkyTra_Cfg_Rate(5)!=0&&key!=0);//配置SkyTraF8-BD的更新速率为5Hz
		
	}
	while(1) 
	{	
		//wifi beigin gjw
		atk_8266_quit_trans();
		atk_8266_send_cmd("AT+CIPSEND","OK",20); //开始透传 
		//wifi end gjw
		

		delay_ms(1);
		if(USART2_RX_STA&0X8000)		             //接收到一次数据了
		{
			rxlen=USART2_RX_STA&0X7FFF;	           //得到数据长度
			for(i=0;i<rxlen;i++)USART1_TX_BUF[i]=USART2_RX_BUF[i];	   
 			USART2_RX_STA=0;		          	       //启动下一次接收
			USART1_TX_BUF[i]=0;			               //自动添加结束符                      //USART1
			GPS_Analysis(&gpsx,(u8*)USART1_TX_BUF);//分析字符串                    //USART1
			GPS_Analysis(&gpsx,(u8*)GPS_DATA_BUF); //分析字符串  
			POINT_COLOR=BLUE;
			Gps_Msg_Show();				                 //显示信息	
		
			if(upload)printf("\r\n%s\r\n",USART1_TX_BUF);//发送接收到的数据到串口1   ///USART1				
			sprintf((char *)p,"##%d,%05d,%05d,%02d,%04d,%02d,%02d,%02d,%02d,%02d##\r\n ",1,
				                                                                  gpsx.longitude,			
				                                                                  gpsx.latitude, 
			                                                                    gpsx.altitude,
				                                                                  gpsx.utc.year,gpsx.utc.month,gpsx.utc.date,
			                                                                    gpsx.utc.hour,gpsx.utc.min,gpsx.utc.sec);	//得到经度字符串	
			//	longitude;		  //经度 
			//  latitude;				//纬度 	
			//  altitude;			 	//高度
			//  speed;					//速度
						
			
//		sprintf((char *)p,"##%d,%05d,%05d,%02d,%04d,%02d,%02d,%02d,%02d,%02d##\r\n ",1,117999,39888,54,2018,1,2,12,13,45);	//得到经度字符串	

		Show_Str(30,115,200,12,p,12,0);	
		u3_printf("%s",p);
		delay_ms(100);
		timex=100;

		//wifi begin gjw
		if(timex)timex--;
		if(timex==1)LCD_Fill(30+54,100,239,112,WHITE);
		t++;		
		delay_ms(10);
			
		if(USART3_RX_STA&0X8000)		  //接收到一次数据了
		{ 
			rlen=USART3_RX_STA&0X7FFF;	//得到本次接收到的数据长度
			USART3_RX_BUF[rlen]=0;		  //添加结束符 
			printf("%s",USART3_RX_BUF);	//发送到串口
			USART3_RX_STA=0;
			if(constate!='+')t=1000;		//状态为还未连接,立即更新连接状态
			else t=0;                   //状态为已经连接了,10秒后再检查
		}  
		if((t%20)==0)LED0=!LED0;
		atk_8266_at_response(1);
	 }
	  myfree(SRAMIN,p);		//释放内存    wifi gjw
  }

}
