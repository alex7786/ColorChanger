#include <Arduino.h>
#include <PwmRead.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "BasicStepperDriver.h"

//definitions for stepper motor
#define MOTOR_STEPS 200 // Motor steps per revolution
#define RPM 120
#define MICROSTEPS 1  // 1=full step, 2=half step etc.
#define DIR 3
#define STEP 4

//other pin definitions
const int pwmReadPin = A6;
const int indexSensorPin = 7;
const int buttonRightPin = 8;
const int buttonLeftPin = 9;
const int toggleLidButtonPin = 10;
const int autoManualSwitchPin = 11;
const int homingButtonPin = 12;
const int homedLED = 6;
const int lidOpenLED = 2;

//program constants
const int maxColorNumber = 62;

//program variables
//color position angle definition
double colorAngle[maxColorNumber][2] = {{1,2},{2,40},{3,4},{4,40},{5,14},{6,4},{7,4},{8,104},{9,14},{10,4},{11,74},{12,4},{13,54},{14,4},{15,47},
                         {16,2},{17,4},{18,46},{19,4},{20,4},{21,84},{22,4},{23,140},{24,4},{25,4},{26,54},{27,4},{28,4},{29,74},{30,4},
                         {31,2},{32,45},{33,4},{34,40},{35,4},{36,45},{37,4},{38,4},{39,104},{40,4},{41,4},{42,204},{43,41},{44,4},{45,4},
                         {46,2},{47,42},{48,4},{49,240},{50,4},{51,4},{52,359},{53,4},{54,4},{55,45},{56,4},{57,4},{58,360},{59,4},{60,4},
                         {61,270},{62,4}};

//stepper
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP);
double platformAngle = -1;  //variable to store platform angle
bool platformHomed = false;
bool positioning = false;
//color / pwm reading
PwmRead pwmRead(pwmReadPin);
int colorSelected = 1;  //initialize with first color
Adafruit_SSD1306 display(128, 32, &Wire, -1);
//servo
Servo colorLid;
int servoPos = 0;    //variable to store the servo position
bool lidOpen = false;

void setup() {
  Serial.begin(9600);
  stepper.begin(RPM, MICROSTEPS);
  colorLid.attach(5); //servo pin
  pinMode(indexSensorPin, INPUT);
  pinMode(buttonLeftPin, INPUT_PULLUP);
  pinMode(buttonRightPin, INPUT_PULLUP);
  pinMode(autoManualSwitchPin, INPUT_PULLUP);
  pinMode(homingButtonPin, INPUT_PULLUP);
  pinMode(toggleLidButtonPin, INPUT_PULLUP);
  pinMode(homedLED, OUTPUT);
  pinMode(lidOpenLED, OUTPUT);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
}

void loop() {
  ColorSelect();  

  if(!digitalRead(homingButtonPin)){
    HomeColorWheel();
  }

  if(!digitalRead(toggleLidButtonPin)){
    ToggleLid();
  }

  MovePlatformToAngle(colorAngle[colorSelected][1]); //test with color value

  //debug serial prints
  Serial.print("indexSensorPin:");
  Serial.println(digitalRead(indexSensorPin));
  Serial.print("buttonLeftPin:");
  Serial.println(digitalRead(buttonLeftPin));
  Serial.print("buttonRightPin:");
  Serial.println(digitalRead(buttonRightPin));
  Serial.print("autoManualSwitchPin:");
  Serial.println(digitalRead(autoManualSwitchPin));
  Serial.print("homingButtonPin:");
  Serial.println(digitalRead(homingButtonPin));
  Serial.print("platformAngle:");
  Serial.println(platformAngle);
  Serial.print("positioning:");
  Serial.println(positioning);
  Serial.print("colorSelected:");
  Serial.println(colorSelected);
  Serial.print("colorAngle[colorSelected][1]:");
  Serial.println(colorAngle[colorSelected][1]);

  //setting outputs
  if(lidOpen){
    digitalWrite(lidOpenLED, HIGH);
  }
  else{
    digitalWrite(lidOpenLED, LOW);
  }

  if(platformHomed){
    digitalWrite(homedLED, HIGH);
  }
  else{
    digitalWrite(homedLED, LOW);
  }
}

