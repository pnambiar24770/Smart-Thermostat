#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "MQ135.h"
#include "DHT20.h"
#include <pgmspace.h>
#include <TimeLib.h>




#define SENSOR 32
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"
 
#define SECRET
#define THINGNAME "smart_temp_monitor"                         //change this
 
const char WIFI_SSID[] = "";               //change this
const char WIFI_PASSWORD[] = "";           //change this
const char AWS_IOT_ENDPOINT[] = "";       //change this
 
// Amazon Root CA 1
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
 -----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";
 
// Device Certificate                                               //change this
static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----

 
 
)KEY";
 
// Device Private Key                                               //change this
static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
 
 
)KEY";
 
MQ135 gasSensor = MQ135(SENSOR);
int t;
int h;
int c;
int uptime=0;
char TimeString[9];




DHT20 dht;

 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
 
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.setServer(AWS_IOT_ENDPOINT, 8883);
 
  // Create a message handler
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
 
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
 
  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}
 
void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humidity"] = String(h);
  doc["temperature"] = String(t);
  doc["co2"] = String(c);
  doc["uptime"]=TimeString;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}
 
 
void setup()
{
  Serial.begin(9600);
  connectAWS();
  dht.begin();
  setTime(initialHour, initialMinute, initialSecond , initialDay , initialMonth , initialYear);
  pinMode(12,OUTPUT);

  delay(1000);
}
 
void loop()
{
  int status = dht.read();
   h=dht.getHumidity();
   t=((dht.getTemperature()*9/5)+32);
   c=gasSensor.getPPM();
   uptime++;
   // Get the current time
  time_t n = now();

 sprintf(TimeString, "%02d:%02d:%02d", hour(n), minute(n), second(n));

 
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.println("Humidity: "+h);
  Serial.println("Temp: "+t);
  Serial.println("CO2: "+c);
  Serial.println(TimeString);
  Serial.println("--------------------------");
 
  publishMessage();
  client.loop();

  if(c>40){
  tone(12,1000);
  delay(1000);
  noTone(12);
  }

  delay(1000);
}
