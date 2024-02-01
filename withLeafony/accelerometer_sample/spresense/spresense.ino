#include <SoftwareSerial.h>
#include <BMI160Gen.h>

#define bootPin 7

#define rxPin 1
#define txPin 0

SoftwareSerial mySerial =  SoftwareSerial(rxPin, txPin);

void setup()
{
  pinMode(bootPin, INPUT);

  Serial.begin(9600);
  Serial.println("Spresense booted!");

  sleep(1);
  while(digitalRead(bootPin) == HIGH);

  BMI160.begin();

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  mySerial.begin(9600);
}

void loop() {
  float ax, ay, az;   //scaled accelerometer values

  // read accelerometer measurements from device, scaled to the configured range
  BMI160.readAccelerometerScaled(ax, ay, az);

  // display tab-separated accelerometer x/y/z values
  mySerial.print("a:\t");
  mySerial.print(ax);
  mySerial.print("\t");
  mySerial.print(ay);
  mySerial.print("\t");
  mySerial.print(az);
  mySerial.println();

  sleep(1);

}
