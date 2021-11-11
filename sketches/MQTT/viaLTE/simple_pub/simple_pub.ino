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
#define MQTT_TOPIC "spresense/mqtt"        // replace with your topic

// MQTT publish interval settings
#define PUBLISH_INTERVAL_SEC   1           // MQTT publish interval in sec
#define MAX_NUMBER_OF_PUBLISH  30          // Maximum number of publish

LTE lteAccess;
LTEClient client;
MqttClient mqttClient(client);

int numOfPubs = 0;
unsigned long lastPubSec = 0;
char broker[] = BROKER_NAME;
int port = BROKER_PORT;
char topic[]  = MQTT_TOPIC;

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

  int result;

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

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }

  Serial.println("You're connected to the MQTT broker!");

}

void loop()
{
  String testString = "test" + String(numOfPubs) + "!";

  unsigned long currentTime = lteAccess.getTime();
  if (currentTime >= lastPubSec + PUBLISH_INTERVAL_SEC) {
    // Publish to broker
    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print("Publish: ");
    Serial.println(testString);

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(testString);
    mqttClient.endMessage();
    lastPubSec = currentTime;
    numOfPubs++;
  }

  if (numOfPubs >= MAX_NUMBER_OF_PUBLISH) {
    Serial.println("Publish end");
    // do nothing forevermore:
    for (;;)
      sleep(1);
  }

}
