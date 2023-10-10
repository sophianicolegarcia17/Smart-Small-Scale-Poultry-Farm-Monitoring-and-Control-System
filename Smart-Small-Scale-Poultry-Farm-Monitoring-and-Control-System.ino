#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "time.h"
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>

#define BLYNK_TEMPLATE_ID "TMPL6tAf5ta_Y"
#define BLYNK_TEMPLATE_NAME "ESP32"
#define BLYNK_AUTH_TOKEN "6xVWUJpdG5D27_MRwup524F9saiFyMJX"
#define AUTH_TOKEN "6xVWUJpdG5D27_MRwup524F9saiFyMJX"    // Blynk authorization token
//#define WIFI_SSID "Sophia"
//#define WIFI_PASS "nicolegarcia17"


#define DHTPIN 5          // DHT22 data pin
#define DHTTYPE DHT22     // DHT22 sensor type
DHT dht(DHTPIN, DHTTYPE);

#define MQPIN 34          // MQ137 signal pin

#define RelayPin1 27 //heater    
#define RelayPin2 26 //fan
#define RelayPin3 25 //humidifier    
#define RelayPin4 33 //exhaust
#define RelayPin5 32 //bulb

//ultrasonic1
#define TRIGPIN1 18
#define ECHOPIN1 19

//ultrasonic2
#define TRIGPIN2 23
#define ECHOPIN2 22

#define VPIN_BUTTON_8    V8 //heater
#define VPIN_BUTTON_9    V9 //fan
#define VPIN_BUTTON_10   V10 //humidifier
#define VPIN_BUTTON_11   V11 //exhaust
#define VPIN_BUTTON_12   V12 //bulb
#define VPIN_BUTTON_13   V13 //servo

#define FULL_DISTANCE_CM 26 // full distance of water and feed storage in cm

#define V14_TIME_DISPLAY V14 //Time Label Display

static const int servoPin = 14; // printed G14 on the board
Servo servo1;

bool toggleState_1 = LOW; //Define integer to remember the toggle state for relay 1
bool toggleState_2 = LOW; //Define integer to remember the toggle state for relay 2
bool toggleState_3 = LOW; //Define integer to remember the toggle state for relay 3
bool toggleState_4 = LOW; //Define integer to remember the toggle state for relay 4
bool toggleState_5 = LOW; //Define integer to remember the toggle state for relay 5

bool toggleState_6 = LOW; // servo motor

BlynkTimer timer;
int days;
float temp;
float humid;
float mq137;
float nh3ppm;

int hour;
int minute;
int aver[3];

long duration1, dist1, duration2, dist2;

float maxtempThreshold, mintempThreshold, maxhumidityThreshold, minhumidityThreshold;  // Variables to hold the threshold values

int flag=0;

void checkAmmonia() {
  if (nh3ppm >= 150 && flag == 0) {
    Blynk.logEvent("ammonia_notification","Ammonia level in beyond the maximum threshold!");
    flag=1;   

  } else {
    flag=0;    
  }
  
}

BLYNK_WRITE(V0){  // Blynk virtual pin to receive the number of days input
  days = param.asInt();  // Get the number of days from Blynk app
}

//heater
BLYNK_WRITE(VPIN_BUTTON_8) {
  toggleState_1 = param.asInt();
  if(toggleState_1 == 1){
    digitalWrite(RelayPin1, LOW);
  }
  else { 
    digitalWrite(RelayPin1, HIGH);
  }
}
//fan
BLYNK_WRITE(VPIN_BUTTON_9) {
  toggleState_2 = param.asInt();
  if(toggleState_2 == 1){
    digitalWrite(RelayPin2, LOW);
  }
  else { 
    digitalWrite(RelayPin2, HIGH);
  }
}
//humidifier
BLYNK_WRITE(VPIN_BUTTON_10) {
  toggleState_3 = param.asInt();
  if(toggleState_3 == 1){
    digitalWrite(RelayPin3, LOW);
  }
  else { 
    digitalWrite(RelayPin3, HIGH);
  }
}
//exhaust fan
BLYNK_WRITE(VPIN_BUTTON_11) {
  toggleState_4 = param.asInt();
  if(toggleState_4 == 1){
    digitalWrite(RelayPin4, LOW);
  }
  else { 
    digitalWrite(RelayPin4, HIGH);
  }
} 

//bulb
BLYNK_WRITE(VPIN_BUTTON_12) {
  toggleState_5 = param.asInt();
  if(toggleState_5 == 1){
    digitalWrite(RelayPin5, LOW);
  }
  else { 
    digitalWrite(RelayPin5, HIGH);
  }
} 

