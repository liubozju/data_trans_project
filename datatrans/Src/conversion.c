#include "conversion.h"
#include "message.h"
#include "log.h"
#include "upgrade.h"

//ascii型字符转化为HEX值
/*实例：  '64'--0x64   只可以转换大写A-F*/
void StrToHex(char* str,uint8_t * hex,int strlen)
{
	int i = 0;
	int j = 0;
	uint8_t s1,s2;
	for (j = 0; j < strlen/2; j++)
	{
//		s1 = str[i++] - '0'; 
//		s2 = str[i++] - '0';
		switch (str[i++])
		{
			case '0':s1 = 0;break;
			case '1':s1 = 1; break;
			case '2':s1 = 2; break;
			case '3':s1 = 3; break;
			case '4':s1 = 4; break;
			case '5':s1 = 5; break;
			case '6':s1 = 6; break;
			case '7':s1 = 7; break;
			case '8':s1 = 8; break;
			case '9':s1 = 9; break;
			case 'A':s1 = 10; break;
			case 'B':s1 = 11; break; 
			case 'C':s1 = 12; break;
			case 'D':s1 = 13; break;
			case 'E':s1 = 14; break;
			case 'F':s1 = 15; break;
		}
		switch (str[i++])
		{
			case '0':s2 = 0;break;
			case '1':s2 = 1; break;
			case '2':s2 = 2; break;
			case '3':s2 = 3; break;
			case '4':s2 = 4; break;
			case '5':s2 = 5; break;
			case '6':s2 = 6; break;
			case '7':s2 = 7; break;
			case '8':s2 = 8; break;
			case '9':s2 = 9; break;
			case 'A':s2 = 10; break;
			case 'B':s2 = 11; break; 
			case 'C':s2 = 12; break;
			case 'D':s2 = 13; break;
			case 'E':s2 = 14; break;
			case 'F':s2 = 15; break;
		}
		hex[j] = s1*16+s2;
	}
}

uint8_t gPackCheck(char * str,uint16_t strlen)
{
	int ret = -1;
	uint8_t * temp_str =pvPortMalloc(150); 
	memset(temp_str,0,150);
	StrToHex(str+2,temp_str,strlen-2);
	uint8_t checksum = 0;
	uint8_t arr_len = temp_str[0];
	for(int i = 1;i<arr_len;i++)
	{
		checksum += temp_str[i];
	}
	if(checksum == temp_str[arr_len])
		return PackOK;
	else
		return PackFail;
}


//HEX值转为ASCII型字符
/*实例：  0x64--"64"*/
void HexToStr(uint8_t* hex,char * str,int hexlen)
{
	int i = 0;
	int j = 0;
	for (i = 0; i < hexlen; i++)
	{
		str[j++] = hex[i] / 16 + '0';
		str[j++] = hex[i] % 16 + '0';	
	}
	for (j = 0; j < 2*hexlen; j++)
	{
		switch (str[j])
		{
			case ':':str[j] = 'A'; break;
			case ';':str[j] = 'B'; break;
			case '<':str[j] = 'C'; break;
			case '=':str[j] = 'D'; break;
			case '>':str[j] = 'E'; break;
			case '?':str[j] = 'F'; break;
			default:break;
		}
	}
	str[2*hexlen] = 0;  //结束符
}

//DEC值转为ASCII型字符
void DecToStr(uint32_t dec,char * str)
{
	char a[10] = {0};
	uint8_t cnt,i=0;
	
	for(cnt=10;cnt>0;cnt--)
	{
		a[cnt-1] = dec%10+'0';
		dec = dec/10;
		if(dec == 0)
		{
			break;
		}
	}
	for(i=0;i<11-cnt;i++)
	{
		str[i] = a[cnt+i-1];
	}
	str[i] = 0;
}

//Float值转为ASCII型字符 最大值<1000 3位小数
void FloatToStr(float f,char * str)
{
	uint8_t cnt;
	char a[8] = {0};
	uint32_t d;
	if(f > 1000)
	{
		printf("This float is out of range!\r\n");
		return;
	}
	d=(uint32_t)(f*1000);
	for(cnt = 7;cnt>0;cnt--)
	{
		if(cnt == 4)
		{
			a[cnt-1]='.';
		}
		else
		{
			a[cnt-1]=d%10+'0';
			d=d/10;
		}
	}
	for(cnt=0;cnt<3;cnt++)
	{
		if(a[cnt] > '0')
		{
			break;
		}
	}
	if(cnt == 3)
	{
		cnt = 2;
	}
	memcpy(str,&a[cnt],7-cnt);
	str[7-cnt]=0;
}


uint16_t StrToDec(char * str,uint8_t len)
{
	uint16_t retVal=0;
	for(int i =0 ;i < len ;i++)
	{
			retVal = retVal *10 + *str -'0';
			str ++;
	}
	return retVal;
}

//检测有无目标字符串     比strstr好在可以指定长度做检测，读到0x00不会停止向后读取
char* FindStr(char* buf,uint16_t buflen,char* aim)
{
	uint16_t i,j;
	uint16_t aimlen;
	
	aimlen=strlen(aim);
	if(buflen < aimlen)
	{
		return NULL;
	}
	for(i=0;i<=(buflen-aimlen);i++)
	{
		for(j=0;j<aimlen;j++)
		{
			if(buf[i+j] != aim[j])
			{
				break;
			}
		}
		if(j == aimlen)
		{
			return &buf[i];//找到目标地址
		}
	}
	return NULL;
}

