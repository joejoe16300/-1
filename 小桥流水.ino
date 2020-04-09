/*
 Name:    小熊家居.ino
 Created: 2020/3/30 17:22:26
 Author:  joe
*/
/*************************************************************
  Download latest Blynk library here:
  https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

  Downloads, docs, tutorials: http://www.blynk.cc
  Sketch generator:           http://examples.blynk.cc
  Blynk community:            http://community.blynk.cc
  Follow us:                  http://www.fb.com/blynkapp
		http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on ESP8266 chip.

  Note: This requires ESP8266 support package:
  https://github.com/esp8266/Arduino

  Please be sure to select the right ESP8266 module
  in the Tools -> Board menu!

  NOTE: SmartConfig might not work in your environment.
  Please try basic ESP8266 SmartConfig examples
  before using this sketch!

  Change Blynk auth token to run 11:)

 *************************************************************/

 /* Comment this out to disable prints and save space */
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


#include <ArduinoJson.h>
#include <Ticker.h> 

#define BLYNK_PRINT Serial
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 4, /* data=*/ 5, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display
#define BQ 12
float t = 0;
float h = 0;
Ticker flipper;
// You should get Auth Token in the Blynk App.
char auth[] = "FgNjLohQT1p5K5ihdWl-Po76BLuCQhqr";
BlynkTimer timer;
BlynkTimer timer1;
//dht11
#define DHTPIN D7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// NTP Servers:
static const char ntpServerName[] = "cn.ntp.org.cn";

//static const char ntpServerName[] = "time.nist.gov";

//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";

//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";

//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";
const int timeZone = 8; //BeiJing in China
WiFiUDP Udp;

WiFiClient client;
/*  请求的Json数据格式如下：
 * {
 *    "results": [
 *        {
 *            "location": {
 *                "id": "WX4FBXXFKE4F",
 *                "name": "北京",
 *                "country": "CN",
 *                "path": "北京,北京,中国",
 *                "timezone": "Asia/Shanghai",
 *                "timezone_offset": "+08:00"
 *            },
 *            "now": {
 *                "text": "多云",
 *                "code": "4",
 *                "temperature": "23"
 *            },
 *            "last_update": "2019-10-13T09:51:00+08:00"
 *        }
 *    ]
 *}
 */
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t prevDisplay = 0; // when the digital clock was displayed
int step_input = 1;
time_t getNtpTime();

void digitalClockDisplay();

void printDigits(int digits);

void sendNTPpacket(IPAddress& address);


WidgetLED led1(V1);

BLYNK_WRITE(V0) {
	int button = param.asInt(); // read button
	if (button == 1)
	{
		digitalWrite(BQ, LOW);
		led1.on();
	}

	else {
		digitalWrite(BQ, HIGH);

		led1.off();
	}

}



BLYNK_WRITE(V4) {

	step_input = param.asInt();
}

void setup()
{
	// Debug console
	Serial.begin(9600);
	client.setTimeout(5000);//设置服务器连接超时时间
	u8g2.begin();
	dht.begin();
	pinMode(BQ, OUTPUT);

	WiFi.mode(WIFI_STA);

	int cnt = 0;
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
		if (cnt++ >= 10) {
			WiFi.beginSmartConfig();
			while (1) {
				delay(1000);
				if (WiFi.smartConfigDone()) {
					Serial.println();
					Serial.println("SmartConfig: Success");
					break;
				}
				Serial.print("|");
			}
		}
	}

	WiFi.printDiag(Serial);

	Blynk.config(auth);

	//ntc
	Serial.print("IP number assigned by DHCP is ");

	Serial.println(WiFi.localIP());

	Serial.println("Starting UDP");

	Udp.begin(localPort);

	Serial.print("Local port: ");

	Serial.println(Udp.localPort());

	Serial.println("waiting for sync");

	setSyncProvider(getNtpTime);

	setSyncInterval(300);


	timer.setInterval(60 * 1000, sendSensor);//1分钟上传一次温湿度 天气数据


}
void loop()
{


	Blynk.run();
	timer.run();
	fanye();



}