void StepperTest(){
    // energize coils - the motor will hold position
    stepper.enable();
  
    //Moving motor one full revolution using the degree notation
    stepper.rotate(360);

    //Moving motor to original position using steps
    stepper.move(-MOTOR_STEPS*MICROSTEPS);

    // pause and allow the motor to be moved by hand
    stepper.disable();

    delay(5000);
}

void MovePlatformToAngle(double anglePos){
  //200 steps per motor revolution
  //1 motor revolution = 19.6121° rotation of the platform
  //1 motor step = 0.09806046854° rotation of the platform
  //platformAngle -> current position
  const double degreePerStep = 0.09806046854;
  if(anglePos != platformAngle && platformHomed && !positioning){//check if already in position and if homed
    positioning = true;
    double rotateSteps = (anglePos - platformAngle)/degreePerStep;
    stepper.enable();
    stepper.move(rotateSteps*MICROSTEPS);
    stepper.disable();
    platformAngle = anglePos;
    positioning = false;
  }
}

void HomeColorWheel(){
  platformHomed = false;
  stepper.enable();
  while(digitalRead(indexSensorPin) != true){
    digitalWrite(homedLED, HIGH);
    stepper.move(10*MICROSTEPS);
    delay(10);
    digitalWrite(homedLED, LOW);
    delay(10);
  }
  stepper.disable();
  platformAngle = 0;
  platformHomed = true;
}

void ColorSelect(){
    if(digitalRead(autoManualSwitchPin)){
      //read value every 1000ms and make sure it is consistent within 10ms
      int readValue = pwmRead.ReadVal(10, 1000);
    
      //ReadVal will return -1 if value was not consistent in given timeframe
      if(readValue != -1) 
      {
        Serial.print("readval:");
        Serial.println(readValue);
    
        colorSelected = readValue;
      }
    }
    else{
      if(!digitalRead(buttonRightPin)){
        colorSelected++;
        if(colorSelected > maxColorNumber){
          colorSelected = 1;
        }
        delay(200);
      }
      else if(!digitalRead(buttonLeftPin)){
        colorSelected--;
        if(colorSelected < 1){
          colorSelected = maxColorNumber;
        }
        delay(200);
      }
    }
    
    //show color number on display
    DrawColor(colorSelected);
}

void ToggleLid(){
  if(lidOpen){
    CloseLid();
  }
  else{
    OpenLid();
  }
}

void OpenLid(){
  if(servoPos < 180){
    for (servoPos = 0; servoPos <= 180; servoPos += 1) { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      colorLid.write(servoPos);              // tell servo to go to position in variable 'pos'
      delay(5);                       // waits 15ms for the servo to reach the position
    }
  }
  lidOpen = true;
}

void CloseLid(){
  if(servoPos > 0){
    for (servoPos = 180; servoPos >= 0; servoPos -= 1) { // goes from 180 degrees to 0 degrees
      colorLid.write(servoPos);              // tell servo to go to position in variable 'pos'
      delay(5);                       // waits 15ms for the servo to reach the position
    }
  }
  lidOpen = false;
}

void DrawColor(int number) {
  display.clearDisplay();

  int firstDigit = ConvertCode437Number(number / 10);
  int secondDigit = ConvertCode437Number(number% 10);
  
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 10);
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.write("color: ");
  display.write(firstDigit);
  display.write(secondDigit);
  display.write("\n");

  display.display();
}

int ConvertCode437Number(int singleDigitNumber){
  if(singleDigitNumber >= 0 && singleDigitNumber <= 9){
    return singleDigitNumber + 48;  //offset is 48 in ASCII code 437
  }
  else{
    return 63;  //return ? if number not in range 
  }
}
