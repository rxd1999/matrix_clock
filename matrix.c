#include "stc12.h"
#include <intrins.h>
#define LARGE_FRONT_ADDR 0X00
#define MIDDLE_FRONT_ADDR 0X158
#define SMALL_FRONT_ADDR 0X180
#define sec 0X00
#define min 0X01
#define hour 0X02
#define wk 0x03
#define day 0x04
#define month 0x05
#define year 0x06
#define dot 10
#define ADC_FLAG 0X10
void show_current_time(char hour10,char hour1,char min10,char min1,char sec10,char sec1);
void show8x8(char *p);
void changeTime(unsigned char i,unsigned char tmp);
void Outchr(unsigned char);
void Outstr(char*);
unsigned char IIC_single_byte_read(unsigned char Waddr);
unsigned char strlen(char *p);
unsigned char BCD_Decimal(unsigned char bcd);
sbit SCK_IN_DUAN=P1^0;
sbit SCK_OUT_DUAN=P1^1;
sbit SDA_DUAN=P1^2;
sbit SCK_IN=P1^3;//SPI SCK IN
sbit SDA=P1^4;//SPI DATA
sbit SCK_OUT=P3^5;//SPI CLK OUT
sbit IIC_SDA=P3^3;
sbit IIC_SCL=P3^2;
char Rev[25]="";
char Date[11]="";
unsigned char pos=0;
unsigned char mode=0;
unsigned char ADC_time=20;
unsigned char Read_time=10;
unsigned char got_flag=0;
void Delay100us()		//@11.0592MHz
{
	unsigned char i, j;

	i = 3;
	j = 35;
	do
	{
		while (--j);
	} while (--i);
}