//u8g2页面1
void display_u8g2_1() {
	//------------>x
	//|
	//|
	//|
	//|
	//v y
	u8g2.clearBuffer();         // clear the internal memory
	u8g2.setFont(u8g2_font_helvB12_tf); // choose a suitable font
	//u8g2.setFont(u8g2_font_unifont_t_chinese1);

	u8g2.setCursor(0, 30);
	u8g2.print("Tem:");
	u8g2.setCursor(50, 30);
	u8g2.print(t);
	u8g2.setCursor(105, 30);
	u8g2.setFont(u8g2_font_helvB10_tf);
	u8g2.print("C");

	u8g2.setFont(u8g2_font_helvB12_tf);
	u8g2.setCursor(0, 50);
	u8g2.print("Hum:");
	u8g2.setCursor(50, 50);
	u8g2.print(h);
	u8g2.setCursor(105, 50);
	u8g2.setFont(u8g2_font_helvB10_tf);
	u8g2.print("%");

	//wifi标识logo
	if (WiFi.status() == WL_CONNECTED)
	{
		draw_wifi(8, 10);
	}
	//页面1标识
	yemian_index1();
	//打印时间
	printtime();

	u8g2.sendBuffer();


}
void sendSensor() {

	t = dht.readTemperature();
	h = dht.readHumidity();
	Blynk.virtualWrite(V2, t);
	Blynk.virtualWrite(V3, h);
	Serial.println(t);
	Serial.println(h);
	ws_sender();

}

void ws_sender() {
	if (client.connect("api.seniverse.com", 80) == 1)              //连接服务器并判断是否连接成功，若成功就发送GET 请求数据下发       
	{                                           //换成你自己在心知天气申请的私钥//改成你所在城市的拼音
		client.print("GET /v3/weather/now.json?key=SgvfjlJFSm4QOMdLE&location=jinhua&language=zh-Hans&unit=c HTTP/1.1\r\n"); //心知天气的URL格式          
		client.print("Host:api.seniverse.com\r\n");
		client.print("Accept-Language:zh-cn\r\n");
		client.print("Connection:close\r\n\r\n"); //向心知天气的服务器发送请求。


		String status_code = client.readStringUntil('\r');        //读取GET数据，服务器返回的状态码，若成功则返回状态码200
		Serial.println(status_code);

		if (client.find("\r\n\r\n") == 1)                            //跳过返回的数据头，直接读取后面的JSON数据，
		{
			String json_from_server = client.readStringUntil('\n');  //读取返回的JSON数据
			Serial.println(json_from_server);
			parseUserData(json_from_server);                      //将读取的JSON数据，传送到JSON解析函数中进行显示。
		}
	}
	else
	{
		Serial.println("connection failed this time");
		delay(5000);                                            //请求失败等5秒
	}

	client.stop();                                            //关闭HTTP客户端，采用HTTP短链接，数据请求完毕后要客户端要主动断开https://blog.csdn.net/huangjin0507/article/details/52396580

}
void fanye() {
	switch (step_input)
	{
	case 1: {


		display_u8g2_1();

		break;
	}
	case 2: {
		Serial.print("asinput==");
		Serial.println(step_input);
		u8g2.clearBuffer();         // clear the internal memory
		u8g2.setFont(u8g2_font_helvB14_tf); // choose a suitable font
		//u8g2.setFont(u8g2_font_unifont_t_chinese1);

		u8g2.setCursor(0, 40);
		u8g2.print("qiao dawei wei");
		u8g2.sendBuffer();
		break;
	}
	default:
		break;
	}
}
void parseUserData(String content)  // Json数据解析并串口打印.可参考https://www.bilibili.com/video/av65322772
{
	const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 210;
	DynamicJsonBuffer jsonBuffer(capacity);

	JsonObject& root = jsonBuffer.parseObject(content);

	JsonObject& results_0 = root["results"][0];

	JsonObject& results_0_location = results_0["location"];
	const char* results_0_location_id = results_0_location["id"];
	const char* results_0_location_name = results_0_location["name"];
	const char* results_0_location_country = results_0_location["country"];
	const char* results_0_location_path = results_0_location["path"];
	const char* results_0_location_timezone = results_0_location["timezone"];
	const char* results_0_location_timezone_offset = results_0_location["timezone_offset"];

	JsonObject& results_0_now = results_0["now"];
	const char* results_0_now_text = results_0_now["text"];
	const char* results_0_now_code = results_0_now["code"];
	const char* results_0_now_temperature = results_0_now["temperature"];
	const char* results_0_last_update = results_0["last_update"];

	Serial.println(results_0_location_name);                       //通过串口打印出需要的信息
	Serial.println(results_0_now_text);
	Serial.println(results_0_now_code);
	Serial.println(results_0_now_temperature);
	Serial.println(results_0_last_update);
	Serial.print("\r\n");
}
void yemian_index1() {

	u8g2.drawCircle(45, 60, 3);
	u8g2.drawLine(50, 60, 55, 60);
	u8g2.drawLine(57, 60, 62, 60);
	u8g2.drawLine(64, 60, 69, 60);
}

