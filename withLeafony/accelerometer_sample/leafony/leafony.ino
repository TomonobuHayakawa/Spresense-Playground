#define SprPin 7

void setup()
{
  pinMode(SprPin, OUTPUT);
  digitalWrite(SprPin,LOW);
  Serial.begin(9600);
}

void loop()
{
  while(Serial.available()){
    Serial.write(Serial.read());
  }
  delay(1000);
}
