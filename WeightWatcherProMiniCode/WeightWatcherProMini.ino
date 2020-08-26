#include <SoftwareSerial.h>
#include <TimeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <DS1307RTC.h> 
#include "HX711.h"
#include "LowPower.h"

HX711 scale(4, 5); //(DT,SCK)
HX711 scale2(6, 7); //(DT,SCK)
SoftwareSerial espCam(8, 3); // RX, TX

float calibration_factor = -502; // this calibration factor is adjusted according to my load cell 1
float calibration_factor2= -502; // this calibration factor is adjusted according to my load cell 2

float units;
float units2;

int calibrationTime = 5;       
 
//the time when the sensor outputs a low impulse
long unsigned int lastMotion;        
 
//the amount of milliseconds the sensor has to be low
//before we assume all motion has stopped
long unsigned int pause = 15000; 
 
boolean motion = false; 
 
int pirPin = 2;  


void setup() {
  
  Serial.begin(9600);

  //rfid reader
  espCam.begin(115200);
  
  //motion detection setup
  pinMode(pirPin, INPUT);
  digitalWrite(pirPin, LOW);

  //NATURE CHIP
  espCam.println("Nature Chip V1");


  espCam.println("Tare Scale 1");
  delay(1000);
  //Scale--------------------------------------------------
  scale.set_scale();
  scale.tare();  //Reset the scale to 0
  
  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  espCam.println("Tare 2 Scale 2");
  delay(1000);
  //Scale 2 ------------------------------------------------
  scale2.set_scale();
  scale2.tare();  //Reset the scale to 0
  
  scale2.set_scale(calibration_factor2); //Adjust to this calibration factor

  //Clock -----------------------------------------------------
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
     espCam.println("Unable to sync with the RTC");
  else
     espCam.println("RTC has set the system time");

  LoRa.setPins(A0, A1);
  
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    //while (1);
  }
  
  //LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(31.25E3);

  LoRa.sleep();
  LoRa.setSyncWord(0xF3);
  delay(50);
  LoRa.idle();
  Serial.println("LoRa Initialised");
  delay(20);
  
  //Send out LoRa confirmation on 0xAA
  LoRa.sleep();
    delay(20);
    LoRa.setSyncWord(0xAA);
    delay(20);
    LoRa.idle();
    
    LoRa.beginPacket();
    LoRa.print("Nature running");
    LoRa.endPacket();
    delay(10);
    
    LoRa.sleep();
    delay(20);
    LoRa.setSyncWord(0xF3);
    delay(200);
    LoRa.idle();
    
}
//WAKE UP FROM SLEEP HANDLER ---------------------------------
void wakeUp()
{
  //espCam.begin(115200);
  
}


//LOOP ---------------------------------------------
void loop(){
 
    takeReading();
    /* if(digitalRead(pirPin) == HIGH){
         takeReading();
         lastMotion = millis();
         motion = true;
       }
 
     else if(digitalRead(pirPin) == LOW){      
        
        if (motion == true){
           takeReading();
        }
        if(millis() - lastMotion > pause){ 
                            
           motion = false;
           espCam.println("Sleep");

           // Power down till motion
          
           attachInterrupt(digitalPinToInterrupt(pirPin), wakeUp, RISING);
           delay(200);
           
           LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
        
           delay(200);
           detachInterrupt (digitalPinToInterrupt(pirPin));
    
           }
      }*/
}
  


void takeReading() {
  String output;
  String rf = "rf:";
  String data = " ";
  units = scale.get_units(), 1;
  units2 = scale2.get_units(), 1;
  if (units < 0)
  {
    units = 0.0;
  }
  if (units2 < 0)
  {
    units2 = 0.0;
  }

  while (Serial.available()) {
      delay(3);
      output += char(Serial.read());
  }
  //Checking if time is set. If set, calls RTC functions
  if (timeStatus() == timeSet) {
    digitalClockDisplay();
  } else {
    espCam.println("No Time Stamp");
    delay(500);
  }
  
  espCam.print(units);
  espCam.print(" ");
  espCam.print(units2);
  espCam.print(" ");
  espCam.print(output);
  espCam.println();

  /*LoRa.beginPacket();
  LoRa.print(output);
  LoRa.endPacket();
  delay(10);
 */
  delay(1000); 
 
}
//RTC Functions --------------------------------------------------
void digitalClockDisplay(){
  // digital clock display of the time
  espCam.print(hour());
  printDigits(minute());
  printDigits(second());
  espCam.print(" ");
  espCam.print(day());
  //espCam.print("");
  espCam.print(month());
//  espCam.print(" ");
  espCam.print(year()); 
  espCam.print(" "); 
}
//RTC Function---------------------------------------------------------
void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  //espCam.print(":");
  if(digits < 10)
    espCam.print('0');
  espCam.print(digits);
}