void draw_wifi(int x, int y) {
	u8g2.drawCircle(x, y, 2);
	u8g2.drawLine(x - 3, y - 5, x + 3, y - 5);
	u8g2.drawLine(x - 5, y - 7, x + 5, y - 7);
	u8g2.drawLine(x - 7, y - 9, x + 7, y - 9);
}


void printtime() {
	//打印时间
	u8g2.setFont(u8g2_font_helvB10_tf);

	u8g2.setCursor(35, 12);
	u8g2.print(hour());
	u8g2.setCursor(53, 12);
	u8g2.print(":");

	u8g2.setCursor(60, 12);
	u8g2.print(minute());
	u8g2.setCursor(78, 12);
	u8g2.print(":");

	u8g2.setCursor(84, 12);
	u8g2.print(second());


}

void digitalClockDisplay()

{

	// digital clock display of the time




	Serial.print(hour());

	printDigits(minute());

	printDigits(second());

	Serial.print(" ");

	Serial.print(day());

	Serial.print(".");

	Serial.print(month());

	Serial.print(".");

	Serial.print(year());

	Serial.println();

}



void printDigits(int digits)

{

	// utility for digital clock display: prints preceding colon and leading 0

	Serial.print(":");

	if (digits < 10)

		Serial.print('0');

	Serial.print(digits);

}



/*-------- NTP code ----------*/



const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets



time_t getNtpTime()

{

	IPAddress ntpServerIP; // NTP server's ip address



	while (Udp.parsePacket() > 0); // discard any previously received packets

	Serial.println("Transmit NTP Request");

	// get a random server from the pool

	WiFi.hostByName(ntpServerName, ntpServerIP);

	Serial.print(ntpServerName);

	Serial.print(": ");

	Serial.println(ntpServerIP);

	sendNTPpacket(ntpServerIP);

	uint32_t beginWait = millis();

	while (millis() - beginWait < 1500) {

		int size = Udp.parsePacket();

		if (size >= NTP_PACKET_SIZE) {

			Serial.println("Receive NTP Response");

			Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer

			unsigned long secsSince1900;

			// convert four bytes starting at location 40 to a long integer

			secsSince1900 = (unsigned long)packetBuffer[40] << 24;

			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;

			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;

			secsSince1900 |= (unsigned long)packetBuffer[43];

			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;

		}

	}

	Serial.println("No NTP Response :-(");

	return 0; // return 0 if unable to get the time

}



// send an NTP request to the time server at the given address

void sendNTPpacket(IPAddress& address)

{

	// set all bytes in the buffer to 0

	memset(packetBuffer, 0, NTP_PACKET_SIZE);

	// Initialize values needed to form NTP request

	// (see URL above for details on the packets)

	packetBuffer[0] = 0b11100011;   // LI, Version, Mode

	packetBuffer[1] = 0;     // Stratum, or type of clock

	packetBuffer[2] = 6;     // Polling Interval

	packetBuffer[3] = 0xEC;  // Peer Clock Precision

	// 8 bytes of zero for Root Delay & Root Dispersion

	packetBuffer[12] = 49;

	packetBuffer[13] = 0x4E;

	packetBuffer[14] = 49;

	packetBuffer[15] = 52;

	// all NTP fields have been given values, now

	// you can send a packet requesting a timestamp:

	Udp.beginPacket(address, 123); //NTP requests are to port 123

	Udp.write(packetBuffer, NTP_PACKET_SIZE);

	Udp.endPacket();

}