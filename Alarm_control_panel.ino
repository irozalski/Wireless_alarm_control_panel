#include "BluetoothSerial.h"

#define timeSeconds 10

BluetoothSerial SerialBT;

const int ledPin = 2;
const int ledPin2 = 4;
const int motionSensor = 27;
const int buzzer = 26;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
unsigned long alarmTrigger = 0;
boolean startTimer = false;
boolean motion = false;

boolean sensor_armed = true;
// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  if(sensor_armed){
  digitalWrite(ledPin, HIGH);
  startTimer = true;
  lastTrigger = millis();
  }
}

//Password holder
String data;

String code = "arm";
//password
String password = "1234";

void setup() {
  SerialBT.begin("ESP32-Bluetooth");
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(ledPin2, OUTPUT);
  digitalWrite(ledPin2, LOW);
  
  pinMode(buzzer, OUTPUT);
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

}

void loop() {

  now = millis();
  
  if((digitalRead(ledPin) == HIGH) && (motion == false)) {
    Serial.println("MOTION DETECTED!!!");
    alarmTrigger = millis();
    motion = true;
  }
  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("Motion stopped...");
    digitalWrite(ledPin, LOW);
    Serial.println("ALARM!!!");
    startTimer = false;
    motion = false;
  }
  if(startTimer && (now - alarmTrigger > (timeSeconds*1000))) {
    digitalWrite(buzzer, HIGH);
  }
  
  if(!sensor_armed){
    digitalWrite(buzzer, LOW);
    digitalWrite(ledPin, LOW);
  }
  
  if (SerialBT.available()) {
    //Read Password
    while (SerialBT.available()) {
      char c = (char)SerialBT.read();
      data += c;
    }
   }
   if (data == code) {
      sensor_armed = true;
      SerialBT.println("Alarm armed");
      Serial.println("armed");
      delay(5000);
   }
   if (data == password) {
      sensor_armed = false;
      Serial.println("Sensor disarmed");
      // Correct Password
      SerialBT.println("Alarm disarmed");
      // Activate LED
      digitalWrite(ledPin2, HIGH);
      delay(3000);
      digitalWrite(ledPin2, LOW);
    } 
    else {
      // Incorrect Password
      SerialBT.println("Wrong Password");
      delay(1000);
    }

    //Reset
    data = "";

}
