/*
 * multivarka.c
 * Created: 31.05.2021 15:31:08
 * Author : ������� ������� 748
 */ 
#define F_CPU 8000000 // ������� ������ ��
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#define E1 PORTD|=0b00100000 // ��������� ����� E � 1
#define E0 PORTD&=0b11011111 // ��������� ����� E � 0
#define RS1 PORTD|=0b10000000 // ��������� ����� RS � 1 (������)
#define RS0 PORTD&=0b01111111 // ��������� ����� RS � 0 (�������)
#define PORTTEMP PORTD // ������ ��� ������ ������������
#define DDRTEMP DDRD
#define PINTEMP PIND
#define BITTEMP 1
#define NOID 0xCC //���������� �������������
#define T_CONVERT  0x44 //��� ��������� �����������
#define READ_DATA 0xBE //�������� ������ ��������
char str[12];
unsigned int rezhim = 0, minutes = 0, seconds = 0;//����� ������, ������ � �������
unsigned int tt=0; //���������� ��� �������� �����������

//--- ������� �������� ����� �� ������� ---
void sendbyte(unsigned char byte, unsigned char mode){
	if (mode == 0) RS0; // ���� mode = 0, ����� �������
	else RS1; // ���� mode = 1, ����� ������
	E1; // � = 1
	_delay_us(50); // �������� 50 ���
	PORTA = 0;
	PORTA |= byte; // ����������� ����� � ���� ��� ��������
	E0; // E = 0
	_delay_us(50); // �������� 50 ���
}
//--- ������� ������ ����� �� ������� ---
void str_lcd (char str1[])
{
	wchar_t n; // �������� ���������� ��� �����
	for (n=0; str1[n]!='\0';n++) // ���� �������� ��������
	sendbyte(str1[n], 1); // �������� �������
}
//--- ����� ������� �� ������� �� ������� ---
void setpos(unsigned char x, unsigned y)
{
	char adress; // ������� ���������� ������
	adress=(0x40*y+x)|0b10000000; // ������ ����� ������
	sendbyte(adress, 0); // �������� ������
}
//--- ������������� ������� ---
void LCD_ini(void){
	_delay_ms(15); // �������� 15 �� 
	sendbyte(0b00110000, 0); // �������� ������������ �����
	_delay_ms(5); // �������� 5 �� 
	sendbyte(0b00110000, 0); // �������� ������������ �����
	_delay_us(120); // �������� 120 ��� 
	sendbyte(0b00110000, 0); // �������� ������������ �����
	_delay_ms(1); // �������� 1 ��
	sendbyte(0b00111100, 0); // ����� ������ ������ 8 ���, 2
	_delay_ms(1); // �������� 1 ��
	sendbyte(0b00001100, 0);// ��������� �������, ������ ��������
	_delay_ms(1); // �������� 1 ��
	sendbyte(0b00000001, 0); // ������� �������
	_delay_ms(2); // �������� 2 ��
 } //	�������� �� ���.������������
