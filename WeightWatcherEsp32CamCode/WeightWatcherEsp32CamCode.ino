/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-cam-pir-motion-detector-photo-capture/
 
  IMPORTANT!!!
   - Select Board "AI Thinker ESP32-CAM"
   - GPIO 0 must be connected to GND to upload a sketch
   - After connecting GPIO 0 to GND, press the ESP32-CAM on-board RESET button to put your board in flashing mode
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
 
#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#include "index.h"  //Web page header file

WebServer server(80);

//Enter your SSID and PASSWORD
const char* ssid = "Toad";
const char* password = "9686936563";


// define the number of bytes you want to access
#define EEPROM_SIZE 2
 
RTC_DATA_ATTR int bootCount = 0;

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
 
int pictureNumber = 0;

long unsigned int lastMotion; 

void handleRoot() {
 String s = MAIN_page; //Read HTML contents
 server.send(200, "text/html", s); //Send web page
}
 
void handleADC() {
 //int a = analogRead(A0);
 //String adcValue = String(a);
 
 //server.send(200, "text/plane", adcValue); //Send ADC value only to client ajax request
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);
 
  Serial.setDebugOutput(true);

  
  Serial.setTimeout(100);
 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  pinMode(13, INPUT);
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_XGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
 
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
 
  Serial.println("Starting SD Card");
 
  delay(50);
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    //return;
  }
 
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }

  uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
  Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

  WiFi.mode(WIFI_STA); //Connectto your wifi
  WiFi.begin(ssid, password);

  Serial.println("Connecting to ");
  Serial.print(ssid);

  //Wait for WiFi to connect
  while(WiFi.waitForConnectResult() != WL_CONNECTED){      
      Serial.print(".");
    }
    
  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP
//----------------------------------------------------------------
 
  server.on("/", handleRoot);      //This is display page
  server.on("/readADC", handleADC);//To get update of ADC Value only
  //server.on("/readTime", handleADC);
  //server.on("/readDate", handleADC);
  //server.on("/readWeight", handleADC);
  //server.on("/readRfid", handleADC);
  
  server.begin();                  //Start server
  Serial.println("HTTP server started");
  pinMode(0, OUTPUT);
  digitalWrite(0,HIGH);
  
} 

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
}

void appendFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
}




void takePhoto(){
  camera_fb_t * fb = NULL;
 
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
 
  // Path where new picture will be saved in SD Card
  String path = "/picture" + String(pictureNumber) +".jpg";
 
  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());
 
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }
  file.close();
  esp_camera_fb_return(fb);
  
  delay(50);
  
  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  

}
int count = 0;
void loop() {

   
   server.handleClient();
   while(Serial.available() > 0)
  {
    EEPROM.begin(EEPROM_SIZE);
    pictureNumber = EEPROM.read(0) + 1;

    String data1 = String(pictureNumber); 

      data1 +=" ";
      data1 += Serial.readString();
      // initialize EEPROM with predefined size
      
      Serial.print("DATA :");
      Serial.println(data1);
      
      //appendFile(SD_MMC, "/Monk.txt", String(pictureNumber));
      appendFile(SD_MMC, "/Monk.txt", data1);

      server.send(200, "text/plane", SendHTML(data1));
    
      takePhoto();
      count = 0;
      lastMotion = millis();
    
      EEPROM.write(0, pictureNumber);
      EEPROM.commit();
  }
}
String SendHTML(String data2){
    int startpos = 0;
    int endpos = 1;
    endpos = data2.indexOf(" ", startpos);
    String picNum = data2.substring(startpos,endpos);
    
    startpos=endpos+1;
    endpos = data2.indexOf(" ",startpos);
    String tim = data2.substring(startpos, endpos);
    
    startpos=endpos+1;
    endpos = data2.indexOf(" ",startpos);
    String date = data2.substring(startpos, endpos);
    
    startpos=endpos+1;
    endpos = data2.indexOf(" ",startpos);
    String weight1 = data2.substring(startpos, endpos);
    
    startpos=endpos+1;
    endpos = data2.indexOf(" ",startpos);
    String weight2 = data2.substring(startpos, endpos);
    
    startpos=endpos+1;
    endpos = data2.indexOf(" ",startpos);
    String rfid = data2.substring(startpos, endpos);
 
 
 // String ptr = "<!DOCTYPE html> <html>\n";
  //ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  
  String ptr ="<title>ESP32 Weather Report</title>\n";
  ptr +="<body>\n";
  ptr +="<div id=\"webpage\">\n";
  
  ptr +="<p>Picture Number : ";
  ptr +=picNum;
  
  ptr +="<p>Time : ";
  ptr +=tim;

  ptr +="<p>Date : ";
  ptr +=date;

  ptr +="<p>Weight 1 : ";
  ptr +=weight1;

  ptr +="<p>Weight 2 : ";
  ptr +=weight2;

  ptr +="<p>RFID : ";
  ptr +=rfid;

  
  
  ptr +="</div>\n";
  ptr +="</body>\n";
  
  //String ptr = dat;
  return ptr;
}
