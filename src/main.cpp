#include <Arduino.h>
#include <Wire.h>

#include <FS.h>
#include <WiFiManager.h>

#include <time.h>
#include <sys/time.h>
// #include <coredecls.h>

#include <DS3231RTC.h>

#include "driver/adc.h"//ADC文件

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
#define STUDYHOUR 22//休息时间
#define YEAR1 2023 //高考年份
#define MONTH1 6 //高考月
#define DAY1 7 //高考天

#define uS_TO_S_FACTOR 1000000  
#define TIME_TO_SLEEP  60 //深度睡眠唤醒时间sec
#define STANDTIME 3 //待机时间min

//时间显示相关定义
#define TZ              8      // 中国时区为8
#define DST_MN          0      // 默认为0
#define TZ_MN           ((TZ)*60)   //时间换算
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)
#define NTP1 "ntp1.aliyun.com"
#define NTP2 "ntp2.aliyun.com"
#define NTP3 "ntp3.aliyun.com"

WiFiManager wm;//wifi 实例化
DS3231RTC rtc;//RTC实例化
Time t;//实例化时间

bool res = false;//判断WiFi是否连接
int waitTime = 0;//待机计数
RTC_DATA_ATTR bool firstwifiElinkconfit = true;//第一次WiFi和Elnk配置标志
RTC_DATA_ATTR int Modestate = 1;//模式选择状态默认为1
RTC_DATA_ATTR bool drawpage = true;//绘制页面标记

//函数声明
void Elnksetup();//墨水屏初始化
void Wifisetup();//wifi初始化
void RTC_Setup();//RTC初始化
void RTC_timeupdate();//RTC时间更新
int Switch(); //按键检测
void shake(int i);//按键抖动反馈
int Date_difference(int y1,int m1,int d1,int y2,int m2,int d2);//两日期之差
void page1(int num,int year,int month,int day,int hour ,int week);//主页(高考)
void page2();//激励页
void page3();//绘制配网页面
void page4();//显示电量低
// void alarm();//rtc中断执行函数
void mode1();//模式1显示高考页
void mode2();//模式2显示激励页
void mode3();//显示网络配置状态
int Voltage();//电压采集

//墨水屏初始化
void Elnksetup(){
  DEV_Module_Init();//引脚定义初始化

  printf("e-Paper Init and Clear...\r\n");
  EPD_2IN13BC_Init();//屏幕型号初始化
  EPD_2IN13BC_Clear();//清屏
  DEV_Delay_ms(500);

  // UBYTE *BlackImage, *RYImage;
  // UWORD Imagesize = ((EPD_2IN13BC_WIDTH % 8 == 0) ? (EPD_2IN13BC_WIDTH / 8 ) : (EPD_2IN13BC_WIDTH / 8 + 1)) * EPD_2IN13BC_HEIGHT;
  // if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
  //   Serial.println("Failed to apply for black memory...\r\n");
  //   while (1);
  // }
  // if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL) {
  //   Serial.println("Failed to apply for red memory...\r\n");
  //   while (1);
  // }
  // Paint_NewImage(BlackImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);
  // Paint_NewImage(RYImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);//页面初始化参数

  // //清屏
  // // Paint_SelectImage(BlackImage);
  // // Paint_Clear(WHITE);
  // // Paint_SelectImage(RYImage);
  // // Paint_Clear(WHITE);
  // // delay(500);

  // //黑色页 
  // Paint_SelectImage(BlackImage);
  // Paint_Clear(WHITE);
  // Paint_DrawString_EN(15,15,"Star Plan",&Font24,WHITE,BLACK);
  // Paint_DrawString_CN(15,35,"繁星计划",&Font24CN,WHITE,BLACK);

  // //红色页
  // const char chinese[25] = "前方还有星辰大海";
  // Paint_SelectImage(RYImage);
  // Paint_Clear(WHITE);
  // Paint_DrawChar(5,78,chinese[25],&Font12,WHITE,RED);
  // Paint_DrawString_CN(5,78,"前方还有星辰大海",&Font12CN,WHITE,RED);
  
  Serial.println("Dispiay..");
  EPD_2IN13BC_Display(gImage_setupblack,gImage_setupred);
  delay(2000);

  Serial.println("Goto Sleep...");
  EPD_2IN13BC_Sleep();//关闭墨水屏
  // free(BlackImage);
  // free(RYImage);
  // BlackImage = NULL;
  // RYImage = NULL;
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
        configTime(TZ_SEC, DST_SEC, NTP1, NTP2,NTP3);//获取当前时间"ntp.ntsc.ac.cn""ntp1.aliyun.com"
        struct tm  timeinfo;
        if (!getLocalTime(&timeinfo)){ //一定要加这个条件判断，否则内存溢出
          Serial.println("Failed to obtain time");
          return;
        }
        else{
          Serial.println("获得网络时间");
          Serial.println(asctime(&timeinfo));
          rtc.init();
          Time t(timeinfo.tm_year + 1900,timeinfo.tm_mon + 1,timeinfo.tm_mday,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec,timeinfo.tm_wday);
          rtc.setTime(t);
          Serial.println("setting RTC date& time.");
          // return true;
          t = rtc.getTime();
        }
    }
    // configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com");//获取当前时间
    // struct tm*  timeinfo;
    // if(getLocalTime(timeinfo)){
    //   Serial.println("获得网络时间"); 
    //   Serial.println(asctime(timeinfo));
    //   Time t(timeinfo->tm_year + 1900,timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_wday); 
    // }
  
}