//���������������
//������� ����������� ������� �� ����
char dt_testdevice(void) //dt - digital termomether | ���������, ���� �� ���������� �� ����
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	char dt;
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(485); //�������� ��� ������� �� 480 �����������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(65); //�������� ��� �������� �� 60 �����������
	if ((PINTEMP & (1<<BITTEMP))==0)//���������, ������� �� ����������
	{
		dt=1;//���������� ����
	}
	else dt=0;//���������� ���
	SREG = stektemp;// ������ �������� �����
	_delay_us(420); //�������� 420, �� ��� � ������ ������� ��������� ������
	return dt; //������ ���������
}
//������� ������ ���� �� ����������
void dt_sendbit(char bt)
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(2); //�������� ��� ������� �� 2 ������������
	if(bt)
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(65); //�������� ��� ������� �� 60 �����������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	SREG = stektemp;// ������ �������� �����
}
//������� ������ ����� �� ����������
void dt_sendbyte(unsigned char bt)
{
	char i;
	for(i=0;i<8;i++)//�������� �������� ������ ��� �� ����������
	{
		if((bt & (1<<i)) == 1<<i)//�������� 1
		dt_sendbit(1);
		else //�������� 0
		dt_sendbit(0);
	}
}
//������� ������ ���� � ����������
char dt_readbit(void)
{
	char stektemp=SREG;// �������� �������� �����
	cli(); //��������� ����������
	char bt; //���������� �������� ����
	DDRTEMP |= 1<<BITTEMP; //����������� ����
	_delay_us(2); //�������� ��� ������� �� 2 ������������
	DDRTEMP &= ~(1<<BITTEMP); //��������� ����
	_delay_us(13);
	bt = (PINTEMP & (1<<BITTEMP))>>BITTEMP; //������ ���
	_delay_us(45);
	SREG = stektemp;// ������ �������� �����
	return bt; //������ ���������
}
//������� ������ ����� � ����������
unsigned char dt_readbyte(void)
{
	char c=0;
	char i;
	for(i=0;i<8;i++)
	c|=dt_readbit()<<i; //������ ���
	return c;
}
//������� �������������� ��������� ������� � �����������
int dt_check(void)
{
	unsigned char bt;//���������� ��� ���������� �����
	unsigned int tt=0;
	if(dt_testdevice()==1) //���� ���������� �������
	{
		dt_sendbyte(NOID); //���������� �������������, �� ������ ���� ���������� �� ����
		dt_sendbyte(T_CONVERT); //�������� �����������
		_delay_ms(750); //� 12������ ������ �������������� - 750 ����������
		dt_testdevice(); //�������� �� �����������
		dt_sendbyte(NOID); //���������� �������������
		dt_sendbyte(READ_DATA); //������� �� ������ ������ � ����������
		bt = dt_readbyte(); //������ ������� ���
		tt = dt_readbyte(); //������ ������� ��� MS
		tt = (tt<<8)|bt;//������� �����, ������� �� ��� �����, �������� ����� ���������
	}
	return tt;
}
//�������������� ����������� � �������
char converttemp (unsigned int tt)
{
	char t = tt>>4;//����� � ��������� ����� �������� �����
	return t;
}
//���������������
//--- ����� ������ ������� ---
void choose_mode(void){
	setpos(0,0); // ����� ������� �������
	str_lcd("Varka");
	setpos(0,1); // ����� ������� �������
	str_lcd("Jarka");
	setpos(8,0); // ����� ������� �������
	str_lcd("Tushenie");
	setpos(8,1); // ����� ������� �������
	str_lcd("Razogrev");
	while(rezhim == 0){ // ����� ������ ��� ������� ��������������� ������
		if (!(PINB&0b01000000)) rezhim = 1;
		else if (!(PINB&0b00100000)) rezhim = 2;
		else if (!(PINB&0b00010000)) rezhim = 3;
		else if (!(PINB&0b00001000)) rezhim = 4;
	}
}
//--- ������� ����� ---
void varka(void){
	sendbyte(0b00000001, 0); // ������� �������
	_delay_ms(2); // �������� 2 ��
	PORTC = 0b00000101;  // ��������� ��������� � ������������� �����
minutes = 50; // ����� �������
	while(1){			
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2); // �������� 2 ��		
		str_lcd("ostalos vremeni:");
		setpos(0, 1); // ����� ������� �������
		sprintf(str, "%d", minutes);
		str_lcd(str);//����� ����� �� �����
		seconds += 1; // ��������� ������
		if(seconds==60){
			minutes -= 1; //��������� �����
			seconds = 0;// ��������� ���������� ��������
		}
		if(minutes <= 0) break;//����� (������������� ���������)
		tt = converttemp(dt_check()); //�������� �����������
		if (tt >= 120) PORTC = 0b00000100;// ���������� ��������� ���
		if (PINC == 0b00000100){
			if(tt<115) PORTC = 0b00000101;// ��������� ��������� � ������������� �����
		}
	}
}
//--- ������� ����� ---
void jarka(void){
	sendbyte(0b00000001, 0); // ������� �������
	_delay_ms(2); // �������� 2 ��
	PORTC = 0b00000101;  // ��������� ��������� � ������������� �����
	minutes = 25;
	while(1){		
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2); // �������� 2 ��
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//����� ����� �� �����
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//����� (������������� ���������)
		tt = converttemp(dt_check()); //�������� �����������
		if (tt >= 125) PORTC = 0b00000100;// ���������� ��������� ���
		if (PINC == 0b00000100){
			if(tt<115) PORTC = 0b00000101;// ��������� ��������� � ������������� �����
		}
	}
}
//--- ������� ������� ---
void tushenie(void){
	sendbyte(0b00000001, 0); // ������� �������
	_delay_ms(2); // �������� 2 ��
	PORTC = 0b00000101;  // ��������� ��������� � ������������� �����
minutes = 25;
	while(1){		
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2); // �������� 2 ��
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//����� ����� �� �����
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//����� (������������� ���������)
		tt = converttemp(dt_check()); //�������� �����������
		if (tt >= 110) PORTC = 0b00000100;// ���������� ��������� ���
		if (PINC == 0b00000100){
			if(tt<105) PORTC = 0b00000101;// ��������� ��������� � ������������� �����
		}
	}
	minutes = 25;
