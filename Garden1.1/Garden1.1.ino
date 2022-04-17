// ce_pin  (RST): default 4 8
// sck_pin (CLK): default 5 6
// io_pin  (DAT): default 6 7
//#include <RTClib.h>
#include <IRremote.h>
#include <virtuabotixRTC.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

int offMinutes=0;
int onMinutes=0;
int onSeconds=0;
int offhours=0;
int nextOnMins=0;
int nextOnHrs=0;
int nextOffmins=0;
int nextOffHrs=0;
int nextOffSecs=0;
const int RECV_PIN = 2;
const int RELAY_PIN = 4;
char buf[20];
bool onTime= false;
bool sysOn= true;
bool lcdLight= true;

LiquidCrystal_I2C lcd(0x27, 16, 2);
//virtuabotixRTC myRTC(5, 6, 4);
virtuabotixRTC myRTC(6, 7, 8);
IRrecv irrecv(RECV_PIN);
decode_results results;

void irRemoteControl();
void increaseHr();
void decreaseHr();
void updateOffTime();
void updateOnTime();
void displaySetTimeLCD();
void displayCurrentTimeLCD();
void loadPreviouState();
void checkDayTime();


void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);
  pinMode(RELAY_PIN, OUTPUT);
  lcd.begin();
  lcd.clear();
  loadPreviouState();
  digitalWrite(RELAY_PIN, LOW);

}

void loadPreviouState(){
  onMinutes=EEPROM.read(0);
  offMinutes=EEPROM.read(1);
  offhours=EEPROM.read(2);
  onSeconds=EEPROM.read(4);
  sysOn=EEPROM.read(3);
  updateOffTime();
  updateOnTime();
}

void loop() {
  irRemoteControl();
  checkDayTime(); //chech if it is day time to run motor
  if(sysOn){
    myRTC.updateTime();
    //attachInterrupt(digitalPinToInterrupt(2), irRemoteControl, HIGH);
    if(lcdLight)
      lcd.backlight();
    else
      lcd.noBacklight();
    if(myRTC.hours==nextOnHrs&&myRTC.minutes==nextOnMins){
      lcd.setCursor(0,0);
      //lcd.print("Motor on");
      digitalWrite(RELAY_PIN, HIGH);
      onTime=true;
    }
    if(myRTC.hours==nextOffHrs&&myRTC.minutes==nextOffmins&&myRTC.seconds==nextOffSecs){
      digitalWrite(RELAY_PIN, LOW);
      updateOffTime();
      updateOnTime();
      onTime=false;
    }
    if(onTime){
      displaySetTimeLCD();
      lcd.setCursor(10,0);
      lcd.print("on");
    }
    else{
      displaySetTimeLCD();
      lcd.setCursor(10,0);
      lcd.print("  ");
    }
  }
  else{
     lcd.clear(); 
     lcd.noBacklight();
     digitalWrite(RELAY_PIN, LOW); 
     onTime=false; 
  }
}

void checkDayTime(){
  if(myRTC.hours==18 && myRTC.minutes==0)
    sysOn=false;
  if(myRTC.hours==8 && myRTC.minutes==0)
    sysOn=true;
}

