#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include "FastLED.h"
#include "common.h"

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define NUM_LEDS 180 // TODO //Specify the number of LEDs you have per Light-Node.  This number needs to be correct.
#define DATA_PIN 2 // This is D4 in NodeMCU

CRGB leds[NUM_LEDS];

#define WIFI_TIMEOUT 30000              // checks WiFi every 30s. Reset after this time, if WiFi cannot reconnect.

void ensureNetworkConnection(unsigned long now);
String convertPayloadToString(byte* payload, unsigned int length);
void receivedMessage(char* topic, byte* payload, unsigned int length);
bool connect();
void wifiSetup();

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;

/*
* MQTT Code
*/
const char *mqtt_topic_color_remote = "color-remote";

// TODO: Make sure the clientIDs are different per light node.
const char *clientID = "ESP8266_6";
WiFiClient wifiClient;

// TODO: Change the IP Address of the MQTT Broker
PubSubClient client("192.168.0.106", 1883, wifiClient);  

void setup(){
  Serial.begin(115200);

  Serial.println("Wifi setup");
 wifiSetup();

  // connect to MQTT Broker
  // setCallback sets the function to be called when a message is received.
  client.setCallback(receivedMessage);
  if (connect()){
		Serial.println("Connected Successfully to MQTT Broker!");
  }
  else {
		Serial.println("Connection Failed!");
  }

	FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
	Serial.println("Ready!");
}

void loop() {
	unsigned long now = millis();

	// Checking wifi connection
	if (now - last_wifi_check_time > WIFI_TIMEOUT) {
		ensureNetworkConnection(now);
	}

	// Checking MQTT is still connected
  if (!client.connected()) {
		connect();
  }
  // client.loop() just tells the MQTT client code to do what it needs to do itself (i.e. check for messages, etc.)
  client.loop();
} 

bool connect() {
  // connect to MQTT Server and subscribe to the topic
  if (client.connect(clientID)) {
	  client.subscribe(mqtt_topic_color_remote);
	  return true;
	}
	else {
	  return false;
  }
}

void ensureNetworkConnection(unsigned long now) {
	Serial.print("Checking WiFi... ");
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi connection lost. Reconnecting...");
	 wifiSetup();
	} else {
		Serial.println("OK");
	}
	last_wifi_check_time = now;
}

String convertPayloadToString(byte* payload, unsigned int length) {
	char message[length];

	for(int i = 0; i < length; i++) {
		message[i] = (char)payload[i];
	}
	String decodedMessage = String(message);
	return decodedMessage;
}

void receivedMessage(char* topic, byte* payload, unsigned int length) {
	String decodedMessage = convertPayloadToString(payload, length);

	int rgbValues[] = {-1, -1, -1};
	String rgbValue = "";
	int rgbIndex = 0;

	for(int i = 0; i < decodedMessage.length() && i < 11; i++) {
		if (decodedMessage.charAt(i) == ',') {
			rgbValues[rgbIndex] = rgbValue.toInt();
			rgbValue = "";
			rgbIndex += 1;
			continue;
		}
		rgbValue += decodedMessage[i];
	}
	rgbValues[2] = rgbValue.toInt();

	int red = rgbValues[0];
	int green = rgbValues[1];
	int blue = rgbValues[2];
	
	for(uint16_t i=0; i <= NUM_LEDS; i++) {
		leds[i] = CRGB(red,green,blue);
  }
	FastLED.show();
	return;
}

/*
 * connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
void wifiSetup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);

  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
	delay(500);
	Serial.print(".");

	if(millis() - connect_start > WIFI_TIMEOUT) {
	  Serial.println();
	  Serial.print("Tried ");
	  Serial.print(WIFI_TIMEOUT);
	  Serial.print("ms. Resetting ESP now.");
	  ESP.reset();
	}
}

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}