void Iapldle()
{
	IAP_CONTR=0;
	IAP_CMD=0;
	IAP_TRIG=0;
	IAP_ADDRH=0x80;
	IAP_ADDRL=0;
}
char IapReadByte(unsigned int addr)
{
	char dat;//Data buffer
	IAP_CONTR=0x82;//Open IAP function,and set wait time
	IAP_CMD=1;//Set ISP/IAP/EEPROM READ command
	IAP_ADDRL=addr;//Set ISP/IAP/EEPROM address low
	IAP_ADDRH=addr>>8;//Set ISP/IAP/EEPROM address high
	IAP_TRIG=0x5a;//Send trigger command1(0x5a)
	IAP_TRIG=0xa5;//Send trigger command2(0xa5)
	_nop_();//MCU will hold here untilISP/IAP/EEPROM
	//operation complete
	dat=IAP_DATA;//Read ISP/IAP/EEPROM data
	Iapldle();//Close ISPIAP/EEPROM function
	return dat;//Return Flash data
}
unsigned char l_tmpdate[]={0,0,0,0,0,0,0};//秒，分，时，星期，日，月,年
void delay_IIC(void)   
{//IIC总线限速延时函数。
//该函数是空函数，延时4个机器周期。
_nop_();_nop_();
}
void Init()
{	
	//PWM设置----------------------------------------------------------------------
	CL=0;
	CMOD = 0X02;        //  设置脉冲源
 	CCAPM0 = 0X42;      //  开启比较器，允许输出脉宽调制信号
    PCA_PWM0 = 0X00;    //  组成9位比较器，可以设置成1，也可以设置成0
    CCAP0L = 0x80;      //  比较器中的初值
    CCAP0H = 0X80;      //  比较器初值重装
    CR =1;  //  启动计数模式
    P1ASF=0x80;     //将P1^7设为模拟输入口
	ADC_CONTR=0X8F; //开始进行ADC转换,转换周期480个时钟周期
    	//定时器相关配置-----------------------------------------
    AUXR=0X00;
    PCON=0X80;
	TMOD &= 0x0F;		//清除定时器1模式位
	TMOD |= 0x21;		//设定定时器1为8位自动重装方式
	TL1 = 0xfd;		//设定定时初值
	TH1 = 0xfd;		//设定定时器重装值
	ET1 = 0;		//禁止定时器1中断
	TR1 = 1;		//启动定时器1
	SM0=0;
	SM1=1;
	TH0=0X3C;
	TL0=0XB0;
	TR0=1;
	ET0=1;
	EA=1;
	ES=1;
	REN=1;
	//--------------------------------------------------------------
}
void ADC(void)
{
	unsigned char res=0;
	ADC_CONTR=0X8F;
	_nop_();_nop_();_nop_();_nop_();
	while(!ADC_CONTR&ADC_FLAG);
	{
		res=ADC_RES;
		if(res<0x05)
		{
			res=0x05;
		}
		Outchr(res);
		CCAP0H=res;
	}
}
void IIC_start(void)
{//IIC总线产生起始信号函数
     
     IIC_SDA=1;//拉高数据线
     IIC_SCL=1;//拉高时钟线
     delay_IIC();
     IIC_SDA=0;//在时钟线为高电平时，拉低数据线，产生起始信号。
     delay_IIC();
     IIC_SCL=0;//拉低时钟线
}
void IIC_stop(void)
{//IIC总线产生停止信号函数
    IIC_SDA=0;//拉低数据线
    delay_IIC();
    IIC_SCL=1;//拉高时钟线。
    delay_IIC();
    IIC_SDA=1;//时钟时线为高电平时，拉高数据线，产生停止信号。
    delay_IIC();
}
bit IIC_Tack(void)
{//接收应答信号函数
    bit ack;//定义一个位变量，来暂存应答状态。
    IIC_SDA=1;//释放数据总线，准备接收应答信号。
    delay_IIC();
    IIC_SCL=1;//拉高时钟线。
    delay_IIC();
    ack=IIC_SDA;//读取应答信号的状态。
    delay_IIC();
    IIC_SCL=0;//拉低时钟线。
    delay_IIC();
    return ack;//返回应答信号的状态，0表示应答，1表示非应答。
}
void IIC_write_byte(unsigned char Data)
{//向IIC总线写入一个字节的数据函数
    unsigned char i;
    IIC_SCL=0;//拉低时钟线，允许改变数据线的状态
    delay_IIC();
     for(i=0;i<8;i++)//有8位数据
    {
    	IIC_SDA=Data&0x80;//写最高位的数据
        delay_IIC();
        IIC_SCL=1; //拉高时钟线，将数写入到设备中。
        delay_IIC();
        Data=Data<<1;//数据左移一位，把次高位放在最高位,为写入次高位做准备
        IIC_SCL=0;
        delay_IIC();
    }
}
unsigned char IIC_read_byte()
{//从IIC总线读取一个字节的数据函数
    unsigned char retc;
	unsigned char BitCnt;
	retc=0;
	IIC_SDA=1; //置数据线为输入方式
	for(BitCnt=0;BitCnt<8;BitCnt++)
	{
		delay_IIC();
		IIC_SCL=0; //置时钟线为低，准备接收数据位
		delay_IIC(); //时钟低电平周期大于4.7μs
		IIC_SCL=1; //置时钟线为高使数据线上数据有效
		delay_IIC();
		retc=retc<<1;
		if(IIC_SDA==1)
			retc=retc+1; //读数据位，接收的数据位放入retc中
		delay_IIC();
	}
	IIC_SCL=0;
	delay_IIC();
	return(retc);
}
void IIC_single_byte_write(unsigned char Waddr,unsigned char Data)
{//向任意地址写入一个字节数据函数
    IIC_start();//产生起始信号
    IIC_write_byte(0xd0);//写入设备地址（写）
    IIC_Tack();//等待设备的应答
    IIC_write_byte(Waddr);//写入要操作的单元地址。
    IIC_Tack();//等待设备的应答。
    IIC_write_byte(Data);//写入数据。
    IIC_Tack();//等待设备的应答。
    IIC_stop();//产生停止符号。
}
unsigned char IIC_single_byte_read(unsigned char Waddr)
{//从任意地址读取一个字节数据函数
    unsigned char Data=0x00;//定义一个缓冲寄存器。
    IIC_start();//产生起始信号
    IIC_write_byte(0xd0);//写入设备地址（写）
    IIC_Tack();//等待设备的应答
    IIC_write_byte(Waddr);//写入要操作的单元地址。
    IIC_Tack();//等待设备的应答。
   //IIC_stop();
    IIC_start();//产生起始信号
    IIC_write_byte(0xd1);//写入设备地址（写）
    IIC_Tack();//等待设备的应答
    Data=IIC_read_byte();//写入数据。
    IIC_stop();//产生停止符号。
    //-------------------返回读取的数据--------------------
    return Data;//返回读取的一个字节数据。
}
void Outchr(char chr)
{
	SBUF=chr;
	while(!TI);
	TI=0;
}
void Outstr(char *str)
{
	while(*str)
	{
		Outchr(*str);
		str++;
	}
	return ;
}
unsigned char BCD_Decimal(unsigned char bcd)
{ 
	return((bcd>>4)*10+(bcd&0x0F));
}
void Read_RTC()
{
	unsigned char i=0;
	unsigned char tmp=0;
	tmp=BCD_Decimal(IIC_single_byte_read(0x00));
	if (l_tmpdate[0]!=tmp)
	{
		l_tmpdate[0]=tmp;
		for (i=1;i<3;i++)
		{
			l_tmpdate[i]=BCD_Decimal(IIC_single_byte_read(i));
		}
	}
}
void Set_RTC(void)
{
    unsigned char i,tmp;
    for(i=0;i<7;i++)
    {     
    	//BCD处理
        tmp=l_tmpdate[i]/10;
        l_tmpdate[i]=l_tmpdate[i]%10;
        l_tmpdate[i]|=tmp<<4;
    } 
//    IIC_single_byte_write(0x0e,0X8c);
     for(i=0;i<7;i++)        //7次写入 秒、分、时、星期、日、月、年
     {
        IIC_single_byte_write(i,l_tmpdate[i]);
     }
    IIC_single_byte_write(0x0e,0x0c);
}
void main()
{
	Init();
	Read_RTC();
	while(1)
	{
		unsigned char i=0;	
		if (got_flag)
		{
			if (Rev[0]=='t')
			{ 
				l_tmpdate[0]=(Rev[18]-'0')*10+Rev[19]-'0';
				l_tmpdate[1]=(Rev[15]-'0')*10+Rev[16]-'0';
				l_tmpdate[2]=(Rev[12]-'0')*10+Rev[13]-'0';
				l_tmpdate[3]=(Rev[10]-'0');
				l_tmpdate[4]=(Rev[7]-'0')*10+Rev[8]-'0';
				l_tmpdate[5]=(Rev[4]-'0')*10+Rev[5]-'0';
				l_tmpdate[6]=(Rev[1]-'0')*10+Rev[2]-'0';
				Set_RTC();
				Rev[0]='T';
			}
			Date[0]=BCD_Decimal(IIC_single_byte_read(month))/10+'0';
			Date[1]=BCD_Decimal(IIC_single_byte_read(month))%10+'0';
			Date[2]=' ';
			Date[3]=BCD_Decimal(IIC_single_byte_read(day))/10+'0';
			Date[4]=BCD_Decimal(IIC_single_byte_read(day))%10+'0';
			Date[5]=' ';
			Date[6]=BCD_Decimal(IIC_single_byte_read(wk))+'0';
			Date[7]='\0';
			got_flag=0;
		}
		if(!ADC_time)
		{
			ADC();
			ADC_time=20;
		}
		if(!Read_time)
		{
			Read_RTC();
			Read_time=4;
		}
		if(mode==0)
			show_current_time(l_tmpdate[2]/10,l_tmpdate[2]%10,l_tmpdate[1]/10,l_tmpdate[1]%10,l_tmpdate[0]/10,l_tmpdate[0]%10);
		else
		{
			show8x8(Date);
		}
	}
}
void show_current_time(char hour10,char hour1,char min10,char min1,char sec10,char sec1)
{
	unsigned char i,j,k;
	unsigned char duan;
	//unsigned char Animation[2]={0x14,0x22};
	SDA=0;
	for (j=0;j<29;j++)
		{
			SCK_IN=0;
			SCK_OUT=0;
			k=j/4;
			switch(k)
			{
				case 0:duan=IapReadByte(SMALL_FRONT_ADDR+hour10*4+j%4);break;
				case 1:duan=IapReadByte(SMALL_FRONT_ADDR+hour1*4+j%4);break;
				case 2:duan=0x14;j+=1;if (j==11) duan=0x00; break;
				case 3:duan=IapReadByte(SMALL_FRONT_ADDR+min10*4+j%4);break;
				case 4:duan=IapReadByte(SMALL_FRONT_ADDR+min1*4+ j%4);break;
				case 5:duan=0x00;j+=1;break;
				case 6:duan=IapReadByte(MIDDLE_FRONT_ADDR+sec1*4+j%4);break;
			}
			if (sec10>k)
			{
				duan|=0x80;
			}
			SCK_OUT_DUAN=0;
			SCK_IN=1;
				for (i = 0; i < 8; ++i)
				{
					SDA_DUAN=duan&0x80;
					SCK_IN_DUAN=0;
					duan<<=1;
					SCK_IN_DUAN=1;
				}
				SCK_OUT=SCK_OUT_DUAN=1;
			Delay100us();
			SDA=1;
		}
}
void show8x8(char *p)
{
	unsigned char len=0;
	unsigned char NO=0;
	static char speed=0; 
	unsigned char character,duan,i;
	len=strlen(p);
	if (pos>len*8-1)
	{
		pos=0;
	}
	NO=pos;
	SDA=0;
	for(pos;pos<NO+32;pos++)
	{
		SCK_IN=0;
		SCK_OUT=0;
		SCK_IN=1;
		if (pos<len*8)
		{
			character=*(p+pos/8);
			if (character==' ')
			{
				character=16+'0';
			}
			duan=IapReadByte(LARGE_FRONT_ADDR+(character-'0')*8+pos%8);
		}
		else
			duan=0X00;
		SCK_OUT_DUAN=0;
		for (i = 0; i < 8; ++i)
		{
			SDA_DUAN=0X80&duan;
			SCK_IN_DUAN=0;
			duan<<=1;
			SCK_IN_DUAN=1;
		}
		SDA=1;
		SCK_OUT_DUAN=1;
		SCK_OUT=1;
	}
	pos=NO;
	speed++;
	if (len>4 && speed==50)
	{
		speed=0;
		pos++;
	}
}
unsigned char strlen(char *p)
{
	unsigned char len=0;
	while(*p)
	{
		++len;
		p++;
	}
	return len;
}
void timer0() interrupt 1
{
	TH0=0X3C;
	TL0=0XB0;
	if(ADC_time)
		ADC_time--;
	if(Read_time)
		Read_time--;
}
void Rec() interrupt 4
{
	unsigned char temp;
	static unsigned char i=0;
	if(RI)
	{
		RI=0;
		temp=SBUF;
		//Outchr(temp);
		if(temp!='\n')
		{
			Rev[i]=temp;
			if(i<24)
			{
				i++;
			}
		}
		else
		{
			got_flag=1;
			Rev[i]='\0';
			if (Rev[0]=='s')
			{
				mode=!mode;
				Outstr("ok");
			}
			i=0;
		}

	}
}
