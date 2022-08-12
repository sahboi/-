/****************************************************
* Arduino library for DS3231 RTC
*
* Author:   Pami
* Date:     2014-4-12
* Version:  1.0
*****************************************************/

#ifndef DS3231RTC_H
#define DS3231RTC_H

/****************************************
DS3231 寄存器地址
****************************************/
#define DS3231_I2C_ADDRESS		0x68
// 时钟和日历
#define DS3231_SEC			0x00
#define DS3231_MIN			0x01
#define DS3231_HOUR			0x02
#define DS3231_DAY			0x03
#define DS3231_DATE			0x04
#define DS3231_MONTH		0x05
#define DS3231_YEAR			0x06
// 闹钟1
#define DS3231_A1_SEC		0x07
#define DS3231_A1_MIN		0x08
#define DS3231_A1_HOUR		0x09
#define DS3231_A1_DYDT		0x0A
// 闹钟2
#define DS3231_A2_MIN		0x0B
#define DS3231_A2_HOUR		0x0C
#define DS3231_A2_DYDT		0x0D
// 控制寄存器
#define DS3231_CONTROL		0x0E
// 控制寄存器位标记
#define DS3231_C_A1IE		0
#define DS3231_C_A2IE		1
#define DS3231_C_INTCH		2
#define DS3231_C_RS1		3
#define DS3231_C_RS2		4
#define DS3231_C_CONV		5
#define DS3231_C_BBSQW		6
#define DS3231_C_EOSC		7
// 状态
#define DS3231_STATUS		0x0F
#define DS3231_S_A1F		0
#define DS3231_S_A2F		1
#define DS3231_S_BSY		2
#define DS3231_S_EN32Khz	3
#define DS3231_S_OSF		7
// 老化补偿
#define DS3231_AGING_OFFSET	0x10
// 温度 H
#define DS3231_TMP_MSB		0x11
// 温度 L
#define DS3231_TMP_LSB		0x12

class Time {
	public:
		Time();
		Time(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec, uint8_t day);
		uint16_t year() { return _year; }
		uint8_t month() { return _month; }
		uint8_t date() { return _date; }
		uint8_t hour() { return _hour; }
		uint8_t minute() { return _min; }
		uint8_t second() { return _sec; }
		uint8_t day() { return _day; }

	private:
		uint16_t _year;
		uint8_t _month, _date, _day, _hour, _min, _sec;
};


class DS3231RTC {
	public:
		void setTime(Time time);
		Time getTime();
		uint8_t readRegister(uint8_t add);
		void writeRegister(uint8_t add,uint8_t val);
		void init();
		/**
		* val 0 = 1Hz, 1 = 1.024kHz, 4 = 4.096 kHz, 8 = 8.192 kHz
		* enBBSQW 为 true 时 掉电也输出 SQW
		**/
		void enSQWOUT(uint8_t val, bool enBBSQW);

		/* 启用闹铃 1 中断 */
		void enAlarm1Interrupt(uint8_t hour, uint8_t min);

		/* 启用闹铃 2 中断 */
		void enAlarm2Interrupt(uint8_t hour, uint8_t min);

		/* 停用闹铃 1 中断 */
		void cleanAlarm1Flag();

		/* 停用闹铃 2 中断 */
		void cleanAlarm2Flag();

		/**
		* force 参数如果为 true ,代表强制转换温度, 大约 120ms时间, 为 false 直接从寄存器中获取
		* 
		**/
		float getTemperature(bool force);

	private:
		void waitConvertTemperature();
		
// 	static uint8_t bcd2dec (uint8_t val);
// 	static uint8_t dec2bcd (uint8_t val);
};

#endif
