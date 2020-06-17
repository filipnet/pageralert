#include <Arduino.h>
#include <ESP8266WiFi.h>
#define MQTT_MAX_PACKET_SIZE 256
#include <WiFiClientSecure.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include "credentials.h"
#include "config.h"

const char *hostname = WIFI_HOSTNAME;
const char *ssid = WIFI_SSID;
const char *password =  WIFI_PASSWORD;
const char *mqttServer = MQTT_SERVER;
const int mqttPort = MQTT_PORT;
const char *mqttUser = MQTT_USERNAME;
const char *mqttPassword = MQTT_PASSWORD;
const char *mqttID = MQTT_ID;

unsigned long heartbeat_previousMillis = 0;
const long heartbeat_interval = HEARTBEAT_INTERVALL;

const int ledPin = LED_BUILTIN; // the number of the LED pin
int ledState = HIGH;            // ledState used to set the LED
bool blinkState = true;

const int buttonPin = RELAYCONTACT;
int buttonState = HIGH;    // current state of the button (startup high = no alert)
int lastButtonState = 0;   // previous state of the button

WiFiClientSecure espClient;
PubSubClient client(espClient);
 
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Input with pullup - so you dont need an external resistor
  espClient.setInsecure();
  reconnect();
}

void reconnect() {
  while (!client.connected()) {
    WiFi.mode(WIFI_STA);
	  WiFi.hostname(hostname);
    delay(100);
    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("Connected to WiFi network");
    Serial.print("  SSID: ");
    Serial.print(ssid);
    Serial.print(" / Channel: ");
    Serial.println(WiFi.channel());
    Serial.print("  IP Address: ");
    Serial.print(WiFi.localIP());
    Serial.print(" / Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("  Gateway: ");
    Serial.print(WiFi.gatewayIP());
    Serial.print(" / DNS: ");
    Serial.print(WiFi.dnsIP());
    Serial.print(", ");
    Serial.println(WiFi.dnsIP(1));
    Serial.println("");

    // https://pubsubclient.knolleary.net/api.html
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    Serial.println("Connecting to MQTT broker");
    Serial.print("  MQTT Server: ");
    Serial.println(mqttServer);
    Serial.print("  MQTT Port: ");
    Serial.println(mqttPort);
    Serial.print("  MQTT Username: ");
    Serial.println(mqttUser);
    Serial.print("  MQTT Identifier: ");
    Serial.println(mqttID);
    Serial.println("");

    while (!client.connected()) {
      if (client.connect(mqttID, mqttUser, mqttPassword)) {
        Serial.println("Connected to MQTT broker");
        Serial.println("Subscribe MQTT Topics");
        Serial.println("");
        digitalWrite(LED_BUILTIN, HIGH); 
       } else {
        Serial.print("Connection to MQTT broker failed with state: ");
        Serial.println(client.state());
        char puffer[100];
        espClient.getLastSSLError(puffer,sizeof(puffer));
        Serial.print("TLS connection failed with state: ");
        Serial.println(puffer);
        Serial.println("");
        delay(4000);
       }
    }
  }
}

// Function to receive MQTT messages
void mqttloop() {
  if (!client.loop())
    client.connect(mqttID);
}

// Function to send MQTT messages
void mqttsend(const char *_topic, const char *_data) {
  client.publish(_topic, _data);
}

// Pointer to a message callback function called when a message arrives for a subscription created by this client.
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message topic: ");
  Serial.print(topic);
  Serial.print(" | Message Payload: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

void loop() {
  client.loop();
  reconnect();
  heartbeat();
  handle_alerts();
  mqttloop();
}

void heartbeat() {
  unsigned long heartbeat_currentMillis = millis();
  if (heartbeat_currentMillis - heartbeat_previousMillis >= heartbeat_interval) {
    heartbeat_previousMillis = heartbeat_currentMillis;
    Serial.println("Send heartbeat signal to MQTT broker");
    Serial.println("");
    client.publish("home/pager/alert/heartbeat", "on");
  }
}

void handle_alerts() {
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      Serial.println("Relaycontact: closed (on)");
      Serial.println("Send alertmessage to mqtt-broker. Topic /home/pager/alert");
      client.publish("home/pager/alert", "on");
      delay(100);      
      blinkState = true;
    } else {
      Serial.println("Relaycontact: opened (off)");
      Serial.println("Send off-message to mqtt-broker. Topic /home/pager/alert");
      client.publish("home/pager/alert", "off");
      delay(100);
      blinkState = false;
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
  lastButtonState = buttonState;

  if (blinkState == true) {
    blink(LED_BUILTIN, 250, 5); 
  }
}

// Blink (LED, Frequency, Intervall)
unsigned long blink_intervall_previousMillis = 0; 
unsigned long blink_frequency_previousMillis = 0; 
void blink(const int ledPin, unsigned long blink_frequency, unsigned long blink_intervall) {
  unsigned long blink_intervall_currentMillis = millis();
  if (blink_intervall_currentMillis - blink_intervall_previousMillis >= blink_intervall) {
    blink_intervall_previousMillis = blink_intervall_currentMillis;
    unsigned long blink_frequency_currentMillis = millis();
    if (blink_frequency_currentMillis - blink_frequency_previousMillis >= blink_frequency) {
      blink_frequency_previousMillis = blink_frequency_currentMillis;
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }
      digitalWrite(ledPin, ledState);
    }
  }
}