//RTC初始化   
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
// void RTC_timeupdate(){
//   if(!res){
//     // return false;
//   }
//   else{
//     configTime(TZ_SEC, DST_SEC, "ntp.ntsc.ac.cn", "ntp1.aliyun.com");//获取当前时间
//     struct tm*  timeinfo;
//     if(getLocalTime(timeinfo)){
//       Serial.println("获得网络时间");
//       Serial.println(asctime(timeinfo));
//       rtc.init();
//       Time t(timeinfo->tm_year + 1900,timeinfo->tm_mon + 1,timeinfo->tm_mday,timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_wday);
//       rtc.setTime(t);
//       Serial.println("setting RTC date& time.");
//       // return true;
//     }
//     // return false;
//   }
//   t = rtc.getTime();
// }

// 触摸按键1
// bool Buttontouch_1(){
//   int i;
//   i = touchRead(T8);//触摸脚引33
//   if(i <= 40){//触摸阈值由后期读数为准，不一定为40
//     delay(2);//延时防抖，具体值还需测试
//     return true;
//   }  
//   else{
//     return false;
//   }
// }

// // 触摸按键2
// bool Buttontouch_2(){
//   int i;
//   i = touchRead(T9);//触摸引脚32
//   if(i <= 40){
//     delay(2);
//     return true;
//   }  
//   else{
//     return false;
//   }
// }
// //按键检测
// void Swishtest(){
//   if(Buttontouch_1())
//     Serial.print("Buttontouch_1 按下");
//   if(Buttontouch_2())
//     Serial.print("Buttontouch_2 按下");
// }

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

//判断开关状态 time判断时长 未按下返回0，短按中键返回1，长按中键返回2，上下键分别返回3，4
int Switch(int time){
  int cnt = 0;
  if(digitalRead(36)){
    // digitalWrite(4,HIGH);
    delay(10);//防抖
    // digitalWrite(4,LOW);
    if(digitalRead(36)){
      while(digitalRead(36))//等待按键释放
      {
        delay(10);
        cnt++;
        // Serial.println(cnt);
      }
    }
  }
  if(digitalRead(34)){
    // digitalWrite(4,HIGH);
    delay(10);//防抖
    // digitalWrite(4,LOW);
    if(digitalRead(34)){
      while(digitalRead(34)){
        delay(10);
      }
      return 3;
    }
  }
  if(digitalRead(39)){
    // digitalWrite(4,HIGH);
    delay(10);//防抖
    // digitalWrite(4,LOW);
    if(digitalRead(39)){
      while (digitalRead(39)){
        delay(10);
      }
      return 4;
    }
  }

  if(cnt == 0){
    return 0;
  }
  else{
    if(cnt<time/10)
      return 1;//短按
    else
      return 2;//长按  
  }
}

//按键振动
void shake(int i){
  if(i==2){
    digitalWrite(4,HIGH);
    delay(100);
    digitalWrite(4,LOW);
    delay(50);
    digitalWrite(4,HIGH);
    delay(100);
    digitalWrite(4,LOW);
  }
  if(i==0){
    return;
  }
  else{
    digitalWrite(4,HIGH);
    delay(200);
    digitalWrite(4,LOW);
  }
}

//RTC中断执行函数
// void alarm(){
//   alarmFlag = true;
  
