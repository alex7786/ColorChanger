#include <PwmRead.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

const int pwmReadPin = A6;

PwmRead pwmRead(pwmReadPin);
Servo colorLid;
int pos = 0;    // variable to store the servo position

Adafruit_SSD1306 display(128, 32, &Wire, -1);

void setup() {
  Serial.begin(9600);
  
  colorLid.attach(5); //servo pin

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
}

void loop() {
  //read value every 1000ms and make sure it is consistent within 10ms
  int readValue = pwmRead.ReadVal(10, 1000);

  //ReadVal will return -1 if value was not consistent in given timeframe
  if(readValue != -1) 
  {
    Serial.println(readValue);

    //show color number on display
    DrawColor(readValue);
  }

  //ServoTest();

  
}

void ServoTest(){
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    colorLid.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    colorLid.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
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
  delay(2000);
}

int ConvertCode437Number(int singleDigitNumber){
  if(singleDigitNumber >= 0 && singleDigitNumber <= 9){
    return singleDigitNumber + 48;  //offset is 48 in ASCII code 437
  }
  else{
    return 63;  //return ? if number not in range 
  }
}
