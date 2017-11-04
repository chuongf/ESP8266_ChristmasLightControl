
#include <DHT.h>  
#include <ESP8266WiFi.h>  
#include <WiFiClient.h>  
#include <ThingSpeak.h>  
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "FS.h"
#include <string.h>


// user macro
#define deviceID 1
#define OTAWaitTime 3000  //ms
#define MaxLine 100
uint16_t nLine;
String * data_str;

// Wifi
const char* ssid = "FRITZ!Box 7362 SL";  
const char* password = "mighiu11";  
uint8_t timeout = 10*25; //25 second
WiFiClient client;  

// others
uint32_t tRunOffSet;
uint8_t i;
char fileName[10];

// user functions
//void lineProcess();

void setup()  
{ 
  tRunOffSet = millis();
  // Setup Serial
  Serial.begin(115200);  
  Serial1.begin(125000);
  Serial.print("\r\nDevice #");
  Serial.print(deviceID); 
  Serial.println(" booting...");  

  // setup wifi
  Serial.print("Connect to wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("Wifi connected. IP address: ");
  Serial.println(WiFi.localIP());
  // Setup Over The Air 
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  Serial.print("OTA Setup... ");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  
 // Setup SPIFFS
 Serial.print("Start SPIFFS... ");
  SPIFFS.begin();
  // Next lines have to be done ONLY ONCE!!!!!
  // When SPIFFS is formatted ONCE you can comment these lines out!! and Upload again
//  Serial.println("Please wait 30 secs for SPIFFS to be formatted");
//  SPIFFS.format();
//  Serial.println("Spiffs formatted");
  Serial.print("SPIFFS ready");


}  
void loop()   
{  
      
  // read file
  Serial.println("\n\rReading files from SPIFFS:");
  for(i = 1; i<= 99; i++){

    // set filename
    if(i<10) {
      fileName[0] = '/'; fileName[1] = i + '0'; fileName[2] = '.'; 
      fileName[3] = 'c'; fileName[4] = 's'; fileName[5] = 'v'; fileName[6] = 0;
    }
    else {
      fileName[0] = '/'; fileName[1] = (int)i/10 + '0'; fileName[2] = i%10 + '0'; 
      fileName[3] = '.'; fileName[4] = 'c'; fileName[5] = 's'; fileName[6] = 'v';      
    }

    // open file
    File f = SPIFFS.open(fileName, "r");
    if (!f) {
//        Serial.println("file open failed");
//        Serial.println("end of program, delay 3s: ");
////        Serial.print(millis());
////        Serial.print(" to ");
//        delayWithOTA(3000);
////        Serial.println(millis());
        break;
    }
    Serial.print("\nWorking on file: ");
    Serial.print(fileName);
  
    // process each line
    for(nLine = 1; nLine < MaxLine; nLine++){
      // read line
      String s = f.readStringUntil('\r');   // take 1s to read
      if(s == 0) {
        Serial.println("done, delay 0.5s: ");
//        Serial.print(millis());
//        Serial.print(" to ");
        delayWithOTA(500);
//        Serial.println(millis());
        break;
      }
      // process
      lineProcess(s);
    }

    // closed file 
    
    f.close();

    // wait a few seconds for change to new file, OTA in the mean time
    Serial.println(" ...done \nWait 0.5s for next file and OTA upload: ");
//    Serial.print(millis());
//    Serial.print(" to ");
    delayWithOTA(500);
//    Serial.print(millis());
    
  }
  Serial.println("\nEnd of program. Repeat LED program files");
}

// one line has time_trigger, dimValue1, dimValue2..., dimValuen.
void lineProcess(String s)
{
  int nextCommaIndex;
  String dimValue;
  
//    Serial.print(s);
//    Serial.print("\r");
    // Send start character 81H = 129;
    Serial1.write(129);
//    Serial.print("\rStart Fram: ");

    // first value: time
    int firstCommaIndex = s.indexOf(',');
    int lastCommaIndex = s.lastIndexOf(',');
    
    String time_triggerStr = s.substring(0, firstCommaIndex);
    int32_t time_trigger = (int)(1000*time_triggerStr.toFloat());
//    Serial.println(time_trigger); // for debug only

    // second value: dimVal1
    int previousCommaIndex = firstCommaIndex;
    while(previousCommaIndex < lastCommaIndex) {
      nextCommaIndex = s.indexOf(',', previousCommaIndex+1);
      dimValue = s.substring(previousCommaIndex+1, nextCommaIndex);
      previousCommaIndex = nextCommaIndex;
      Serial1.write(dimValue.toInt());   //for debug only (must be Serial.write)  
//      Serial.println(dimValue.toInt());   //for debug only (must be Serial.write)     
    }
    // last value: dimValn
    dimValue = s.substring(lastCommaIndex+1, s.length());
    Serial1.write(dimValue.toInt());   //for debug only (must be Serial.write)  

    // wait until trigger time;
    time_trigger += tRunOffSet;
    while(millis() < time_trigger) {
      ArduinoOTA.handle();
      yield();
    }
    Serial1.write(130);    //82h as a trigger signal
//    Serial.println("\rEnd Frame");

}

void delayWithOTA(uint32 ms) {
  while(ms--) {
    ArduinoOTA.handle();
    delay(1);
  }
}

