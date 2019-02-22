#include <EEPROM.h>
#include "bsec.h"
#include "bsec_serialized_configurations_iaq.h"
#include <LowPower.h>

#define STATE_SAVE_PERIOD  UINT32_C(360 * 60 * 1000) // 360 minutes - 4 times a day

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void loadState(void);
void updateState(void);

// Create an object of the class Bsec
Bsec iaqSensor;
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = {0};
uint16_t stateUpdateCounter = 0;
uint32_t millisOverflowCounter = 0;
uint32_t lastTime = 0;

String output;

bool result() {
  char c;
  do{
   c = Serial2.read();
   if(c==0xFF){
    printf("Error!");
    return false;
   }else{
     printf("%c", c);
   }
  }while(c!='\r');    

  // for LF
  c = Serial2.read();
  printf("%c", c);

  return true;
}

// Entry point for the example
void setup(void)
{
//  EEPROM.begin(BSEC_MAX_STATE_BLOB_SIZE + 1); // 1st address for the length
  EEPROM.begin(); // 1st address for the length
  Serial.begin(115200);

  // Initialize LowPower library
  LowPower.begin();
  // Clock mode: HV -> RCOSC
  LowPower.clockMode(CLOCK_MODE_LOW);

  /* Setup button interrupt to trigger ULP plus */
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), ulp_plus_button_press, FALLING);

  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  iaqSensor.setConfig(bsec_config_iaq);
  checkIaqSensorStatus();
/*-----------------*/
  // Configure baud rate to 9600
  Serial2.begin(9600);

  // Configure reset pin
  pinMode(PIN_D26, OUTPUT);

  // Do reset
  digitalWrite(PIN_D26, LOW);
  delay(100);

  // Release reset
  digitalWrite(PIN_D26, HIGH);
  delay(100);
  result();

/*-----------------*/

  loadState();

  bsec_virtual_sensor_t sensorList[7] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 7, BSEC_SAMPLE_RATE_ULP);
  checkIaqSensorStatus();

  // Print the header
//  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%]";
  output = "Timestamp [ms], pressure [hPa], IAQ, IAQ accuracy, temperature [°C], relative humidity [%]";
  Serial.println(output);
}

void send_sigfox(int16_t i0,int16_t i1,int16_t i2,int16_t i3,int16_t i4,int16_t i5)
{
  String output2;
  char val[24];
  Serial2.write("0x00");
  sleep(1);

  puts("send");
  Serial2.write("AT$SF=");
  sprintf(val,"%04x%04x%04x%04x%04x%04x", i0,i1,i2,i3,i4,i5);
  output2 = String(val);
  Serial.println(output2);
  Serial2.println(output2);

  sleep(10);
  result();
  puts("end");
  
}

void send_sigfox(float f0,float f1,float f2)
{
  union {float f; int i;} data;
  String output2;

  // wakeup.
  Serial2.write("0x00");
  sleep(1);

  puts("send");
  Serial2.write("AT$SF=");
  data.f = f0;
  output2 = String(data.i, HEX);
  data.f = f1;
  output2 += String(data.i, HEX);
  data.f = f2;
  output2 += String(data.i, HEX);
  Serial.println(output2);
  Serial2.println(output2);

//  Serial2.write("\r");
  sleep(10);
  result();
//  sleep(10);
  puts("end");
}

// Function that is looped forever
void loop(void)
{

  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available

    // Get voltage
    int vol = LowPower.getVoltage();
    float volf = (float)vol / 1000.0f;
    Serial.print("voltage[V]= ");
    Serial.println(volf);

    output = String(time_trigger);
//    output += ", " + String(iaqSensor.rawTemperature);
    output += ", " + String(iaqSensor.pressure);
//    output += ", " + String(iaqSensor.rawHumidity);
//    output += ", " + String(iaqSensor.gasResistance);
    output += ", " + String(iaqSensor.iaqEstimate);
    output += ", " + String(iaqSensor.iaqAccuracy);
    output += ", " + String(iaqSensor.temperature);
    output += ", " + String(iaqSensor.humidity);
    Serial.println(output);

/*    send_sigfox(iaqSensor.pressure);
    send_sigfox(iaqSensor.iaqEstimate);
    send_sigfox(iaqSensor.temperature);
    send_sigfox(iaqSensor.humidity);
    send_sigfox(iaqSensor.pressure);*/
//    send_sigfox(iaqSensor.temperature,iaqSensor.humidity,iaqSensor.iaqEstimate);

    send_sigfox((int16_t)(iaqSensor.temperature*100),
                (int16_t)(iaqSensor.humidity*100),
                (int16_t)(iaqSensor.pressure/100),
                (int16_t)(iaqSensor.iaqEstimate),
                (int16_t)(iaqSensor.iaqAccuracy),
                mvolt);
                
    updateState();
  } else {
    checkIaqSensorStatus();
  }
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      for (;;)
        errLeds(); /* Halt in case of failure */
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

void errLeds(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
}

void loadState(void)
{
  if (EEPROM.read(0) == BSEC_MAX_STATE_BLOB_SIZE) {
    // Existing state in EEPROM
    Serial.println("Reading state from EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++) {
      bsecState[i] = EEPROM.read(i + 1);
      Serial.println(bsecState[i], HEX);
    }

    iaqSensor.setState(bsecState);
    checkIaqSensorStatus();
  } else {
    // Erase the EEPROM with zeroes
    Serial.println("Erasing EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE + 1; i++)
      EEPROM.write(i, 0);

//    EEPROM.commit();
  }
}

void updateState(void)
{
  bool update = false;
  if (stateUpdateCounter == 0) {
    /* First state update when IAQ accuracy is >= 3 */
    if (iaqSensor.iaqAccuracy >= 3) {
      update = true;
      stateUpdateCounter++;
    }
  } else {
    /* Update every STATE_SAVE_PERIOD minutes */
    if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
      update = true;
      stateUpdateCounter++;
    }
  }

  if (update) {
    iaqSensor.getState(bsecState);
    checkIaqSensorStatus();

    Serial.println("Writing state to EEPROM");

    for (uint8_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE ; i++) {
      EEPROM.write(i + 1, bsecState[i]);
      Serial.println(bsecState[i], HEX);
    }

    EEPROM.write(0, BSEC_MAX_STATE_BLOB_SIZE);
//    EEPROM.commit();
  }
}

/*!
   @brief       Interrupt handler for press of a ULP plus button

   @return      none
*/
void ulp_plus_button_press()
{
  /* We call bsec_update_subscription() in order to instruct BSEC to perform an extra measurement at the next
     possible time slot
  */
  Serial.println("Triggering ULP plus.");
  bsec_virtual_sensor_t sensorList[1] = {
    BSEC_OUTPUT_IAQ,
  };

  iaqSensor.updateSubscription(sensorList, 1, BSEC_SAMPLE_RATE_ULP_MEASUREMENT_ON_DEMAND);
  checkIaqSensorStatus();
}
