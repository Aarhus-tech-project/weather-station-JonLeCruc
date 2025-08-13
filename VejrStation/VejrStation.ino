#include <WiFiS3.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoMqttClient.h>

#define SEA_LEVEL_PRESSURE (1013.25)

const char* ssid = "h4prog";
const char* password = "1234567890";

const char* brokerIP = "192.168.105.254";
const char* mqttTopic = "test";

WiFiClient wifiClient;
MqttClient mqtt(wifiClient);

Adafruit_BME280 sensor;

void setup() {
  delay(1000); //small boot delay
  Serial.begin(115200);
  while (!Serial); //wait for serial montior to open

  connectToWifi(); //connect to wifi

  delay(1000); // stability delay

  if (!sensor.begin(0x76)) {
    Serial.println("Failed to find BME280 sensor");
    while (true); // halt
  }

  Serial.println("BME280 ready");

  connectToMQTT(); //connect to mqtt broker
}

void loop() {
  mqtt.poll();
  delay(10000); //Set the reading to an 10s interval

  if (!mqtt.connected()) { //checks to handle if the broker is disconnected
    Serial.println("MQTT disconnected. Reconnecting...");
    connectToMQTT();
  }

  sendSensorData();
}

/* connectToWifi
Params: none
Functionality: connects to wifi using the defined ssid/password.
It won't continue the code as long as wifi isn't connected.

Returns: nothing, only prints in serial.
 */
void connectToWifi() {
  Serial.print("Connecting to WiFi");

  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, password);
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/* connectToMQTT
Params: none
Functionality: Connects to the MQTT broker, it won't continue until the broker is connected.

Returns: nothing, only prints in serial.
 */
void connectToMQTT() {
  Serial.println("Connecting to MQTT...");

  mqtt.setId("arduino-weather-sensor");

  while (!mqtt.connect(brokerIP)) {
    Serial.print("MQTT failed code: ");
    Serial.println(mqtt.connectError());
    delay(5000);
  }
  
  Serial.println("Connected to MQTT broker");
}

/* sendSensorData
Params: none
Functionality: Uses the BME280 sensor to store reading in variables.
It formats these variables and sends the data to the broker.
It calls the logToSerial function with the different reading in the parameter.
Returns: nothing
 */
void sendSensorData() {
  float temperature = sensor.readTemperature();
  float pressure = sensor.readPressure() / 100.0;
  float altitude = sensor.readAltitude(SEA_LEVEL_PRESSURE);
  float humidity = sensor.readHumidity();

  if (isnan(temperature) || isnan(pressure) || isnan(altitude) || isnan(humidity)) {
    Serial.println("Sesnor reading invalid - skipping publish");
    return;
  }

  // Combine values into CSV format
  String payload = String(temperature, 2) + "," +
                String(pressure, 2) + "," +
                String(altitude, 2) + "," +
                String(humidity, 2);

  mqtt.beginMessage(mqttTopic);
  mqtt.print(payload);
  mqtt.endMessage();

  logToSerial(temperature, pressure, altitude, humidity);
}

/* logToSerial
Params: float temperature - example: 26.54, float pressure - example: 1015.04,
float altitude - example: -15.22, float humidity - example: 50.74
Functionality: Logs the different readings in the serial at 115200 baud.
Returns: nothing
 */
void logToSerial(float t, float p, float a, float h) {
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C, Pressure: ");
  Serial.print(p);
  Serial.print(" hPa, Altitude: ");
  Serial.print(a);
  Serial.print(" m, Humidity: ");
  Serial.print(h);
  Serial.println(" %");
}
// by jon holm