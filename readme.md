Weather Station project

This project logs the atmospheric data such as temperature, humidity and pressure.
Features:

- It subscribes to MQTT broker for sensor data.
- It validates and parses incoming data.
- It stores readings in MySQL database.
- Rejects invalid values such as NaN or Null.

It's coded in C++ and the database/server is set up with linux (debian).

Setup:

- ensure you are on the h4prog wifi, if not then connect via tailscale ip address.
- connect the arduino with a BME280 sensor (3,3V - gnd - sda - scl);
- ssh login via cmd.
- connect to dbeaver.
- compile the code and verify the output in the serial monitor, by checking if the following things are connected: (BME280 sensor, WiFi, MQTT Broker);
- It writes data every 10 seconds, refresh the DB and see the recorded values.

Grafana:
Connect to grafana using my ip address: 192.168.105.254:3000 in the browser.
Click Login (UserName: admin - password: admin)
Navigate to Dashboards to see the different readings (They are currently filtered to show the data from the last 2 days, so probably won't show anything on monday)
Grafana has an automatic refresh every 30 seconds.