void irRemoteControl(){
   //Serial.println("in");
  if (irrecv.decode(&results)){
    Serial.println(results.value, HEX);
    delay(1000);
        if(results.value==0xFFE21D){ //(0,2) increase off minute by one
          offMinutes=offMinutes+1;
          if(offMinutes>=60){
            increaseHr();
          }
          updateOffTime();
        }
        if(results.value==0xFFA25D){ //(0,0) decrease off minute by one
          if((offMinutes-1)==0 && offhours==0 ){ 
              //no changes in offminutes as we can't have zero offtime
          }
          else{ 
            offMinutes=offMinutes-1; //change
            if(offMinutes<0)
              decreaseHr();
          }
          updateOffTime();
        }
        if(results.value==0xFF22DD){ //(1,0) decrease off minute by ten
          if((offMinutes-10)==0 && offhours==0 ){ 
              //no changes in offminutes as we can't have zero offtime
          }
          else{ 
            offMinutes=offMinutes-10; //change
            if(offMinutes<0)
              decreaseHr();
          }
          updateOffTime();
        }
        if(results.value==0xFF02FD){ //(1,1) increase off minute by ten
          offMinutes=offMinutes+10;
          if(offMinutes>=60){
            increaseHr();
          }
          updateOffTime();
        }
        if(results.value==0xFFA857){//(2,1)
          onSeconds=onSeconds+30;
          if(onSeconds>=60){
            onMinutes+=onSeconds/60;
            onSeconds=onSeconds%60;
          }
          updateOnTime();
        }
        if(results.value==0xFFE01F){ //(2,0)
          if(!(onMinutes==0&&(onSeconds-30)==0)){
            onSeconds=onSeconds-30;
            if(onSeconds<0&&
            onMinutes>0){
              onMinutes=onMinutes-1;
              onSeconds=30;
            }
            updateOnTime();
          }
        }
        if(results.value==0xFFC23D){ //(1,2)
          sysOn=!sysOn;
          if(sysOn){
            updateOffTime();
            updateOnTime();
          }
          EEPROM.update(3, sysOn);
        }
        if(results.value==0xFF906F){ //(2,2)
          lcdLight=!lcdLight;
        }
        irrecv.resume();
  }        
}

void increaseHr(){
  offhours++;
  if(offhours>23)
    offhours=0;
  offMinutes=offMinutes-60; 
}

void decreaseHr(){
  offhours--;
  if(offhours<0)
    offhours=23;
  offMinutes=60+offMinutes;//change 
}

void updateOnTime(){
  myRTC.updateTime();
  nextOffSecs = onSeconds; 
  nextOffmins = nextOnMins+onMinutes;
  nextOffHrs = nextOnHrs;
    if(nextOffmins>=60){
      nextOffmins=nextOffmins-60;
      nextOffHrs++;
    }
    if(nextOffHrs>23)
      nextOffHrs=nextOffHrs-24;
    EEPROM.update(0,onMinutes ); 
    EEPROM.update(4,onSeconds ); 
    lcd.clear();
    displaySetTimeLCD();        
}

void updateOffTime(){
  myRTC.updateTime();
  nextOnMins= myRTC.minutes+offMinutes;
  nextOnHrs= myRTC.hours+offhours;
    if(nextOnMins>=60){
      nextOnMins=nextOnMins-60;
      nextOnHrs++;
    }
    if(nextOnHrs>23)
      nextOnHrs=nextOnHrs-24;
    EEPROM.update(1, offMinutes);
    EEPROM.update(2, offhours);
    updateOnTime();
    lcd.clear();
    displaySetTimeLCD();
}

void displaySetTimeLCD(){
  if(sysOn){
    displayCurrentTimeLCD();
    lcd.setCursor(0,1);
    if(nextOnHrs<10) lcd.print("0");
    lcd.print((byte)nextOnHrs); 
    lcd.print(":"); 
    if(nextOnMins<10) lcd.print("0");
    lcd.print((byte)nextOnMins); 
    lcd.print("-");
    if(offhours<10) lcd.print("0");
    lcd.print((byte)offhours);
    lcd.print(":");
    if(offMinutes<10) lcd.print("0");
    lcd.print((byte)offMinutes);
    lcd.print("-");
    if(onMinutes<10) lcd.print("0");
    lcd.print((byte)onMinutes);
    if(onSeconds<10) lcd.print("0");
    lcd.print((byte)onSeconds);
    Serial.println(nextOffmins);
  }
}

void displayCurrentTimeLCD(){
  myRTC.updateTime();
  lcd.setCursor(0,0);
  if(myRTC.hours<10) lcd.print("0");
  lcd.print((byte)myRTC.hours); 
  lcd.print(":"); 
  if(myRTC.minutes<10) lcd.print("0");
  lcd.print((byte)myRTC.minutes);
  lcd.print(":"); 
  if(myRTC.seconds<10) lcd.print("0");
  lcd.print((byte)myRTC.seconds);
  
}
