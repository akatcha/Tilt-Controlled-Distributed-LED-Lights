#include "Arduino.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "helpers.h"
#include "common.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
# define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
# define OLED_RESET LED_BUILTIN // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, OLED_RESET);

int VECTOR_DIVISOR = 64;

static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

Adafruit_LIS3DH lis = Adafruit_LIS3DH();

//TODO: Change this to IP Address of MQTT Broker
const char * mqtt_server = "192.168.0.106";
long lastReconnectAttempt;
bool isConfigureColorEnabled;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
int buttonRead;

void setup() {
  Serial.begin(115200);

  // Connecting to screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Connecting to accelerometer
  Serial.println("LIS3DH test!");
  if (!lis.begin(0x18)) { // change this to 0x19 for alternative i2c address
    Serial.println("Couldnt start Accelerometer");
    for (;;);
  }

  Serial.println("LIS3DH found!");
  lis.setRange(LIS3DH_RANGE_2_G); // 2, 4, 8 or 16 G!

  // Connecting to wifi and configuring MQTT broker
  setupWifi();
  client.setServer(mqtt_server, 1883);

  long lastReconnectAttempt = 0;
  isConfigureColorEnabled = true;
  buttonRead = 1; // 1 means that the button is not , 0 means it is clicked
  pinMode(0, INPUT_PULLUP);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(400);

  // Clear the buffer
  display.clearDisplay();
}

void loop() {
  // Asynchronous (non-blocking) reconnection attempt
  if (!client.connected()) {
    timeSensitiveReconnect();
  } else {
    // Client connected
    client.loop();
  }

  if (isConfigureColorEnabled) {
    lis.read(); // get X Y and Z data at once
    displayToScreen(lis.x, lis.y, lis.z);
    publishRGBValues(lis.x, lis.y, lis.z);
  }

  buttonRead = digitalRead(0);
  if (isButtonPressed()) {
    freezeColors();
  }

  buttonRead = digitalRead(0);
  if (isButtonPressed()) {
    turnOffLights();
  }
}

boolean isButtonPressed() {
  return buttonRead == 0;
}

boolean reconnect() {
  if (client.connect("ESP8266-remote")) {
    Serial.println("Reconnected");
  }
  return client.connected();
}

void timeSensitiveReconnect() {
  long now = millis();
  if (now - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = now;
    // Attempt to reconnect
    if (reconnect()) {
      lastReconnectAttempt = 0;
    }
  }
}

void turnOffLights() {
  String payload = String(0) + "," + String(0) + "," + String(0);
  client.publish("color-remote", payload.c_str());
  delay(400);
}

void freezeColors() {
  Serial.println("BUTTON CLICK");
  isConfigureColorEnabled = !isConfigureColorEnabled;
  delay(400);
}

void formatTitle() {
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 2);
  String title = "Clicker Readings";
  display.println(title);
}

void formatXYZValues(int x, int y, int z) {
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(0, 18);

  String xToPrint = "X: " + String(x);
  display.println(xToPrint);

  String yToPrint = "Y: " + String(y);
  display.println(yToPrint);

  String zToPrint = "Z: " + String(z);
  display.println(zToPrint);
}

void formatRGBValues(int x, int y, int z) {
  int red = convertVectorValueToColorValue(x);
  int green = convertVectorValueToColorValue(y);
  int blue = convertVectorValueToColorValue(z);

  String RGB = "RGB: (" + String(red) + ", " + String(green) + ", " + String(blue) + ")";
  display.println(RGB);
}

void displayToScreen(int x, int y, int z) {
  // Clear whatever was on screen
  display.clearDisplay();
  formatTitle();
  formatXYZValues(x, y, z);
  formatRGBValues(x, y, z);
  display.display();
}

void publishRGBValues(int x, int y, int z) {
  int red = convertVectorValueToColorValue(lis.x);
  int green = convertVectorValueToColorValue(lis.y);
  int blue = convertVectorValueToColorValue(lis.z);
  String payload = String(red) + "," + String(green) + "," + String(blue);
  client.publish("color-remote", payload.c_str());
}

int sanitizeSegment(int value) {
  if (value < 0) {
    return 0;
  } else if (value > 255) {
    return 255;
  } else {
    return value;
  }
}

int convertVectorValueToColorValue(int vectorValue) {
  int segment = abs(vectorValue / VECTOR_DIVISOR);
  return sanitizeSegment(segment);
}

void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
