////////////////////////////////////////////////////////////////////////
// Arduino Bluetooth Interface with Mindwave
// 
// This is example code provided by NeuroSky, Inc. and is provided
// license free.
////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include "FanMonitor.h"
#include <Wire.h>
#include <SoftwareSerial.h>

#define FAN_MONITOR_PIN 12
#define LED 13
#define BAUDRATE 57600
#define DEBUGOUTPUT 0

#define powercontrol 10

#define LED1 2
#define LED2 3
#define LED3 4
#define relax 7
#define tight 8
//wifi
String api_key="C2EL0JEN19Z673U8";  //Thingspeak API Write Key
SoftwareSerial sSerial(10,11);

// checksum variables
byte generatedChecksum = 0;
byte checksum = 0; 
int payloadLength = 0;
byte payloadData[64] = {0};
byte poorQuality = 0;
byte attention = 0;
byte meditation = 0;

// system variables
long lastReceivedPacket = 0;
boolean bigPacket = false;

// set motor
const int motorIn1 = 9;
const int motorIn2 = 6;
const byte fans = 5;
int speed;

//read rpm
FanMonitor _fanMonitor = FanMonitor(FAN_MONITOR_PIN, FAN_TYPE_UNIPOLE);
int rpms;

//user initialize
int initial = 0;
byte attent_low = 100;
byte attent_high = 0;
byte section;
byte section_1;
byte section_2;
boolean start = false;

//////////////////////////
// Microprocessor Setup //
//////////////////////////
void setup() {
  Serial.begin(BAUDRATE);           // USB
  sSerial.begin(9600);
  _fanMonitor.begin();
  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(relax, OUTPUT);
  pinMode(tight, OUTPUT);
  sSerial.println("AT+RST");
}

////////////////////////////////
// Read data from Serial UART //
////////////////////////////////
byte ReadOneByte() {
  int ByteRead;

  while(!Serial.available());
  ByteRead = Serial.read();

#if DEBUGOUTPUT  
  Serial.print((char)ByteRead);   // echo the same byte out the USB serial (for debug purposes)
#endif

  return ByteRead;
}

/////////////
//MAIN LOOP//
/////////////
void loop() {
  motorControl();
}
 
void motorControl(){
 // Look for sync bytes
  if(ReadOneByte() == 170) {
    if(ReadOneByte() == 170) {

      payloadLength = ReadOneByte();
      if(payloadLength > 169)                      //Payload length can not be greater than 169
          return;

      generatedChecksum = 0;        
      for(int i = 0; i < payloadLength; i++) {  
        payloadData[i] = ReadOneByte();            //Read payload into memory
        generatedChecksum += payloadData[i];
      }   

      checksum = ReadOneByte();                      //Read checksum byte from stream      
      generatedChecksum = 255 - generatedChecksum;   //Take one's compliment of generated checksum

        if(checksum == generatedChecksum) {    

        poorQuality = 200;
        attention = 0;
        meditation = 0;

        for(int i = 0; i < payloadLength; i++) {    // Parse the payload
          switch (payloadData[i]) {
          case 2:
            i++;            
            poorQuality = payloadData[i];
            bigPacket = true;            
            break;
          case 4:
            i++;
            attention = payloadData[i];                        
            break;
          case 5:
            i++;
            meditation = payloadData[i];
            break;
          case 0x80:
            i = i + 3;
            break;
          case 0x83:
            i = i + 25;      
            break;
          default:
            break;
          } // switch
        } // for loop

#if !DEBUGOUTPUT

        // *** Add your code here ***

        if(bigPacket) {
          if(poorQuality == 0)
            digitalWrite(LED, HIGH);
          else
            digitalWrite(LED, LOW);
          Serial.print("PoorQuality: ");
          Serial.print(poorQuality, DEC);
          Serial.print(" Attention: ");
          Serial.print(attention, DEC);
          Serial.print(" Time since last packet: ");
          Serial.print(millis() - lastReceivedPacket, DEC);
          lastReceivedPacket = millis();
          Serial.print("\n");
      
      if(attention){
      digitalWrite(LED1, LOW);
      digitalWrite(LED2, LOW);
      digitalWrite(LED3, LOW);
      start = true;
      }
      
      if(!start){
      }else{
        if(initial < 30){
        digitalWrite(relax, HIGH);
        digitalWrite(tight, LOW);
        if(attention < attent_low){
          attent_low = attention;
        }
        }else if(initial>=30 && initial<60){
        digitalWrite(relax, LOW);
        digitalWrite(tight, HIGH);
            if(attention > attent_high){
          attent_high = attention;
        }
        }
        initial++;
      if(initial >=60){
        digitalWrite(relax, LOW);
        digitalWrite(tight, LOW);
          section = (attent_high - attent_low)/3;
          section_1 = attent_low + section;
          section_2 = attent_high - section;
        Serial.print(section_1);
        Serial.print(" ");
        Serial.println(section_2);
        if(attention < section_1){
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, LOW);
        if(poorQuality >21){
            motorstop();
        }
        }else if(attention > section_2){
            speed = 255;
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, LOW);
        digitalWrite(LED3, HIGH);
        if(poorQuality >21){
            forward();
        }
              }else{
          speed = 180;
        digitalWrite(LED1, LOW);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, LOW);
        if(poorQuality >21){
            forward();
        }
        }
          
        readingRPM();
        //wifi_Humidity();
      }
      }
        }
#endif        
        bigPacket = false;
      }
      else {
        // Checksum Error
      }  // end if else for checksum
    } // end if read 0xAA byte
  } // end if read 0xAA byte
}

void motorstop()
{
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, LOW);
}

void forward()
{
  analogWrite(fans, speed);
  digitalWrite(motorIn1, HIGH);
  digitalWrite(motorIn2, LOW);
}

void readingRPM(){
  uint16_t rpm = _fanMonitor.getSpeed();

  //將轉速計算後輸出到1602A LCD上
  Serial.print(" RPM = ");
  Serial.print(rpm);
  Serial.print("\n");
  
  rpms = (int)rpm;
}

void wifi_Humidity(){
  String param="&field1=" + (String)attention + "&field2=" + (String)rpms; 
  String cmd="AT+CIPSTART=\"TCP\",\"184.106.153.149\",80";
  sSerial.println(cmd);
  //Serial.println(cmd);  //輸出 AT 指令於監控視窗
  delay(1000);
  String GET="GET /update?api_key=" + api_key + param + "\r\n\r\n";
  //Serial.println(GET);  //顯示 GET 字串內容於監控視窗 
  cmd="AT+CIPSEND=" + String(GET.length());
  sSerial.println(cmd);
  delay(1000);
  //Serial.println(cmd); //輸出 AT 指令於監控視窗
    sSerial.print(GET);  //向 ESP8266 傳送 GET 字串內容
}