// }
//模式1
void mode1(){
  t = rtc.getTime();
  // RTC_DATA_ATTR int hour = 0;
  // attachInterrupt(0,alarm, FALLING);
  // rtc.enAlarm1Interrupt(hour + 1,1);
  int hour = t.hour();
  if(hour>=6 && hour<=22){
    int i = Date_difference(t.year(),t.month(),t.date(),YEAR1,MONTH1,DAY1);
    if(drawpage)
      page1(i,t.year(),t.month(),t.date(),t.hour(),t.day());
    // page1(i,t.year(),t.month(),t.date(),t.hour(),t.day());
    // alarmFlag = false;
    int lateminute = (60 - t.minute());
    if(lateminute>30){
      esp_sleep_enable_timer_wakeup(31*TIME_TO_SLEEP*uS_TO_S_FACTOR);//每31分钟苏醒检测状态
      drawpage = false;
      // delay(1000);
      // esp_deep_sleep_start();
    }
    else{
      drawpage = true;
      esp_sleep_enable_timer_wakeup(lateminute*TIME_TO_SLEEP*uS_TO_S_FACTOR);//深度睡眠唤醒时间
    }
    Serial.println(lateminute);
    Serial.println(t.hour());
    // if(drawpage)
    //   page1(i,t.year(),t.month(),t.date(),t.hour(),t.day());
  }
  else{
    esp_sleep_enable_timer_wakeup(30*TIME_TO_SLEEP*uS_TO_S_FACTOR);//每30分钟苏醒检测状态
    delay(1000);
    // esp_deep_sleep_start();
  }
  delay(2000);
  // esp_deep_sleep_start();//进入深度睡眠
}

void mode2(){
  page2();
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 1);
  // esp_deep_sleep_start();
}

void mode3(){
  page3();
  if(!res){
    Wifisetup();
    page3();//显示WiFi已配置
  }
  // page3();//显示WiFi已配置
}

int Voltage(){
  int powerV,i;
  // int adc1_reading[1] = {0xcc};
  digitalWrite(25,HIGH);

  
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_7, ADC_ATTEN_DB_11);
  delay(200);
  
  i = adc1_get_raw(ADC1_CHANNEL_7);
  // i = (adc1_reading[0] * 2500.00)/4095.00;
  Serial.println(i);

  delay(100);
  powerV = map(i,0,2545,0,26);
  digitalWrite(25,LOW);
  return powerV;
}

void setup() {
  Serial.begin(115200);
  pinMode(34,INPUT);
  pinMode(36,INPUT);
  pinMode(39,INPUT);//输入引脚
  pinMode(4,OUTPUT);//振动模块引脚
  pinMode(25,OUTPUT);//电压采集通断开关

  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0){
    digitalWrite(4,HIGH);/*振动表示检测到rtc.io唤醒*/
    delay(200);
    digitalWrite(4,LOW);
    delay(100);
    digitalWrite(4,HIGH);
    delay(400);
    digitalWrite(4,LOW);
  }
  // esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 1);
  // esp_sleep_enable_timer_wakeup(2*TIME_TO_SLEEP*uS_TO_S_FACTOR);//深度睡眠唤醒时间
  RTC_Setup();
  if(firstwifiElinkconfit){
    Elnksetup(); 
    Wifisetup();
    firstwifiElinkconfit = false; 
    mode1();
  }
  // Elnksetup();
  // RTC_Setup();
  // Wifisetup();
  // int i = Date_difference(t.year(),t.month(),t.date(),YEAR1,MONTH1,DAY1);//剩余天数
  // page1(i,t.year(),t.month(),t.date(),t.hour(),t.day());
  delay(1000);
  if(res){
    WiFi.mode(WIFI_OFF);//关闭WiFi节能
    Serial.println("WiFi已关闭");
    // res = false;
  }
  // if(Voltage()<=14){
  //   page4();
  // }
  // else{
  if(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER){
    if(Modestate == 1){
      mode1();
      delay(500);
      esp_deep_sleep_start();
    }
    // else if(Modestate == 2){
    //   mode2();
    // }
    // else if(Modestate == 3){
    //   mode3();
    // }
    // else
    //   Modestate = 1;
  }
  // }
  // Modestate++;
  // Serial.println(Voltage());
  // digitalWrite(25,LOW);
  // Serial.println("即将进入睡眠");
  // esp_deep_sleep_start();//进入深度睡眠
}