//servo
BLYNK_WRITE(VPIN_BUTTON_13) {
  toggleState_6 = param.asInt();
  if(toggleState_6 == 1){
    servo1.write(180);
    delay(5000);
    servo1.write(0);
    Blynk.virtualWrite(VPIN_BUTTON_13, LOW); // Update Blynk button state
  }
  else { 
    servo1.write(0);
  }
} 

void setTimezone(String timezone){
  Serial.printf("Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  // Now adjust the TZ. Clock settings are adjusted to show the new local time
  tzset();
}
void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println("Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  timeinfo.tm_hour += 8;
  mktime(&timeinfo); // Normalize the time structure
  
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

// Update the LCD widget with the current hour and minute
  char time_str[16];
  sprintf(time_str, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

  time_t now;
  char timeString[21];
  
  time(&now);
  strftime(timeString, sizeof(timeString), "%B %d %Y %H:%M:%S", &timeinfo);
  Blynk.virtualWrite(V14, String(timeString));
  
  hour = atoi(&timeString[14]);
  minute = atoi(&timeString[17]);

  Serial.print("Hour: ");
  Serial.println(hour);
  Serial.print("Minute: ");
  Serial.println(minute);

}

void sendThresholdValues(){

  // Calculate the threshold values based on the number of days
  if (days >= 1 && days <= 7) {
    maxtempThreshold = 32;
    mintempThreshold = 30;
    maxhumidityThreshold = 99;
    minhumidityThreshold = 50;
  } else if (days >= 8 && days <= 17) {
    maxtempThreshold = 30;
    mintempThreshold = 26;
    maxhumidityThreshold = 99;
    minhumidityThreshold = 50;
  } else if (days >= 18 && days <= 21) {
    maxtempThreshold =26;
    mintempThreshold = 23;
    maxhumidityThreshold = 50;
    minhumidityThreshold = 1;
  } else if (days >= 22 && days <= 29) {
    maxtempThreshold = 23;
    mintempThreshold = 20;
    maxhumidityThreshold = 50;
    minhumidityThreshold = 10;
  } else if (days >= 30 && days <= 45) {
    maxtempThreshold = 20;
    mintempThreshold = 10;
    maxhumidityThreshold = 50;
    minhumidityThreshold = 10;
  } else {
    maxtempThreshold = 0;
    mintempThreshold = 0;
    maxhumidityThreshold = 0;
    minhumidityThreshold = 0;
  }

  // Send the threshold values to Blynk app
  Blynk.virtualWrite(V4, maxtempThreshold);
  Blynk.virtualWrite(V5, mintempThreshold);
  Blynk.virtualWrite(V6, maxhumidityThreshold);
  Blynk.virtualWrite(V7, minhumidityThreshold);

}
void updateRelay(){
  
// Heater and fan
if (temp < mintempThreshold) {
    digitalWrite(RelayPin1, LOW);  // Turn on heater
    Blynk.virtualWrite(VPIN_BUTTON_8, HIGH); // Update Blynk button state
    digitalWrite(RelayPin2, HIGH); // Turn off fan
    Blynk.virtualWrite(VPIN_BUTTON_9, LOW); // Update Blynk button state

}
else if (temp > maxtempThreshold) {
    digitalWrite(RelayPin1, HIGH); // Turn off heater
    Blynk.virtualWrite(VPIN_BUTTON_8, LOW); // Update Blynk button state
    digitalWrite(RelayPin2, LOW);  // Turn on fan
    Blynk.virtualWrite(VPIN_BUTTON_9, HIGH); // Update Blynk button state
}
else {
    digitalWrite(RelayPin1, HIGH); // Turn off heater
    digitalWrite(RelayPin2, HIGH); // Turn off fan
    Blynk.virtualWrite(VPIN_BUTTON_8, LOW); // Update Blynk button state
    Blynk.virtualWrite(VPIN_BUTTON_9, LOW); // Update Blynk button state
}

// Humidifier and exhaust fan
if (humid < minhumidityThreshold) {
    digitalWrite(RelayPin3, LOW);  // Turn on humidifier
    Blynk.virtualWrite(VPIN_BUTTON_10, HIGH); // Update Blynk button state
    digitalWrite(RelayPin4, HIGH); // Turn off exhaust fan
    Blynk.virtualWrite(VPIN_BUTTON_11, LOW); // Update Blynk button state
}
else if (humid > maxhumidityThreshold) {
    digitalWrite(RelayPin3, HIGH); // Turn off humidifier
    Blynk.virtualWrite(VPIN_BUTTON_10, LOW); // Update Blynk button state
    digitalWrite(RelayPin4, LOW);  // Turn on exhaust fan
    Blynk.virtualWrite(VPIN_BUTTON_11, HIGH); // Update Blynk button state
}
else {
    digitalWrite(RelayPin3, HIGH); // Turn off humidifier
    digitalWrite(RelayPin4, HIGH); // Turn off exhaust fan
    Blynk.virtualWrite(VPIN_BUTTON_10, LOW); // Update Blynk button state
    Blynk.virtualWrite(VPIN_BUTTON_11, LOW); // Update Blynk button state
}
}

void sendSensorData() {
  Blynk.run();
  temp = dht.readTemperature();
  humid = dht.readHumidity();
  float mq137 = analogRead(MQPIN);
  float voltage = mq137 / 4095.0 * 3.3; // convert analog value to voltage
  float rs = (3.3 - voltage) / voltage * 10000.0; // calculate resistance of sensor
  float ratio = rs / 10000.0; // calculate ratio of sensor resistance to Ro
  nh3ppm = pow(ratio, -2.68) * 103788.0; // calculate NH3 concentration in ppm
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" *C\tHumidity: ");
  Serial.print(humid);
  Serial.print(" %\tMQ137: ");
  Serial.print(mq137);
  Serial.print("\tNH3: ");
  Serial.print(nh3ppm);
  Serial.println(" ppm");
  Blynk.virtualWrite(V1, temp);
  Blynk.virtualWrite(V2, humid);
  Blynk.virtualWrite(V3, nh3ppm);
  delay(1000);

}

void setup() {
  Serial.begin(9600);
  servo1.attach(servoPin);

  timer.setInterval(2500L, checkAmmonia);

  initTime("Asia/Manila"); // Set timezone to Asia/Manila

  pinMode(TRIGPIN1, OUTPUT);
  pinMode(ECHOPIN1, INPUT);
  pinMode(TRIGPIN2, OUTPUT);
  pinMode(ECHOPIN2, INPUT);

  dht.begin();
  Blynk.begin(AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
  timer.setInterval(1000L, sendSensorData); // send sensor data every 1 second
  timer.setInterval(1000L, sendThresholdValues);  // Call sendThresholdValues function every 1 second
  timer.setInterval(1000L, updateRelay);

  pinMode(RelayPin1, OUTPUT);
  pinMode(RelayPin2, OUTPUT);
  pinMode(RelayPin3, OUTPUT);
  pinMode(RelayPin4, OUTPUT);
  pinMode(RelayPin5, OUTPUT);

  //During Starting all Relays should TURN OFF
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);
  digitalWrite(RelayPin3, HIGH);
  digitalWrite(RelayPin4, HIGH);
  digitalWrite(RelayPin5, HIGH);

  Blynk.virtualWrite(VPIN_BUTTON_8, toggleState_1);
  Blynk.virtualWrite(VPIN_BUTTON_9, toggleState_2);
  Blynk.virtualWrite(VPIN_BUTTON_10, toggleState_3);
  Blynk.virtualWrite(VPIN_BUTTON_11, toggleState_4);
  Blynk.virtualWrite(VPIN_BUTTON_12, toggleState_5);
  Blynk.virtualWrite(VPIN_BUTTON_13, toggleState_6);

}

void measure() {  //detect outside movement
  digitalWrite(TRIGPIN1, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN1, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN1, LOW);

  duration1 = pulseIn(ECHOPIN1, HIGH);
  dist1 = (duration1*.0343)/2;
}
void monitor() {  //detect trash level
  digitalWrite(TRIGPIN2, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN2, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN2, LOW);

  duration2 = pulseIn(ECHOPIN2, HIGH);
  dist2 = (duration2*.0343)/2;
}

void loop() {
  servo1.write(0);
  printLocalTime();
  Blynk.run();
  timer.run();

  for (int i=0;i<=2;i++) {   //average distance
    measure();               
   aver[i]=dist1;            
    delay(10);              //delay between measurements
  }
 dist1=((aver[0]+aver[1]+aver[2])/3);   

 for (int i=0;i<=2;i++) {   //average distance
    monitor();               
   aver[i]=dist2;            
    delay(10);              //delay between measurements
  }
 dist2=((aver[0]+aver[1]+aver[2])/3);

  // calculate the distance as a percentage of the full distance
  int percentage1 = ((FULL_DISTANCE_CM - dist1) * 100) / FULL_DISTANCE_CM;
  int percentage2 = ((FULL_DISTANCE_CM - dist2) * 100) / FULL_DISTANCE_CM;

  Serial.print("Distance 1: ");
  Serial.print(dist1);
  Serial.print("cm, Percentage 1: ");
  Serial.print(percentage1);
  Serial.println("%");

  Serial.print("Distance 2: ");
  Serial.print(dist2);
  Serial.print("cm, Percentage 2: ");
  Serial.print(percentage2);
  Serial.println("%");

//display levels on blynk
  Blynk.virtualWrite(V15, percentage1);
  Blynk.virtualWrite(V16, percentage2);

}