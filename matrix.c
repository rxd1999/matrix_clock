#include "stc12.h"
#include <intrins.h>
#define dot 10
#define ADC_FLAG 0X10
void show(char a,char b,char c,char d,char e ,char f,char g,char h);
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
char Rev[20]="";
char Date[11]="20";
unsigned char pos=0;
unsigned char mode=0;
code unsigned char addr[]={0x00,0x01,0x02,0x04,0x05,0x06};
unsigned char ADC_time=20;
unsigned char Read_time=10;
code unsigned char table[][4]={
	{0x3e,0x22,0x3e,0x00,},//0
	{0x24,0x3e,0x20,0x00,},//1
	{0x3a,0x2a,0x2e,0x00,},//2
	{0x2a,0x2a,0x3e,0x00,},//3
	{0x0e,0x08,0x3e,0x00,},//4
	{0x2e,0x2a,0x3a,0x00,},//5
	{0x3e,0x2a,0x3a,0x00,},//6
	{0x06,0x02,0x3e,0x00,},//7
	{0x3e,0x2a,0x3e,0x00,},//8
	{0x2e,0x2a,0x3e,0x00,},//9
	{0x00,0x36,0x36,0x00,}//dot
};
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
unsigned char l_tmpdate[]={50,59,23,31,7,19};//秒，分，时，日，月,年
void delay_IIC(void)   
{//IIC总线限速延时函数。
//该函数是空函数，延时4个机器周期。
_nop_();_nop_();
}
void Init()
{	
	Date[2]=BCD_Decimal(IIC_single_byte_read(addr[5]))/10+'0';
	Date[3]=BCD_Decimal(IIC_single_byte_read(addr[5]))%10+'0';
	Date[4]=' ';
	Date[5]=BCD_Decimal(IIC_single_byte_read(addr[4]))/10+'0';
	Date[6]=BCD_Decimal(IIC_single_byte_read(addr[4]))%10+'0';
	Date[7]=' ';
	Date[8]=BCD_Decimal(IIC_single_byte_read(addr[3]))/10+'0';
	Date[9]=BCD_Decimal(IIC_single_byte_read(addr[3]))%10+'0';
	Date[10]='\0';
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
	tmp=BCD_Decimal(IIC_single_byte_read(addr[0]));
	if (l_tmpdate[0]!=tmp)
	{
		l_tmpdate[0]=tmp;
		for (i=1;i<3;i++)
		{
			l_tmpdate[i]=BCD_Decimal(IIC_single_byte_read(addr[i]));
		}
		if (l_tmpdate[2]==23&&l_tmpdate[0]==59&&l_tmpdate[1]==59)
		{
			Date[2]=BCD_Decimal(IIC_single_byte_read(addr[5]))/10+'0';
			Date[3]=BCD_Decimal(IIC_single_byte_read(addr[5]))%10+'0';
			Date[4]=' ';
			Date[5]=BCD_Decimal(IIC_single_byte_read(addr[4]))/10+'0';
			Date[6]=BCD_Decimal(IIC_single_byte_read(addr[4]))%10+'0';
			Date[7]=' ';
			Date[8]=BCD_Decimal(IIC_single_byte_read(addr[3]))/10+'0';
			Date[9]=BCD_Decimal(IIC_single_byte_read(addr[3]))%10+'0';
			Date[10]='\0';
		}
	}
}
void Set_RTC(void)
{
    unsigned char i,tmp;
    for(i=0;i<6;i++)
    {       //BCD处理
        tmp=l_tmpdate[i]/10;
        l_tmpdate[i]=l_tmpdate[i]%10;
        l_tmpdate[i]|=tmp<<4;
    } 
//    IIC_single_byte_write(0x0e,0X8c);
     for(i=0;i<6;i++)        //6次写入 秒分时日月年
     {
        IIC_single_byte_write(addr[i],l_tmpdate[i]);
     }
    IIC_single_byte_write(0x0e,0x0c);
}
void main()
{
	Init();
	Read_RTC();
	while(1)
	{	
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
			show(l_tmpdate[2]/10,l_tmpdate[2]%10,dot,l_tmpdate[1]/10,l_tmpdate[1]%10,dot,l_tmpdate[0]/10,l_tmpdate[0]%10);
		else
			show8x8(Rev);
		
		//show(1,2,3,4,5,6,7,8);
	}
}
void show(char a,char b,char c,char d,char e ,char f,char g,char h)
{
	unsigned char i,j;
	unsigned char duan;
	SDA=0;
	for (j=0;j<32;j++)
		{
			SCK_IN=0;
			SCK_OUT=0;
			switch(j/4)
			{
				case 0:duan=table[a][j%4];break;
				case 1:duan=table[b][j%4];break;
				case 2:duan=table[c][j%4];break;
				case 3:duan=table[d][j%4];break;
				case 4:duan=table[e][j%4];break;
				case 5:duan=table[f][j%4];break;
				case 6:duan=table[g][j%4];break;
				case 7:duan=table[h][j%4];break;
			}
			SCK_OUT_DUAN=0;
			SCK_IN=1;
			//if(duan)
			{
				for (i = 0; i < 8; ++i)
				{
					SDA_DUAN=duan&0x80;
					SCK_IN_DUAN=0;
					duan<<=1;
					SCK_IN_DUAN=1;
				}
				SCK_OUT=SCK_OUT_DUAN=1;
			}	
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
		if (pos<len*8)
		{
			character=*(p+pos/8);
			if (character==':'&&pos%8==0)
			{
				pos+=4;
			}
			if (character==' ')
			{
				character=16+'0';
				if(pos%8==0)
				{
					pos+=4;
				}
			}
			duan=IapReadByte((character-'0')*8+pos%8);
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
		SCK_OUT_DUAN=1;
		SCK_IN=1;
		SCK_OUT=1;
		SDA=1;
	}
	pos=NO;
	speed++;
	if (len>4 && speed==40)
	{
		speed=0;
		pos++;
	}
	if (*(p+pos/8)==' '&&pos%8==0)
	{
		pos+=4;
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
		Outchr(temp);
		if(temp!='\n')
		{
			Rev[i]=temp;
			if(i<19)
			{
				i++;
			}
		}
		else
		{
			if (Rev[0]=='t')
			{ 
				l_tmpdate[0]=(Rev[16]-'0')*10+Rev[17]-'0';
				l_tmpdate[1]=(Rev[13]-'0')*10+Rev[14]-'0';
				l_tmpdate[2]=(Rev[10]-'0')*10+Rev[11]-'0';
				l_tmpdate[3]=(Rev[7]-'0')*10+Rev[8]-'0';
				l_tmpdate[4]=(Rev[4]-'0')*10+Rev[5]-'0';
				l_tmpdate[5]=(Rev[1]-'0')*10+Rev[2]-'0';
				Set_RTC();
				Rev[0]='T';
			}
			Rev[i]='\0';
			if (Rev[0]=='s')
			{
				mode=!mode;
				Outstr(Rev);
			}
			
			i=0;
		}

	}
}
