/*
 * multivarka.c
 * Created: 31.05.2021 15:31:08
 * Author : Алексей Денисов 748
 */ 
#define F_CPU 8000000 // частота работы мк
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#define E1 PORTD|=0b00100000 // установка линии E в 1
#define E0 PORTD&=0b11011111 // установка линии E в 0
#define RS1 PORTD|=0b10000000 // установка линии RS в 1 (данные)
#define RS0 PORTD&=0b01111111 // установка линии RS в 0 (команда)
#define PORTTEMP PORTD // замены для портов термодатчика
#define DDRTEMP DDRD
#define PINTEMP PIND
#define BITTEMP 1
#define NOID 0xCC //Пропустить идентификацию
#define T_CONVERT  0x44 //Код измерения температуры
#define READ_DATA 0xBE //Передача байтов ведущему
char str[12];
unsigned int rezhim = 0, minutes = 0, seconds = 0;//выбор режима, минуты и секунды
unsigned int tt=0; //переменная для хранения температуры

//--- Функция передачи байта на дисплей ---
void sendbyte(unsigned char byte, unsigned char mode){
	if (mode == 0) RS0; // Если mode = 0, режим команды
	else RS1; // Если mode = 1, режим данных
	E1; // Е = 1
	_delay_us(50); // Задержка 50 мкс
	PORTA = 0;
	PORTA |= byte; // Копирование байта в порт для передачи
	E0; // E = 0
	_delay_us(50); // Задержка 50 мкс
}
//--- Функция вывода строк на дисплей ---
void str_lcd (char str1[])
{
	wchar_t n; // создание переменной для цикла
	for (n=0; str1[n]!='\0';n++) // цикл перебора символов
	sendbyte(str1[n], 1); // отправка символа
}
//--- выбор позиции на курсора на дисплее ---
void setpos(unsigned char x, unsigned y)
{
	char adress; // задание переменной адреса
	adress=(0x40*y+x)|0b10000000; // запись байта адреса
	sendbyte(adress, 0); // Отправка адреса
}
//--- Инициализация дисплея ---
void LCD_ini(void){
	_delay_ms(15); // Задержка 15 мс 
	sendbyte(0b00110000, 0); // отправка управляющего байта
	_delay_ms(5); // Задержка 5 мс 
	sendbyte(0b00110000, 0); // отправка управляющего байта
	_delay_us(120); // Задержка 120 мкс 
	sendbyte(0b00110000, 0); // отправка управляющего байта
	_delay_ms(1); // Задержка 1 мс
	sendbyte(0b00111100, 0); // Выбор режима работы 8 бит, 2
	_delay_ms(1); // Задержка 1 мс
	sendbyte(0b00001100, 0);// Включение дисплея, курсор отключен
	_delay_ms(1); // Задержка 1 мс
	sendbyte(0b00000001, 0); // Очистка дисплея
	_delay_ms(2); // Задержка 2 мс
 } //	Задержки по тех.документации
