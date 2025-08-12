#include <Adafruit_BME280.h>
#include <Wire.h>
#include <WiFiS3.h>
#include <PubSubClient.h>

//MQTT
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "vejrstation/herning/data";

WiFiClient espClient;
PubSubClient client(espClient);

//Wifi
const char* ssid = "h4prog";
const char* password = "1234567890";

WiFiServer server(80);

Adafruit_BME280 bme;

unsigned long lastUpdate = 0;
const long updateInterval = 5000;

void setup() {
  
  Serial.begin(9600);

  while (!Serial);

  connectToWifi(ssid, password);

  client.setServer(mqtt_server, mqtt_port);
  
  if (!bme.begin(0x76)) {
    Serial.println("BME280 not found!");
    while (1); // Halt if sensor not found
  }
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  if (millis() - lastUpdate > updateInterval) {
    lastUpdate = millis();

    measureBME();
  }
}

void measureBME() {
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pressure = bme.readPressure() / 100;

    Serial.print("Temp: ");
    Serial.print(temp, 1);
    Serial.print(" °C | Hum: ");
    Serial.print(hum, 1);
    Serial.print(" % | Pressure: ");
    Serial.print(pressure, 1);
    Serial.println(" hPa");

    char payload[128];
    snprintf(payload, sizeof(payload), 
    "{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}",
    temp, hum, pressure);

    client.publish(mqtt_topic, payload);
}

bool connectToWifi(const char* ssid, const char* password) {
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  while (WiFi.localIP() == INADDR_NONE) {
    delay(100);
  }

  Serial.println("\nconnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

//connect to mqtt
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("arduinoClient123")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}