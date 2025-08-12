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


const std::string MQTT_BROKER = "tcp://192.168.105.254:1883";
const std::string MQTT_TOPIC = "test";
const std::string MQTT_CLIENT_ID = "cpp_logger";

// MySQL connection (adjust these as needed)
const std::string MYSQL_HOST = "tcp://127.0.0.1:3306";
const std::string MYSQL_USER = "root";
const std::string MYSQL_PASS = "password";
const std::string MYSQL_DB   = "weather";

class callback : public virtual mqtt::callback {
public:
    void message_arrived(mqtt::const_message_ptr msg) override {
        std::string payload = msg->to_string();
        std::cout << "Received message: " << payload << std::endl;

        // Connect to MySQL and insert message
        try {
            sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
            std::unique_ptr<sql::Connection> con(driver->connect(MYSQL_HOST, MYSQL_USER, MYSQL_PASS));
            con->setSchema(MYSQL_DB);

            std::unique_ptr<sql::PreparedStatement> pstmt(
                con->prepareStatement("INSERT INTO data (temp, pressure, altitude, humidity) VALUES (?, ?, ?, ?)")
            );

            // Dummy parse example: assume message format like "22,1010,150,45"
            std::istringstream iss(payload);
            std::string temp, pressure, altitude, humidity;
            std::getline(iss, temp, ',');
            std::getline(iss, pressure, ',');
            std::getline(iss, altitude, ',');
            std::getline(iss, humidity, ',');

            pstmt->setString(1, temp);
            pstmt->setString(2, pressure);
            pstmt->setString(3, altitude);
            pstmt->setString(4, humidity);
            pstmt->execute();

            std::cout << "Data inserted into MySQL" << std::endl;
        } catch (const sql::SQLException& e) {
            std::cerr << "MySQL error: " << e.what() << std::endl;
        }
    }

    void connection_lost(const std::string& cause) override {
        std::cerr << "MQTT connection lost: " << cause << std::endl;
    }
};

int main() {
    mqtt::async_client client(MQTT_BROKER, MQTT_CLIENT_ID);
    callback cb;
    client.set_callback(cb);

    mqtt::connect_options connOpts;
    try {
        client.connect(connOpts)->wait();
        std::cout << "Connected to MQTT broker." << std::endl;

        client.subscribe(MQTT_TOPIC, 1)->wait();
        std::cout << "Subscribed to topic: " << MQTT_TOPIC << std::endl;

        // Stay alive
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        client.disconnect()->wait();
    } catch (const mqtt::exception& e) {
        std::cerr << "MQTT error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