while(1){ // ������� �������� ��� ���������� ������������
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2); // �������� 2 ��
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//����� ����� �� �����
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//����� (������������� ���������)
		tt = converttemp(dt_check()); //�������� �����������
		if (tt >= 96) PORTC = 0b00000100;// ���������� ��������� ���
		if (PINC == 0b00000100){
			if(tt<70) PORTC = 0b00000101;// ��������� ��������� � ������������� �����
		}
	}

}
//--- ������� ��������� ---
void razogrev(void){
	sendbyte(0b00000001, 0); // ������� �������
	_delay_ms(2); // �������� 2 ��
	PORTC = 0b00000101;  // ��������� ��������� � ������������� �����
	minutes = 10;
	while(1){
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//����� ����� �� �����
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//����� (������������� ���������)
		tt = converttemp(dt_check()); //�������� �����������
		if (tt >= 70) PORTC = 0b00000100;// ���������� ��������� ���
		if (PINC == 0b00000100){
			if(tt<50) PORTC = 0b00000101; // ��������� ��������� � ������������� �����
		}
	}
}

int main(void)  // �������� �������
{	
	while(1){
		DDRA = 0x00; //����� ������ ������ (�� ����/�����) ������
		DDRB = 0b00000001;
		DDRC = 0b00111111;
		DDRD = 0b11100000;		
		PORTA = 0x00; // ������ ���������� �� ����� ���
		PORTB = 0b01111000; // ����������� �������������� ���������
		PORTC = 0x00;
		PORTD = 0x00;
		LCD_ini(); // ������������� �������
		str_lcd("Viberite rezhim");
		setpos(0, 1); // ����� ������� �������
		str_lcd("Prigotovlenia");
		_delay_ms(5000);// �������� 5 � 
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2);// �������� 2 ��
		choose_mode(); // ������� ������ ������ ������
		if (rezhim == 1) varka(); // ������ ������ ������
		if (rezhim == 2) jarka(); // ������ ������ ������
		if (rezhim == 3) tushenie(); // ������ ������ ������
		if (rezhim == 4) razogrev(); // ������ ��������� ������
		PORTB |= 0b00000001;
		_delay_ms(2500);// �������� 2,5 � (������������ �����)
		PORTB &= 0b11111110;
		sendbyte(0b00000001, 0); // ������� �������
		_delay_ms(2); // �������� 2 ��
		setpos(0,0); // ����� ������� �������		
		str_lcd("Bludo Gotovo!");
		PORTC = 0b00000010; // ��������� �������������� ���
		while(1){
			if(!(PINB&0b00001000)) break;//	�������� ������� ������ ��	
		}
	}	
}	