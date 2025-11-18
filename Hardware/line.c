#include "Line.h"
#include "motor.h"
#include "sensor.h"
#include "usart.h"
#include "iic.h"
#include "delay.h"

unsigned char lukou_num = 0; //全局变量定义检测到路口的次数

/*************************************
*函数名称：track_zhixian1()
*函数功能：直线循迹
*参数：
*说明：数字口获取数字量
*			白底黑线，线宽2厘米，其他线宽根据实际情况改写，提供的程序只供参考。
**************************************/
void track_zhixian1()
{    
	unsigned char num = 0,i;  //num个灯压线认为是到达路口
	
	for(i=0;i<2;i++) //循环检测路口2次
	{
		if(D1 == 1)  num++;	if(D2 == 1)  num++;	if(D3 == 1)  num++;	if(D4 == 1)  num++;
		if(D5 == 1)  num++;	
		
		if(num >= (ADC_N)) //大于等于ADC_N个灯压线认为是到达路口
		{ 
			lukou_num++; 
			if(lukou_num == 1)	 Delay_ms(10); //第一次检测到延时10ms，消抖操作
		}  
		num = 0;
	}
	if(lukou_num >= 2) { lukou_num = 0; motor(0,0); Delay_ms(1000);Delay_ms(1000);}  //检测2次都是路口后才认为是真正到达路口，防止误判
                                                                                   //这里的逻辑是1路在小车的左边，ADC_N路在小车的右边
																																										 //5 4321（路）
  if((D1 == 1)&&(D2 == 0)&&(D3 == 0)&&(D4 == 0)&&(D5 == 0))        motor(0,40);      //0 0001
  else if((D1 == 1)&&(D2 == 1)&&(D3 == 0)&&(D4 == 0)&&(D5 == 0))   motor(10,40);     //0 0011
  else if((D1 == 0)&&(D2 == 1)&&(D3 == 0)&&(D4 == 0)&&(D5 == 0))   motor(25,40);     //0 0010
  else if((D1 == 0)&&(D2 == 1)&&(D3 == 1)&&(D4 == 0)&&(D5 == 0))   motor(35,40);     //0 0110
  else if((D1 == 0)&&(D2 == 0)&&(D3 == 1)&&(D4 == 0)&&(D5 == 0))   motor(40,40);     //0 0100 //正中间位置
  else if((D1 == 0)&&(D2 == 0)&&(D3 == 1)&&(D4 == 1)&&(D5 == 0))   motor(40,35);     //0 1100
  else if((D1 == 0)&&(D2 == 0)&&(D3 == 0)&&(D4 == 1)&&(D5 == 0))   motor(40,25);     //0 1000
  else if((D1 == 0)&&(D2 == 0)&&(D3 == 0)&&(D4 == 1)&&(D5 == 1))   motor(40,10);     //1 1000
  else if((D1 == 0)&&(D2 == 0)&&(D3 == 0)&&(D4 == 0)&&(D5 == 1))   motor(40,0);      //1 0000
  else   motor(20,20);

}
/*************************************
*函数名称：track_zhixian2()
*函数功能：直线循迹
*参数：
*说明：串口获取数字量数据,IIC获取数字量数据
*			白底黑线，线宽2厘米，其他线宽根据实际情况改写，提供的程序只供参考。
**************************************/
void track_zhixian2()
{	
	unsigned int Temp[1] = { 0 };       //数据缓存区
	unsigned char num = 0,Temp_data = 0,i;       //数据临时存放

	Read_Data1(0x01,Temp);	//Read_IICData1(0x01,Temp);


	Temp_data = Temp[0];
	for(i=0;i<2;i++) //循环检测路口2次
	{
		while(Temp_data)
		{
			Temp_data &= (Temp_data - 1);  //判断该字节中含有“1”的个数
			num++;
		}	
		if(num >= (ADC_N)) //大于等于ADC_N个灯压线认为是到达路口
		{ 
			lukou_num++; 
			if(lukou_num == 1)	Delay_ms(10); //第一次检测到延时10ms，消抖操作
		}  
		num = 0;
	}
	if(lukou_num >= 2) { lukou_num = 0; motor(0,0); Delay_ms(1000);Delay_ms(1000);}  //检测2次都是路口后才认为是真正到达路口，防止误判

	switch(Temp[0])                           //这里的逻辑是1路在小车的左边，ADC_N路在小车的右边
	{                                         //5 4321 （路）
		case 0x01:		motor(0,40);	break;      //0 0001
		case 0x03:		motor(10,40);	break;      //0 0011
		case 0x02:		motor(25,40);	break;      //0 0010
		case 0x06:		motor(35,40);	break;      //0 0110
		case 0x04:		motor(40,40);	break;      //0 0100	//正中间位置
		case 0x0C:		motor(40,35);	break;      //0 1100
		case 0x08:		motor(40,25);	break;      //0 1000
		case 0x18:		motor(40,10);	break;      //1 1000
		case 0x10:		motor(40,0);	break;      //1 0000			
		
		default :			motor(20,20);	break;
	}
}
/*************************************
*函数名称：track_PID1
*函数功能：直线循迹，用串口线连接，输出偏差值等位置数据
*参数：pwm：最大速度值(例程取值范围0至90)，P：比例系数(例程取值范围0.01至0.1)
*说明：
*			白底黑线，线宽2厘米，其他线宽根据实际情况改写，提供的程序只供参考。
**************************************/
void track_PID1(int pwm,float P)
{
//	static float Integral_error,Last_error;
	static int L_Pwm,R_Pwm;			 //左右轮速度
	unsigned int Temp[2] = { 0 }; //数据缓存区
	int error = 0;         //偏差值
//	float I = 0,D = 0;		 //积分系数，微分系数，取值范围0.01-0.9
	
	Read_Data2(0x01,Temp);	//Read_IICData2(0x01,Temp);

	if((Temp[0]&0x1F) != 0)		//在线上
	{	
		if((Temp[0]&0x1F) != ADC_N)		//部分在线上
		{
			if(((Temp[0] >> 5)%2) == 0)
			{
				error = -Temp[1];
			}
			else if(((Temp[0] >> 5)%2) == 1)
			{
				error = Temp[1];
			}
//			Integral_error += error;
			
//			L_Pwm = (pwm+(error*P+Integral_error*I+(error-Last_error)*D));
//			R_Pwm = (pwm-(error*P+Integral_error*I+(error-Last_error)*D));
			L_Pwm = pwm+error*P;
			R_Pwm = pwm-error*P;	
//			Last_error = error;
//////////////////最高速和最低速限制///////////////////////////
			if(L_Pwm > (pwm+10))
				L_Pwm = (pwm+10);
			if(R_Pwm > (pwm+10))
				R_Pwm = (pwm+10);
			if(L_Pwm <= 15)
				L_Pwm = 15;
			if(R_Pwm <= 15)
				R_Pwm = 15;
//////////////////最高速和最低速限制///////////////////////////
			motor(L_Pwm,R_Pwm);
		}
		else if((Temp[0]&0x1F) == ADC_N)		//全部在线上
		{
			motor(0,0);
			Delay_ms(1000);Delay_ms(1000);
		}		
	}
	else		//出线
	{
		if(((Temp[0] >> 6)%2) == 0)		//左出线
		{
			motor(10,pwm);
		}
		else if(((Temp[0] >> 6)%2) == 1)		//右出线
		{
			motor(pwm,10);
		}
	}
}
/*************************************
*函数名称：track_PID2
*函数功能：直线循迹，用串口线连接，输出模拟量数据
*参数：pwm：最大速度值(例程取值范围0至90)，P：比例系数(例程取值范围0.01至0.1)
*说明：
*			白底黑线，线宽2厘米，其他线宽根据实际情况改写，提供的程序只供参考。
**************************************/
void track_PID2(int pwm,float P)
{
//	static float Integral_error=0,Last_error=0;
	static int L_Pwm=0,R_Pwm=0;			 //左右轮速度
	unsigned char i;
	int H_SETPOINT = 3000; //灰度传感器居中参照值 （需要修改）
	int error = 0;         //偏差值
	float All_Channel = 0.0;   //灰度值总和
	float All_Result = 0.0;    //处理过后的灰度总和
	unsigned int Finall_Result = 0; //最终处理值
//	float I = 0,D = 0;		 //积分系数，微分系数，取值范围0.01-0.9
	unsigned int Temp[ADC_N] = { 0 };     //数据缓存区ADC_N个模拟值

	Read_Data3(0x01,Temp);	//Read_IICData3(0x01,Temp);

	for(i=0; i<ADC_N; i++) //ADC_N个模拟值
	{
		All_Channel = All_Channel+Temp[i];
		All_Result  = All_Result+(Temp[i]*(i+1));
	}
	
	Finall_Result = All_Result/All_Channel*1000;
	error = H_SETPOINT - Finall_Result;
//	Integral_error += error;
	
///////////////场地背景色比线的颜色深/////////////////////////////////	
//	L_Pwm = (pwm-(error*P+Integral_error*I+(error-Last_error)*D));
//	R_Pwm = (pwm+(error*P+Integral_error*I+(error-Last_error)*D));
//	L_Pwm = pwm-error*P;
//	R_Pwm = pwm+error*P;	

///////////////场地背景色比线的颜色深/////////////////////////////////	

///////////////场地背景色比线的颜色浅/////////////////////////////////	
//	L_Pwm = (pwm+(error*P+Integral_error*I+(error-Last_error)*D));
//	R_Pwm = (pwm-(error*P+Integral_error*I+(error-Last_error)*D));
	L_Pwm = pwm+error*P;
	R_Pwm = pwm-error*P;	
///////////////场地背景色比线的颜色浅/////////////////////////////////	
	
//	Last_error = error;
			
//////////////////最高速和最低速限制///////////////////////////
	if(L_Pwm > (pwm+10))
		L_Pwm = (pwm+10);
	if(R_Pwm > (pwm+10))
		R_Pwm = (pwm+10);
	if(L_Pwm <= 15)
		L_Pwm = 15;
	if(R_Pwm <= 15)
		R_Pwm = 15;
//////////////////最高速和最低速限制///////////////////////////
	
	motor(L_Pwm,R_Pwm);
}
/*************************************
*函数名称：track_PID3
*函数功能：直线循迹，用串口线连接，输出Temp[0]数字量、Temp[1]Temp[2]偏差值、Temp[3]~Temp[9]模拟量等数据
*参数：pwm：最大速度值(例程取值范围0至90)，P：比例系数(例程取值范围0.01至0.1)
*说明：
*			白底黑线，线宽2厘米，其他线宽根据实际情况改写，提供的程序只供参考。
**************************************/
void track_PID3(int pwm,float P)
{
//	static float Integral_error,Last_error;
	static int L_Pwm,R_Pwm;			 //左右轮速度
	unsigned int Temp[10] = { 0 };  //数据缓存区
	int error = 0;         //偏差值
//	float I = 0,D = 0;		 //积分系数，微分系数，取值范围0.01-0.1
	
	Read_Data4(0x01,Temp);	

	if((Temp[1]&0x1F) != 0)		//在线上
	{	
		if((Temp[1]&0x1F) != ADC_N)		//部分在线上
		{
			if(((Temp[1] >> 5)%2) == 0)
			{
				error = -Temp[2];
			}
			else if(((Temp[1] >> 5)%2) == 1)
			{
				error = Temp[2];
			}
//			Integral_error += error;
			
//			L_Pwm = (pwm+(error*P+Integral_error*I+(error-Last_error)*D));
//			R_Pwm = (pwm-(error*P+Integral_error*I+(error-Last_error)*D));
			L_Pwm = pwm+error*P;
			R_Pwm = pwm-error*P;	
			
//			Last_error = error;
//////////////////最高速和最低速限制///////////////////////////
			if(L_Pwm > (pwm+10))
				L_Pwm = (pwm+10);
			if(R_Pwm > (pwm+10))
				R_Pwm = (pwm+10);
			if(L_Pwm <= 15)
				L_Pwm = 15;
			if(R_Pwm <= 15)
				R_Pwm = 15;
//////////////////最高速和最低速限制///////////////////////////
			motor(L_Pwm,R_Pwm);
		}
		else if((Temp[1]&0x1F) == ADC_N)		//全部在线上
		{
			motor(0,0);
			Delay_ms(1000);Delay_ms(1000);
		}		
	}
	else		//出线
	{
		if(((Temp[1] >> 6)%2) == 0)		//左出线
		{
			motor(10,pwm);
		}
		else if(((Temp[1] >> 6)%2) == 1)		//右出线
		{
			motor(pwm,10);
		}
	}
}