//——————————————–
//функция определения датчика на шине
char dt_testdevice(void) //dt - digital termomether | определим, есть ли устройство на шине
{
	char stektemp=SREG;// сохраним значение стека
	cli(); //запрещаем прерывание
	char dt;
	DDRTEMP |= 1<<BITTEMP; //притягиваем шину
	_delay_us(485); //задержка как минимум на 480 микросекунд
	DDRTEMP &= ~(1<<BITTEMP); //отпускаем шину
	_delay_us(65); //задержка как максимум на 60 микросекунд
	if ((PINTEMP & (1<<BITTEMP))==0)//проверяем, ответит ли устройство
	{
		dt=1;//устройство есть
	}
	else dt=0;//устройства нет
	SREG = stektemp;// вернем значение стека
	_delay_us(420); //задержка 420, тк это с учетом времени прошедших команд
	return dt; //вернем результат
}
//функция записи бита на устройство
void dt_sendbit(char bt)
{
	char stektemp=SREG;// сохраним значение стека
	cli(); //запрещаем прерывание
	DDRTEMP |= 1<<BITTEMP; //притягиваем шину
	_delay_us(2); //задержка как минимум на 2 микросекунды
	if(bt)
	DDRTEMP &= ~(1<<BITTEMP); //отпускаем шину
	_delay_us(65); //задержка как минимум на 60 микросекунд
	DDRTEMP &= ~(1<<BITTEMP); //отпускаем шину
	SREG = stektemp;// вернем значение стека
}
//функция записи байта на устройство
void dt_sendbyte(unsigned char bt)
{
	char i;
	for(i=0;i<8;i++)//посылаем отдельно каждый бит на устройство
	{
		if((bt & (1<<i)) == 1<<i)//посылаем 1
		dt_sendbit(1);
		else //посылаем 0
		dt_sendbit(0);
	}
}
//функция чтения бита с устройства
char dt_readbit(void)
{
	char stektemp=SREG;// сохраним значение стека
	cli(); //запрещаем прерывание
	char bt; //переменная хранения бита
	DDRTEMP |= 1<<BITTEMP; //притягиваем шину
	_delay_us(2); //задержка как минимум на 2 микросекунды
	DDRTEMP &= ~(1<<BITTEMP); //отпускаем шину
	_delay_us(13);
	bt = (PINTEMP & (1<<BITTEMP))>>BITTEMP; //читаем бит
	_delay_us(45);
	SREG = stektemp;// вернем значение стека
	return bt; //вернем результат
}
//функция чтения байта с устройства
unsigned char dt_readbyte(void)
{
	char c=0;
	char i;
	for(i=0;i<8;i++)
	c|=dt_readbit()<<i; //читаем бит
	return c;
}
//функция преобразования показаний датчика в температуру
int dt_check(void)
{
	unsigned char bt;//переменная для считывания байта
	unsigned int tt=0;
	if(dt_testdevice()==1) //если устройство нашлось
	{
		dt_sendbyte(NOID); //пропустить идентификацию, тк только одно устройство на шине
		dt_sendbyte(T_CONVERT); //измеряем температуру
		_delay_ms(750); //в 12битном режиме преобразования - 750 милисекунд
		dt_testdevice(); //проверка ее присутствия
		dt_sendbyte(NOID); //пропустить идентификацию
		dt_sendbyte(READ_DATA); //команда на чтение данных с устройства
		bt = dt_readbyte(); //читаем младший бит
		tt = dt_readbyte(); //читаем старший бит MS
		tt = (tt<<8)|bt;//старший влево, младший на его место, получаем общий результат
	}
	return tt;
}
//преобразование температуры в единицы
char converttemp (unsigned int tt)
{
	char t = tt>>4;//сдвиг и отсечение части старшего байта
	return t;
}
//——————————————–
//--- Выбор режима готовки ---
void choose_mode(void){
	setpos(0,0); // выбор позиции курсора
	str_lcd("Varka");
	setpos(0,1); // выбор позиции курсора
	str_lcd("Jarka");
	setpos(8,0); // выбор позиции курсора
	str_lcd("Tushenie");
	setpos(8,1); // выбор позиции курсора
	str_lcd("Razogrev");
	while(rezhim == 0){ // выбор режима при нажатии соответствующей кнопки
		if (!(PINB&0b01000000)) rezhim = 1;
		else if (!(PINB&0b00100000)) rezhim = 2;
		else if (!(PINB&0b00010000)) rezhim = 3;
		else if (!(PINB&0b00001000)) rezhim = 4;
	}
}
//--- ПРОЦЕСС ВАРКИ ---
void varka(void){
	sendbyte(0b00000001, 0); // Очистка дисплея
	_delay_ms(2); // Задержка 2 мс
	PORTC = 0b00000101;  // Включение основного и антиконденсат ТЭНов
minutes = 50; // время готовки
	while(1){			
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2); // Задержка 2 мс		
		str_lcd("ostalos vremeni:");
		setpos(0, 1); // выбор позиции курсора
		sprintf(str, "%d", minutes);
		str_lcd(str);//вывод минут на экран
		seconds += 1; // инкремент секунд
		if(seconds==60){
			minutes -= 1; //декремент минут
			seconds = 0;// обнуление секундного счетчика
		}
		if(minutes <= 0) break;//выход (Приготовление завершено)
		tt = converttemp(dt_check()); //измеряем температуру
		if (tt >= 120) PORTC = 0b00000100;// Отключение основного ТЭН
		if (PINC == 0b00000100){
			if(tt<115) PORTC = 0b00000101;// Включение основного и антиконденсат ТЭНов
		}
	}
}
//--- ПРОЦЕСС ЖАРКИ ---
void jarka(void){
	sendbyte(0b00000001, 0); // Очистка дисплея
	_delay_ms(2); // Задержка 2 мс
	PORTC = 0b00000101;  // Включение основного и антиконденсат ТЭНов
	minutes = 25;
	while(1){		
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2); // Задержка 2 мс
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//вывод минут на экран
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//выход (Приготовление завершено)
		tt = converttemp(dt_check()); //измеряем температуру
		if (tt >= 125) PORTC = 0b00000100;// Отключение основного ТЭН
		if (PINC == 0b00000100){
			if(tt<115) PORTC = 0b00000101;// Включение основного и антиконденсат ТЭНов
		}
	}
}
//--- ПРОЦЕСС ТУШЕНИЯ ---
void tushenie(void){
	sendbyte(0b00000001, 0); // Очистка дисплея
	_delay_ms(2); // Задержка 2 мс
	PORTC = 0b00000101;  // Включение основного и антиконденсат ТЭНов
minutes = 25;
	while(1){		
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2); // Задержка 2 мс
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//вывод минут на экран
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//выход (Приготовление завершено)
		tt = converttemp(dt_check()); //измеряем температуру
		if (tt >= 110) PORTC = 0b00000100;// Отключение основного ТЭН
		if (PINC == 0b00000100){
			if(tt<105) PORTC = 0b00000101;// Включение основного и антиконденсат ТЭНов
		}
	}
	minutes = 25;
