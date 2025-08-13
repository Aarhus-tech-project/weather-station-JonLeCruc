#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <chrono>
#include <mqtt/async_client.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>

// MQTT Config
const std::string BROKER_ADDRESS = "tcp://192.168.105.254:1883";
const std::string TOPIC_NAME = "test";
const std::string CLIENT_ID = "cpp_logger";

// MySQL Config
const std::string DB_HOST = "tcp://127.0.0.1:3306";
const std::string DB_USER = "root";
const std::string DB_PASS = "password";
const std::string DB_NAME = "weather";

// Simple validation function to reject NaN or empty strings
bool isValidValue(const std::string& value) {
    return !value.empty() && value != "NaN" && value != "nan";
}

// Custom MQTT callback handler
class MqttMessageHandler : public virtual mqtt::callback {
public:
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string payload = msg->to_string();
        std::cout << "Incoming payload: " << payload << std::endl;

        std::istringstream stream(payload);
        std::string temp, pressure, altitude, humidity;
        std::getline(stream, temp, ',');
        std::getline(stream, pressure, ',');
        std::getline(stream, altitude, ',');
        std::getline(stream, humidity, ',');

        if (!isValidValue(temp) || !isValidValue(pressure) || !isValidValue(altitude) || !isValidValue(humidity)) {
            std::cerr << "Invalid data. Skipping insert: " << payload << std::endl;
            return;
        }

        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> conn(driver->connect(DB_HOST, DB_USER, DB_PASS));
            conn->setSchema(DB_NAME);

            std::unique_ptr<sql::PreparedStatement> stmt(
                conn->prepareStatement("INSERT INTO data (temp, pressure, altitude, humidity) VALUES (?, ?, ?, ?)")
            );

            stmt->setString(1, temp);
            stmt->setString(2, pressure);
            stmt->setString(3, altitude);
            stmt->setString(4, humidity);
            stmt->execute();

            std::cout << "Data written to MySQL\n";
        } catch (const sql::SQLException& e) {
            std::cerr << "MySQL error: " << e.what() << std::endl;
        }
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "MQTT connection lost: " << cause << std::endl;
    }
};

int main() {
    mqtt::async_client mqttClient(BROKER_ADDRESS, CLIENT_ID);
    MqttMessageHandler handler;
    mqttClient.set_callback(handler);

    mqtt::connect_options connOpts;

    try {
        mqttClient.connect(connOpts)->wait();
        std::cout << "Connected to MQTT broker\n";

        mqttClient.subscribe(TOPIC_NAME, 1)->wait();
        std::cout << "Subscribed to topic: " << TOPIC_NAME << "\n";

        // Keep the app running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        mqttClient.disconnect()->wait();  // unreachable but graceful
    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
