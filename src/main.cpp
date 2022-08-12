#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include <FS.h>
#include <WiFiManager.h>

#include <time.h>
#include <sys/time.h>
// #include <coredecls.h>

#include <DS3231RTC.h>

#include "DEV_Config.h"//引脚配置文件
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include <stdlib.h>

// #define CS 5     //墨水屏引脚定义
// #define RST 17
// #define DC 19
// #define BUSY 16
// #define CLK 18
// #define DIN 23

//时间显示相关定义
#define TZ              8      // 中国时区为8
#define DST_MN          0      // 默认为0
#define TZ_MN           ((TZ)*60)   //时间换算
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

WiFiManager wm;//wifi 实例化
DS3231RTC rtc;//RTC实例化
Time t;//实例化时间

bool res;//判断WiFi是否连接

UBYTE *BlackImage, *RYImage; // Red or Yellow 创建墨水屏图像指针

//墨水屏初始化
void Elnksetup(){
  DEV_Module_Init();//引脚定义初始化

  printf("e-Paper Init and Clear...\r\n");
  EPD_2IN13BC_Init();//屏幕型号初始化
  EPD_2IN13BC_Clear();//清屏
  DEV_Delay_ms(500);

  //Create a new image cache named IMAGE_BW and fill it with white
  // UBYTE *BlackImage, *RYImage; // Red or Yellow
  UWORD Imagesize = ((EPD_2IN13BC_WIDTH % 8 == 0) ? (EPD_2IN13BC_WIDTH / 8 ) : (EPD_2IN13BC_WIDTH / 8 + 1)) * EPD_2IN13BC_HEIGHT;
  if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.println("Failed to apply for black memory...\r\n");
    while (1);
  }
  if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.println("Failed to apply for red memory...\r\n");
    while (1);
  }
  Paint_NewImage(BlackImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);
  Paint_NewImage(RYImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);//页面初始化参数

  //清屏
  // Paint_SelectImage(BlackImage);
  // Paint_Clear(WHITE);
  // Paint_SelectImage(RYImage);
  // Paint_Clear(WHITE);
  // delay(500);

  //黑色页
  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(15,15,"Star Plan",&Font24,WHITE,BLACK);
  Paint_DrawString_CN(15,35,"繁星计划",&Font24CN,WHITE,BLACK);

  //红色页
  const char chinese[25] = "前方还有星辰大海";
  Paint_SelectImage(RYImage);
  Paint_Clear(WHITE);
  Paint_DrawChar(5,78,chinese[25],&Font12,WHITE,RED);
  // Paint_DrawString_CN(5,78,"前方还有星辰大海",&Font12CN,WHITE,RED);
  
  Serial.println("Dispiay..");
  EPD_2IN13BC_Display(BlackImage,RYImage);
  delay(2000);

  Serial.println("Goto Sleep...\r\n");
  EPD_2IN13BC_Sleep();//关闭墨水屏
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;
}
//连接WiFi 初始化
void Wifisetup(){
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalTimeout(180);//180秒内未连接则强制返回状态
  wm.setBreakAfterConfig(true);
  // res = wm.autoConnect(); //自动生成WiFi名称
  // res = wm.autoConnect("AutoConnectAP"); // 只有WiFi名称不需要密码
  res = wm.autoConnect("高考加油！！！","123456789"); //ap模式WiFi名称和密码
    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
        WiFi.mode(WIFI_OFF);//未连接则关闭WiFi
        Serial.println("WiFi已关闭");
    } 
    else {    
        Serial.println("connected...yeey :)");
    }
    // configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com");//获取当前时间
    // struct tm*  timeinfo;
    // if(getLocalTime(timeinfo)){
    //   Serial.println("获得网络时间"); 
    //   Serial.println(asctime(timeinfo));
    //   Time t(timeinfo->tm_year + 1900,timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_wday); 
    // }
  
}

//RTC初始化   一定要放在WiFi初始化之后
void RTC_Setup(){
  Wire.begin(26,27);//引脚初始化 SDA/SCL
  char buf[20];
  t = rtc.getTime();
  Serial.println("获取DS3231时间");
  snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", 
		t.year(), t.month(), t.date(), t.hour(), t.minute(), t.second());

	Serial.println(buf);
}

//RTC_timeupdate DSD3231时间更新
bool RTC_timeupdate(){
  if(!res){
    return false;
  }
  else{
    configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com");//获取当前时间
    struct tm*  timeinfo;
    if(getLocalTime(timeinfo)){
      Serial.println("获得网络时间");
      Serial.println(asctime(timeinfo));
      rtc.init();
      Time t(timeinfo->tm_year + 1900,timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_wday);
      rtc.setTime(t);
      Serial.println("setting RTC date& time.");
      return true;
    }
    return false;
  }
  t = rtc.getTime();
}



//拨动按键
// int Switch(){

// }

// 触摸按键1
bool Buttontouch_1(){
  int i;
  i = touchRead(T8);//触摸脚引33
  if(i <= 40){//触摸阈值由后期读数为准，不一定为40
    delay(2);//延时防抖，具体值还需测试
    return true;
  }  
  else{
    return false;
  }
}

// 触摸按键2
bool Buttontouch_2(){
  int i;
  i = touchRead(T9);//触摸引脚32
  if(i <= 40){
    delay(2);
    return true;
  }  
  else{
    return false;
  }
}
//按键检测
void Swishtest(){
  if(Buttontouch_1())
    Serial.print("Buttontouch_1 按下");
  if(Buttontouch_2())
    Serial.print("Buttontouch_2 按下");
}

//两个日期之差开始日期和截止日期
int Date_difference(int y1,int m1,int d1,int y2,int m2,int d2)
{
	int day = 0,t1,t2;//t1,t2分别表示起始与截止的月份
	int mday[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };
	for (int i = y1; i <= y2; i++)
	{
		if (i == y1) t1 = m1;//如果是起始年，让起始月份等于输入月份
		else t1 = 1;
		if (i == y2) t2 = m2;//如果是截止年，让截止月份终于输入月份
		else t2 = 12;
		for (int k = t1; k <= t2; k++)
		{
			if (i % 400 == 0 || (i % 100 != 0 && i % 4 == 0))//判断是否是闰年
				mday[2] = 29;
			else mday[2] = 28;
			if (m1 == m2 && y1 == y2)//如果同年同月
				day += d2 - d1;
			else
			{
				if (i == y1 && k == m1)//如果是起始年月，让起始的日期等于输入
					day += mday[k] - d1;
				else if (i == y2 && k == m2)
					day += d2;
				else day += mday[k];
			}
		}
	}
	return day;
}

void setup() {
  Serial.begin(9600);
  pinMode(34,INPUT_PULLUP);
  pinMode(36,INPUT_PULLUP);
  pinMode(39,INPUT_PULLUP);//输入引脚
  Elnksetup();
  // Wifisetup();
  // RTC_Setup();
  // if(RTC_timeupdate()){
  //   char buf[20];
  //   snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d", 
	// 	t.year(), t.month(), t.date(), t.hour(), t.minute(), t.second());

	//   Serial.println(buf);
  // }
  delay(1000);
}

void loop() {
  Serial.println("hello world!");
  delay(1000);
}
