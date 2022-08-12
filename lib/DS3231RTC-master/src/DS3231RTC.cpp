/****************************************************
* Arduino library for DS3231 RTC
*
* Author:   Pami
* Date:     2014-4-12
* Version:  1.0
*****************************************************/

#include <Arduino.h>
#include <Wire.h>
#include "DS3231RTC.h"

/************************************************
* Time class constructor
************************************************/
Time::Time() {

}

/************************************************
* Time class constructor
************************************************/
Time::Time(uint16_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t min, uint8_t sec, uint8_t day) {
	_year = year;
	_month = month;
	_date = date;
	_day = day;
	_hour = hour;
	_min = min;
	_sec = sec;
}

/************************************************
* BCD Code convert decimal
************************************************/
static uint8_t bcd2dec (uint8_t val) { return val - 6 * (val >> 4); }

/************************************************
* decimal convert BCD Code
************************************************/
static uint8_t dec2bcd (uint8_t val) { return val + 6 * (val / 10); }

/************************************************
* 
************************************************/
uint8_t DS3231RTC::readRegister(uint8_t add) {
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(add);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2C_ADDRESS, 1);
    return Wire.read();
}

/************************************************
* 
************************************************/
void DS3231RTC::writeRegister(uint8_t add,uint8_t val) {
    Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write(add);
    Wire.write(val);
    Wire.endTransmission();
}

/************************************************
* 
************************************************/
void DS3231RTC::init() {
    writeRegister(DS3231_CONTROL, 0x1C);    // 0b0001 1100
}

void DS3231RTC::enSQWOUT(uint8_t val, bool enBBSQW) {
    uint8_t control = readRegister(DS3231_CONTROL);
    control &= ~(1 << DS3231_C_INTCH);    // INTCH位 清0

    if(enBBSQW)
        control |= (1 << DS3231_C_BBSQW);   // BBSQW位 置1
    else
        control &= ~(1 << DS3231_C_BBSQW);    // BBSQW位 清0

    if(val == 0) {
        control &= ~(1 << DS3231_C_RS1);
        control &= ~(1 << DS3231_C_RS2);
    }else if(val == 1) {
        control |= (1 << DS3231_C_RS1);
        control &= ~(1 << DS3231_C_RS2);
    }else if(val == 4) {
        control &= ~(1 << DS3231_C_RS1);
        control |= (1 << DS3231_C_RS2);
    }else{
        control |= (1 << DS3231_C_RS1);
        control |= (1 << DS3231_C_RS2);
    }
    writeRegister(DS3231_CONTROL,control);
}

void DS3231RTC::enAlarm1Interrupt(uint8_t hour, uint8_t min){
    cleanAlarm1Flag();

    writeRegister(DS3231_A1_SEC,0x00);
    writeRegister(DS3231_A1_MIN,(dec2bcd(min) & 0b01111111));
    writeRegister(DS3231_A1_HOUR,(dec2bcd(hour) & 0b00111111));
    writeRegister(DS3231_A1_DYDT,0x80);

    //开启闹钟1中断
    uint8_t control = readRegister(DS3231_CONTROL);
    control |= (1 << DS3231_C_INTCH) | (1 << DS3231_C_A1IE);
    writeRegister(DS3231_CONTROL, control);
}

void DS3231RTC::enAlarm2Interrupt(uint8_t hour, uint8_t min){
    cleanAlarm2Flag();

    writeRegister(DS3231_A2_MIN,(dec2bcd(min) & 0b01111111));
    writeRegister(DS3231_A2_HOUR,(dec2bcd(hour) & 0b00111111));
    writeRegister(DS3231_A2_DYDT,0x80);

    uint8_t control = readRegister(DS3231_CONTROL);
    control |= (1 << DS3231_C_INTCH) | (1 << DS3231_C_A2IE);
    writeRegister(DS3231_CONTROL, control);
}

void DS3231RTC::cleanAlarm1Flag() {
    uint8_t status = readRegister(DS3231_STATUS);
    status &= ~(1 << DS3231_S_A1F);
    writeRegister(DS3231_STATUS, status);
}

void DS3231RTC::cleanAlarm2Flag() {
    uint8_t status = readRegister(DS3231_STATUS);
    status &= ~(1 << DS3231_S_A2F);
    writeRegister(DS3231_STATUS, status);
}

void DS3231RTC::setTime(Time time) {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write((uint8_t)DS3231_SEC);
    Wire.write(dec2bcd(time.second()));
    Wire.write(dec2bcd(time.minute()));
    Wire.write(dec2bcd(time.hour()));
    Wire.write(time.day());
    Wire.write(dec2bcd(time.date()));
    Wire.write(dec2bcd(time.month()));
    Wire.write(dec2bcd(time.year() - 2000));
    Wire.endTransmission();
}

Time DS3231RTC::getTime() {
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
    Wire.write((uint8_t)DS3231_SEC);
    Wire.endTransmission();

    Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
    uint8_t year, month, date, hour, min, sec, day;
    if(Wire.available() == 7){
    	sec = bcd2dec(Wire.read());
    	min = bcd2dec(Wire.read());
    	hour = bcd2dec(Wire.read());
    	day = bcd2dec(Wire.read());
    	date = bcd2dec(Wire.read());
    	month = bcd2dec(Wire.read());
    	year = bcd2dec(Wire.read());
    }
	return Time(year + 2000, month, date, hour, min, sec, day);
}

void DS3231RTC::waitConvertTemperature() {
    uint8_t ct = readRegister(DS3231_CONTROL);
    ct |= 0b00100000;
    writeRegister(DS3231_CONTROL, ct);

    labConvert:
        delay(50);
        ct = readRegister(DS3231_CONTROL);
        bool temp = (ct >> 5) & 1;
        if(temp)
            goto labConvert;
}


float DS3231RTC::getTemperature(bool force) {
    if(force)
        waitConvertTemperature();
    uint8_t th = readRegister(DS3231_TMP_MSB);
    uint8_t tl = readRegister(DS3231_TMP_LSB);
    float t = th + (tl >> 6) * 0.25;
    return t;
}
