#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "./config.h"

ESP8266WiFiMulti wifiMulti;
WiFiClient wifiClient;
HTTPClient http;
const char* ssid1 = MATT_HOTSPOT_SSID;
const char* passwd1 = MATT_HOTSPOT_PASSWORD;

int JACKET_LIGHT = D5;
int BOOTS_LIGHT = D6;
int UMBRELLA_LIGHT = D7;

int jacket_threshold = 50;
unsigned long timer_pointer = 0;
unsigned long five_minutes = 300000;

int temp_current = 0;
int temp_high = 0;
int temp_low = 0;
char* forecast = "";
char* currentUser = "Matt";

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static const unsigned char PROGMEM wifi_icon[] =
{
  0b00000000, 0b00000000, //                 
  0b00000111, 0b11100000, //      ######     
  0b00011111, 0b11111000, //    ##########   
  0b00111111, 0b11111100, //   ############  
  0b01110000, 0b00001110, //  ###        ### 
  0b01100111, 0b11100110, //  ##  ######  ## 
  0b00001111, 0b11110000, //     ########    
  0b00011000, 0b00011000, //    ##      ##   
  0b00000011, 0b11000000, //       ####      
  0b00000111, 0b11100000, //      ######     
  0b00000100, 0b00100000, //      #    #     
  0b00000001, 0b10000000, //        ##       
  0b00000001, 0b10000000, //        ##       
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000 //                 
};

static const unsigned char PROGMEM check_icon[] =
{
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000111, //              ###
  0b00000000, 0b00001111, //             ####
  0b00000000, 0b00011111, //            #####
  0b01110000, 0b00111110, //  ###      ##### 
  0b01111000, 0b01111100, //  ####    #####  
  0b01111100, 0b11111000, //  #####  #####   
  0b00011111, 0b11110000, //    #########    
  0b00001111, 0b11100000, //     #######     
  0b00000111, 0b11000000, //      #####      
  0b00000011, 0b10000000, //       ###       
  0b00000000, 0b00000000, //                 
  0b00000000, 0b00000000 //                 
};

void connectToWifi(){
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid1, passwd1);
  
  Serial.print("\nConnecting");
  
  while (wifiMulti.run() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected to ");
  Serial.print(WiFi.SSID());
  Serial.print(", IP Address: ");
  Serial.println(WiFi.localIP());

  if (wifiMulti.run() == WL_CONNECTED){
    digitalWrite(BUILTIN_LED, LOW);
  }
}

void updateWeather(){
  char* old_forecast = forecast;
  int old_temp_current = temp_current;
  int old_temp_high = temp_high;
  int old_temp_low = temp_low;
  
  String full_uri = "http://api.openweathermap.org/data/2.5/weather?zip=84604,us&units=Imperial&APPID=65afbab12772f80f78a291fe057de213";
  Serial.print("[HTTP] Requesting ");
  Serial.println(full_uri);
  if (http.begin(full_uri)){
    int httpCode = http.GET();

    if (httpCode > 0){
      if (httpCode == HTTP_CODE_OK){
        String payload = http.getString();

        const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(payload);
        JsonObject& weather = root["weather"][0];
        JsonObject& temps = root["main"];

        const char* c_forecast = weather["main"];
        forecast = (char*) c_forecast;
        temp_current = (int) temps["temp"];
        temp_high = (int) temps["temp_max"];
        temp_low = (int) temps["temp_min"];
        
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP] Unable to connect");
    }
  }

  if (strcmp(forecast, old_forecast) != 0 || old_temp_current != temp_current || old_temp_high != temp_high || old_temp_low != temp_low){
    updateDisplay(); 
  }  
}

void updateDisplay(){
  Serial.println("[OLED] Updating Screen...");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, " + String(currentUser));
  display.println();
  display.println("Forecast: " + String(forecast));
  display.println();
  display.print("Temp: " + String(temp_current));
  display.print((char)247);
  display.println("F");
  display.println();  
  display.print("High: " + String(temp_high));
  display.print((char)247);
  display.println("F");
  display.print("Low:  " + String(temp_low));
  display.print((char)247);
  display.println("F");

  display.display();
}

void setup(){
  Serial.begin(115200);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED]\tSSD1306 allocation failed");
    for(;;);
  }

  pinMode(JACKET_LIGHT, OUTPUT);
  pinMode(BOOTS_LIGHT, OUTPUT);
  pinMode(UMBRELLA_LIGHT, OUTPUT);
  
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  // Display static text
  display.println("Connecting to WiFi...");
  display.drawBitmap(56, 24, wifi_icon, 16, 16, 1);
  display.display();
  
  connectToWifi();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  // Display static text
  display.println("Connected!");
  display.drawBitmap(56, 24, check_icon, 16, 16, 1);
  display.display();

  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  // Display static text
  display.println("Starting Services...");
  display.display();
}

void loop(){
  if (millis() > timer_pointer){
    timer_pointer = millis() + 20000;

    updateWeather();
  }

  if ( temp_low < jacket_threshold ){
    digitalWrite(JACKET_LIGHT, true);
  } else {
    digitalWrite(JACKET_LIGHT, false);
  }

  if ( strcmp(forecast, "Rain") == 0 ){
    digitalWrite(UMBRELLA_LIGHT, true);
  } else {
    digitalWrite(UMBRELLA_LIGHT, false);
  }

  if ( strcmp(forecast, "Snow") == 0 || strcmp(forecast, "Rain") == 0 ) {
    digitalWrite(BOOTS_LIGHT, true);
  } else {
    digitalWrite(BOOTS_LIGHT, false);
  }
}