while(1){ // Процесс томления при пониженных температурах
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2); // Задержка 2 мс
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//вывод минут на экран
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//выход (Приготовление завершено)
		tt = converttemp(dt_check()); //измеряем температуру
		if (tt >= 96) PORTC = 0b00000100;// Отключение основного ТЭН
		if (PINC == 0b00000100){
			if(tt<70) PORTC = 0b00000101;// Включение основного и антиконденсат ТЭНов
		}
	}

}
//--- ПРОЦЕСС РАЗОГРЕВА ---
void razogrev(void){
	sendbyte(0b00000001, 0); // Очистка дисплея
	_delay_ms(2); // Задержка 2 мс
	PORTC = 0b00000101;  // Включение основного и антиконденсат ТЭНов
	minutes = 10;
	while(1){
		str_lcd("ostalos vremeni:");
		setpos(0,1);
		sprintf(str, "%d", minutes);
		str_lcd(str);//вывод минут на экран
		seconds += 1;
		if(seconds==60){
			minutes -= 1;
			seconds = 0;
		}
		if(minutes <= 0) break;//выход (Приготовление завершено)
		tt = converttemp(dt_check()); //измеряем температуру
		if (tt >= 70) PORTC = 0b00000100;// Отключение основного ТЭН
		if (PINC == 0b00000100){
			if(tt<50) PORTC = 0b00000101; // Включение основного и антиконденсат ТЭНов
		}
	}
}

int main(void)  // основная функция
{	
	while(1){
		DDRA = 0x00; //выбор режима работы (на вход/выход) портов
		DDRB = 0b00000001;
		DDRC = 0b00111111;
		DDRD = 0b11100000;		
		PORTA = 0x00; // подача напряжения на выход или
		PORTB = 0b01111000; // подключение подтягивающего резистора
		PORTC = 0x00;
		PORTD = 0x00;
		LCD_ini(); // инициализация дисплея
		str_lcd("Viberite rezhim");
		setpos(0, 1); // выбор позиции курсора
		str_lcd("Prigotovlenia");
		_delay_ms(5000);// Задержка 5 с 
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2);// Задержка 2 мс
		choose_mode(); // функция выбора режима работы
		if (rezhim == 1) varka(); // нажата первая кнопка
		if (rezhim == 2) jarka(); // нажата вторая кнопка
		if (rezhim == 3) tushenie(); // нажата третья кнопка
		if (rezhim == 4) razogrev(); // нажата четвертая кнопка
		PORTB |= 0b00000001;
		_delay_ms(2500);// Задержка 2,5 с (длительность звука)
		PORTB &= 0b11111110;
		sendbyte(0b00000001, 0); // Очистка дисплея
		_delay_ms(2); // Задержка 2 мс
		setpos(0,0); // выбор позиции курсора		
		str_lcd("Bludo Gotovo!");
		PORTC = 0b00000010; // Включение подогревающего ТЭН
		while(1){
			if(!(PINB&0b00001000)) break;//	ожидание нажатия кнопки ОК	
		}
	}	
}	