void loop() {
  int i = 0;
  // int waitTime;//当前等待时间为0.01*waitTime
  i = Switch(1500);
  shake(i);
  if(i == 0)
    waitTime++;

  else{
    if(i == 4){
      Modestate++;
    }
    if(i == 3){
      Modestate--;
    }
    if(i == 1){
      if(Modestate == 1){
        drawpage = true;
        mode1();
      }
      else if(Modestate == 2){
        mode2();
      }
      else if(Modestate == 3){
        mode3();
      }
      else
        Modestate = 1;
    }
    waitTime = 0;//等待时间重置
    if(i == 2){
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 1);
      digitalWrite(4,HIGH);/*振动表示准备开始深度睡眠*/
      delay(200);
      digitalWrite(4,LOW);
      delay(100);
      digitalWrite(4,HIGH);
      delay(50);
      digitalWrite(4,LOW);
      delay(100);
      esp_deep_sleep_start();
    }
    // waitTime = 0;//等待时间重置
  }  
  delay(10);
  // waitTime++;
  Serial.println(waitTime);
  if(waitTime>=(STANDTIME*6000)){
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_36, 1);
    delay(10);
    esp_deep_sleep_start();
  }
  // Serial.println(i);
  // Serial.println("hello world!");
  // delay(1000);
}

//主页面(高考)(倒计时天数，年，月，日，小时，星期(0--6))
void page1(int num,int year,int month,int day,int hour ,int week){
  int dayhig = 88; 
  const String WDAY_NAMES[] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"};
  DEV_Module_Init(); 
  EPD_2IN13BC_Init();
  EPD_2IN13BC_Clear();//清屏
  delay(500);

  UWORD Imagesize = ((EPD_2IN13BC_WIDTH % 8 == 0) ? (EPD_2IN13BC_WIDTH / 8 ) : (EPD_2IN13BC_WIDTH / 8 + 1)) * EPD_2IN13BC_HEIGHT;
  UBYTE *RYImage;
  
  if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    Serial.println("Failed to apply for red memory...\r\n");
    while (1);
  }
  
  Paint_NewImage(RYImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);//页面初始化参数
  
  Paint_SelectImage(RYImage);
  Paint_Clear(WHITE);

  
  Paint_SelectImage(RYImage);
  Paint_DrawNum(20,dayhig,year,&Font12,BLACK,WHITE);//年份
  Paint_DrawNum(86,dayhig,month,&Font12,BLACK,WHITE);//月份
  Paint_DrawNum(123,dayhig,day,&Font12,BLACK,WHITE);//天
  Paint_DrawString_EN(174,dayhig,WDAY_NAMES[week].c_str(),&Font12,WHITE,BLACK);//星期

  Paint_SelectImage(RYImage);
  Paint_DrawNum(115,40,num,&Font24,RED,WHITE);//倒计时
  Paint_DrawNum(145,69,STUDYHOUR  - hour,&Font16,RED,WHITE);//单日学习时间
  EPD_2IN13BC_Display(gImage_mainpage,RYImage);//绘制UI界面和数据
  delay(1000);

  Serial.println("Goto Sleep...");
  EPD_2IN13BC_Sleep();//关闭墨水屏
  free(RYImage);
  RYImage = NULL;
}

void page2(){
  DEV_Module_Init();
  EPD_2IN13BC_Init();
  EPD_2IN13BC_Clear();
  delay(500);

  EPD_2IN13BC_Display(gImage_courageblack, gImage_couragered);//绘制鼓励页
  delay(1000);
  EPD_2IN13BC_Sleep();
}

//绘制配网页面
void page3(){
  DEV_Module_Init();
  EPD_2IN13BC_Init();
  EPD_2IN13BC_Clear();
  delay(500);

  UBYTE *BlackImage, *RYImage; // Red or Yellow
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
  Paint_NewImage(RYImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);

  if(!res){
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10,10,"Please use phone to         configure the WIFI....",&Font12,WHITE,BLACK);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(5,70,"Please complete the WIFI in  three minutues...",&Font12,WHITE,BLACK);
  }
  else{
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10,10,"WIFI already configured",&Font12,WHITE,BLACK);
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_DrawString_EN(10,70,"Please choose other modes",&Font12,WHITE,BLACK);
  }
  EPD_2IN13BC_Display(BlackImage,RYImage);
  delay(1000);
  EPD_2IN13BC_Sleep();
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;
}

void page4(){
  DEV_Module_Init();
  EPD_2IN13BC_Init();
  EPD_2IN13BC_Clear();
  delay(500);

  UBYTE *BlackImage, *RYImage; // Red or Yellow
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
  Paint_NewImage(RYImage, EPD_2IN13BC_WIDTH, EPD_2IN13BC_HEIGHT, 270, WHITE);

  Paint_SelectImage(BlackImage);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(10,10,"Low battery power",&Font16,WHITE,BLACK);
  Paint_SelectImage(RYImage);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(15,70,"Please charge",&Font16,WHITE,BLACK);

  EPD_2IN13BC_Display(BlackImage,RYImage);
  delay(1000);
  EPD_2IN13BC_Sleep();
  free(BlackImage);
  free(RYImage);
  BlackImage = NULL;
  RYImage = NULL;
}