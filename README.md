# IoT-Intruder-System:

![Project Photo 2](https://github.com/user-attachments/assets/74cf6a1d-44e8-46dc-8db7-544cb9516859)
![Project Photo 1](https://github.com/user-attachments/assets/33864212-43f0-4885-a98e-9742ce45dc19)


# Brief Description

This project has been created as a final assignment for the subject Internet of Things. The main goal of this project is to simulate an intruder system using a buzzer and other components.

# Components Used

- ESP32-S3 Breadboard
- Jumpers
- Resistors (220 Ohm and 1 kOhm)
- Blue LED light
- Red LED light
- Ultrasonic Sensor
- Buzzer
- RBG LED light
- NPN Transistor

# Softwares Used

- Visual Studio Code (PlatformIO)
- MQTT Explorer
- InfluxDB
- Grafana
- LightBlue (Bluetooth)
- Hotspot

# Functionality

The program starts by a white RBG led light that indicates that no attempt to enter a password has been made so far. Additionally, ESP32 scans the environment looking for a device to connect to using the WiFi hotspot feature. Once the WiFi connection based on the SSID and password has been established, the connection for the MQTT explorer (broker) gets also established. The MQTT explorer shows different WiFi channels with each channel having different SSIDs. Meanwhile, distance from the Ultrasonic Sensor is also displayed since ESP32 continuously updates the distance. <br>
Additionally, since InfluxDB connection is established, the distance value from the MQTT gets sent and synched on the website. This also allows the data to be displayed in Grafana. 
By using LightBlue, the color of the RBG led light is controlled based on the user's input (i.e. password). If the password is correct, the RGB led light turns to green indicating a matching and hence, when the user attempts to place their hand to record a value below the threshold set for the distance, the buzzer will simply not play. <br>
However, if the case was different (i.e. the input entered by the user doens't match the password set), then the RGB led light changes to red indicating a mismatch and therefore, when the user attempts to place their hand to record a value below the threshold set for the distance, the buzzer will start playing and the two led lights (red and blue) will start flashing.
