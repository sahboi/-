// simple.ino
#include <Wire.h>
#include <stdio.h>
#include <DS3231RTC.h>

DS3231RTC rtc;
Time t;

void setup() {
	Wire.begin();
	Serial.begin(9600);

	t = rtc.getTime();
	// 如果小于2000年, 那么就设置本程序的开发日期
	if(t.year() == 2000){
		rtc.init();
		/* 参数格式: 年, 月, 日, 时, 分, 秒, 星期 */
		Time t(2014,4,12,11,0,0,6);
		rtc.setTime(t);
		Serial.println("setting date& time.");
	}
}

void loop() {

	// 声明字符串缓冲区
	char buf[20];			

	// 获取时间.
	t = rtc.getTime();
	
	// 格式化日期为指定的字符串格式
	snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", 
		t.year(), t.month(), t.date(), t.hour(), t.minute(), t.second());

	Serial.println(buf);

	// 获取温度
	Serial.print(rtc.getTemperature(false));	
	//Serial.println(" °C");
	Serial.println(" C");

	delay(1000);
}

	