#include <LTE.h>
#include <ArduinoMqttClient.h>

// APN name
#define APP_LTE_APN "iijmio.jp" // replace your APN

/* APN authentication settings
 * Ignore these parameters when setting LTE_NET_AUTHTYPE_NONE.
 */
#define APP_LTE_USER_NAME "mio@iij"     // replace with your username
#define APP_LTE_PASSWORD  "iij" // replace with your password

// APN IP type
#define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V4V6) // IP : IPv4v6
// #define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V4) // IP : IPv4
// #define APP_LTE_IP_TYPE (LTE_NET_IPTYPE_V6) // IP : IPv6

// APN authentication type
#define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_CHAP) // Authentication : CHAP
// #define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_PAP) // Authentication : PAP
// #define APP_LTE_AUTH_TYPE (LTE_NET_AUTHTYPE_NONE) // Authentication : NONE

/* RAT to use
 * Refer to the cellular carriers information
 * to find out which RAT your SIM supports.
 * The RAT set on the modem can be checked with LTEModemVerification::getRAT().
 */

#define APP_LTE_RAT (LTE_NET_RAT_CATM) // RAT : LTE-M (LTE Cat-M1)
// #define APP_LTE_RAT (LTE_NET_RAT_NBIOT) // RAT : NB-IoT

// MQTT broker
#define BROKER_NAME "test.mosquitto.org"   // replace with your broker
#define BROKER_PORT 1883                   // port 8883 is the default for MQTT over TLS.
                                           // for this client, if required by the server.

// MQTT topic
#define MQTT_TOPIC0 "spresense/mqtt0"        // replace with your topic
#define MQTT_TOPIC1 "spresense/mqtt1"        // replace with your topic

// MQTT publish interval settings
#define PUBLISH_INTERVAL_SEC   1           // MQTT publish interval in sec
#define MAX_NUMBER_OF_PUBLISH  30          // Maximum number of publish

LTE lteAccess;
LTEClient client;
MqttClient mqttClient0(client);
MqttClient mqttClient1(client);

int numOfPubs = 0;
unsigned long lastPubSec = 0;
char broker[] = BROKER_NAME;
int port = BROKER_PORT;
char topic0[]  = MQTT_TOPIC1;
char topic1[]  = MQTT_TOPIC0;

void doAttach()
{
  while (true) {

    /* Power on the modem and Enable the radio function. */

    if (lteAccess.begin() != LTE_SEARCHING) {
      Serial.println("Could not transition to LTE_SEARCHING.");
      Serial.println("Please check the status of the LTE board.");
      for (;;) {
        sleep(1);
      }
    }

    /* The connection process to the APN will start.
     * If the synchronous parameter is false,
     * the return value will be returned when the connection process is started.
     */
    if (lteAccess.attach(APP_LTE_RAT,
                         APP_LTE_APN,
                         APP_LTE_USER_NAME,
                         APP_LTE_PASSWORD,
                         APP_LTE_AUTH_TYPE,
                         APP_LTE_IP_TYPE,
                         false) == LTE_CONNECTING) {
      Serial.println("Attempting to connect to network.");
      break;
    }

    /* If the following logs occur frequently, one of the following might be a cause:
     * - APN settings are incorrect
     * - SIM is not inserted correctly
     * - If you have specified LTE_NET_RAT_NBIOT for APP_LTE_RAT,
     *   your LTE board may not support it.
     */
    Serial.println("An error has occurred. Shutdown and retry the network attach preparation process after 1 second.");
    lteAccess.shutdown();
    sleep(1);
  }
}

void setup()
{
  // Open serial communications and wait for port to open
  Serial.begin(115200);
  while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Starting .");

  /* Connect LTE network */
  doAttach();

  // Wait for the modem to connect to the LTE network.
  Serial.println("Waiting for successful attach.");
  LTEModemStatus modemStatus = lteAccess.getStatus();

  while(LTE_READY != modemStatus) {
    if (LTE_ERROR == modemStatus) {

      /* If the following logs occur frequently, one of the following might be a cause:
       * - Reject from LTE network
       */
      Serial.println("An error has occurred. Shutdown and retry the network attach process after 1 second.");
      lteAccess.shutdown();
      sleep(1);
      doAttach();
    }
    sleep(1);
    modemStatus = lteAccess.getStatus();
  }

  Serial.println("attach succeeded.");

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient0.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient0.connectError());
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }

  if (!mqttClient1.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient1.connectError());
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }

  mqttClient1.subscribe(topic1);

  Serial.println("You're connected to the MQTT broker!");

}

void sendData(String testString)
{
    Serial.print("Sending message to topic: ");
    Serial.println(topic0);
    Serial.print("Publish: ");
    Serial.println(testString);
    mqttClient0.beginMessage(topic0);
    mqttClient0.print(testString);
    mqttClient0.endMessage();
}

void loop()
{

  if (Serial.available() > 0) {
     switch (Serial.read()) {
        case '1':
          sendData(String(1));
          break;
        case '2':
          sendData(String(2));
          break;
        case '3':
          sendData(String(3));
          break;
        case '4':
          sendData(String(4));
          break;
          
        default:
          break;
      }
    }

  int messageSize = mqttClient1.parseMessage();
  if (messageSize) {
    // we received a message, print out the topic and contents
    Serial.print("Received a message with topic '");
    Serial.print(mqttClient1.messageTopic());
    Serial.print("', length ");
    Serial.print(messageSize);
    Serial.println(" bytes:");

    // use the Stream interface to print the contents
    while (mqttClient1.available()) {
     switch (mqttClient1.read()) {
        case '1':
          ledOn(LED0);
          ledOff(LED1);
          ledOff(LED2);
          ledOff(LED3);
          break;
        case '2':
          ledOff(LED0);
          ledOn(LED1);
          ledOff(LED2);
          ledOff(LED3);
          break;
        case '3':
          ledOff(LED0);
          ledOff(LED1);
          ledOn(LED2);
          ledOff(LED3);
          break;
        case '4':
          ledOff(LED0);
          ledOff(LED1);
          ledOff(LED2);
          ledOn(LED3);
          break;

        default:
          break;
      }
    }
    Serial.println();
  }
}
