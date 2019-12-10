#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include "./config.h"

ESP8266WiFiMulti wifiMulti;
WiFiClient wifiClient;
PubSubClient client("172.20.10.14", 1883, wifiClient);
String device_name = "esp8266-stop-light";
const char* ssid1 = MATT_HOTSPOT_SSID;
const char* passwd1 = MATT_HOTSPOT_PASSWORD;

int JACKET_LIGHT = D5;
int BOOTS_LIGHT = D6;
int UMBRELLA_LIGHT = D7:

int jacket_threshold = 50;
unsigned long timer_pointer = 0;
unsigned long five_minutes = 300000;

int temp_current = 0;
int temp_high = 0;
int temp_low = 0;
char* forcast = "";

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

void reconnectToHub() {
  while (!client.connected()){
    if (client.connect((char*) device_name.c_str(), "mqtt", "awesome")){
      Serial.println("Connected to MQTT Hub");
      String message = "Stoplight (" + device_name + ") connected @ " + WiFi.localIP().toString();
      client.publish("/connections/stop-light", message.c_str());
      client.publish("homeassistant/sensor/jacket-threshold/config", "{\"name\": \"jacket-threshold\", \"state_topic\": \"/morning-helper/jacket-threshold\"}"); 
      client.subscribe("/stop-light/change");
    } else {
      Serial.println("Connection to MQTT failed, trying again...");
      delay(3000);
    }  
  }  
}

void updateWeather(){
  String full_uri = "http://api.openweathermap.org/data/2.5/weather?zip=84604,us&APPID=65afbab12772f80f78a291fe057de213"
  Serial.print("[HTTP] Requesting ");
  Serial.println(full_uri);
  if (http.begin(client, full_uri)){
    int httpCode = http.GET();

    if (httpCode > 0){
      if (httpCode == HTTP_CODE_OK){
        String payload = http.getString();

        const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
        DynamicJsonBuffer jsonBuffer(bufferSize);
        JsonObject& root = jsonBuffer.parseObject(payload);
        JsonObject& weather = root["weather"];
        JsonObject& temps = root["main"];

        forecast = weather["main"];
        temp_current = (int) temps["temp"];
        temp_high = (int) temps["temp_max"];
        temp_low = (int) temps["temp_min"];
        
        Serial.print("Forecast: ");
        Serial.println(forecast);
        Serial.print("Current Temp: ");
        Serial.println(temp_current);
        Serial.print("High Temp: ");
        Serial.println(temp_high);
        Serial.print("Low Temp: ");
        Serial.println(temp_low);
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP] Unable to connect");
    }
  }

  updateDisplay();
}

void updateDisplay(){
  Serial.println("Updating Screen...");
}

void setup(){
  Serial.begin(115200);
  connectToWifi();

  pinMode(JACKET_LIGHT, OUTPUT);
  pinMode(BOOTS_LIGHT, OUTPUT);
  pinMode(UMBRELLA_LIGHT, OUTPUT);
}

void loop(){
  if (!client.connected()){
    reconnectToHub();
  }

  client.loop();
  if (millis() > timer_pointer){
    timer_pointer = millis() + five_minutes;

    updateWeather()
  }

  if ( temp_low < jacket_threshold ){
    digitalWrite(JACKET_LIGHT, true);
  } else {
    digitalWrite(JACKET_LIGHT, false);
  }

  if ( forecast == "Rain" ){
    digitalWrite(UMBRELLA_LIGHT, true);
    digitalWrite(BOOTS_LIGHT, true);
  } else {
    digitalWrite(UMBRELLA_LIGHT, false);
    digitalWrite(BOOTS_LIGHT, false);
  }

  if ( forecast == "Snow" ) {
    digitalWrite(BOOTS_LIGHT, true);
  } else {
    digitalWrite(BOOTS_LIGHT, false);
  }
}
