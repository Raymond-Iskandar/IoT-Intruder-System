#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

char *ssid = "insert your ssid";
char *pwd = "insert your password";

#define INFLUXDB_URL "https://us-east-1-1.aws.cloud2.influxdata.com/"
#define INFLUXDB_TOKEN "insert your token"
#define INFLUXDB_ORG "insert your org"
#define INFLUXDB_BUCKET "insert your bucket"

#define trigPin 13 // define trigPin
#define echoPin 14 // define echoPin.
#define MAX_DISTANCE 700 // Maximum sensor distance is rated at 400-500cm.
#define RED_PIN 2
#define BLUE_PIN 42
#define BUZZER_PIN 12

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <string.h>

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t txValue = 0;
long lastMsg = 0;
char rxload[20];
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


//RGB LIGHT:
const byte ledPins[] = {38, 39, 40}; //define red, green, blue led pins
const byte chns[] = {0, 1, 2}; //define the pwm channels
int red, green, blue;
const char* correctPassword = "1234";
bool passwordAccepted = false;
bool isGreen = false;
bool isRed = false;

class MyServerCallbacks : public BLEServerCallbacks {
 void onConnect(BLEServer *pServer) {
 deviceConnected = true;
 };
 void onDisconnect(BLEServer *pServer) {
 deviceConnected = false;
 }
};

class MyCallbacks : public BLECharacteristicCallbacks {
 void onWrite(BLECharacteristic *pCharacteristic) {
 std::string rxValue = pCharacteristic->getValue();

 if(rxValue == correctPassword){
    Serial.println("Correct Password");
    passwordAccepted = true;
    isGreen = true;
    isRed = false;
 }else{
    Serial.println("Incorrect Password");
    passwordAccepted = false;
    isRed = true;
    isGreen = false;
 }

 if (rxValue.length() > 0) {
 for (int i = 0; i < 20; i++) {
 rxload[i] = 0;
 }
 for (int i = 0; i < rxValue.length(); i++) {
 rxload[i] = (char)rxValue[i];
 }
 }
 }
};

void setupBLE(String BLEName) {
 const char *ble_name = BLEName.c_str();
 BLEDevice::init(ble_name);
BLEServer *pServer = BLEDevice::createServer();
 pServer->setCallbacks(new MyServerCallbacks());
 BLEService *pService = pServer->createService(SERVICE_UUID);
 pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
BLECharacteristic::PROPERTY_NOTIFY);
 pCharacteristic->addDescriptor(new BLE2902());
 BLECharacteristic *pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
BLECharacteristic::PROPERTY_WRITE);
 pCharacteristic->setCallbacks(new MyCallbacks());
 pService->start();
 pServer->getAdvertising()->start();
 Serial.println("Waiting a client connection to notify...");
}

//timeOut= 2*MAX_DISTANCE /100 /340 *1000000 = MAX_DISTANCE*58.8
float timeOut = MAX_DISTANCE * 60;
int soundVelocity = 340; // define sound speed=340m/s

#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 20
#define WRITE_BUFFER_SIZE MAX_BATCH_SIZE * 3

// Time zone info
#define TZ_INFO "AEST-10AEDT,M10.1.0,M4.1.0/3"

// Declare InfluxDB client_mqtt instance with preconfigured InfluxCloud certificate
InfluxDBClient client_influxdb(INFLUXDB_URL, INFLUXDB_ORG,
            INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Declare Data point

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
// const char *mqtt_broker = "broker.hivemq.com";
const char *topic = "esp32name/wifiMeta/rssi";
const char *mqtt_username = "emqx";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
const int distanceThreshold = 10;

const char *topicList[] = {
    "esp32name/msgIn/wifiScanControl"
    // Add more topics as needed
};
const int topicCount = sizeof(topicList) / sizeof(topicList[0]);

volatile int8_t ind_channel2scan = 1;
volatile int8_t thr = -80;
volatile int scan_time = 100; // in ms

unsigned long tA_clock_calibration = 0; 

String clientID;

WiFiClient espClient;
PubSubClient client_mqtt(espClient);

void callback_mqtt(char *topic, byte *payload, unsigned int length);
void reconnect();
void fun_subscribe(void);
void fun_connect2wifi();

void setColor(byte r, byte g, byte b) {
  analogWrite(ledPins[0], 255 - r); // Common anode LED, low level to turn on the led.
  analogWrite(ledPins[1], 255 - g);
  analogWrite(ledPins[2], 255 - b);
}

void checkCondition() {
  long now = millis();
  if (now - lastMsg > 100) {
    if (deviceConnected && strlen(rxload) > 0) {
      Serial.print("Received: ");
      Serial.println(rxload);
      //if (strncmp(rxload, "red", 3) == 0) {
      //  setColor(255, 0, 0); // Red
      //}
      //if (strncmp(rxload, "green", 5) == 0) {
      //  setColor(0, 255, 0); // Green
      //}
      memset(rxload, 0, sizeof(rxload));
    }
    lastMsg = now;
  }
}

float getSonar() {
 unsigned long pingTime;
 float distance;
 // make trigPin output high level lasting for 10us to trigger HC_SR04
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10);
 digitalWrite(trigPin, LOW);
 // Wait HC-SR04 returning to the high level and measure out this waiting time
 pingTime = pulseIn(echoPin, HIGH, timeOut);
 // calculate the distance according to the time
 distance = (float)pingTime * soundVelocity / 2 / 10000;
 return distance; // return the distance value
}

void controlLights(){
    if(getSonar() <= distanceThreshold && isRed){
      tone(BUZZER_PIN, 1000);
      delay(500);
      digitalWrite(RED_PIN, HIGH);
      delay(1000);
      digitalWrite(RED_PIN, LOW);
      digitalWrite(BLUE_PIN, HIGH);
      delay(1000);
      digitalWrite(BLUE_PIN, LOW);
      delay(1000);
  }else{
    noTone(BUZZER_PIN);
    digitalWrite(RED_PIN, LOW);
    digitalWrite(BLUE_PIN, LOW);
  }
}

void checkPassword(bool password){
  if(password){
    setColor(0,255,0);
    noTone(BUZZER_PIN);
  }else{
    setColor(255,0,0);
  }
}

void setup()
{
  pinMode(trigPin,OUTPUT);// set trigPin to output mode
  pinMode(echoPin,INPUT); // set echoPin to input mode
  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < 3; ++i) {
    pinMode(ledPins[i], OUTPUT);
  }
  for(int i = 0; i < 3; ++i){
    ledcSetup(chns[i],5000,8);
    ledcAttachPin(ledPins[i],chns[i]);
  }
  setupBLE("ESP32S3_Bluetooth");
  Serial.begin(115200);
  fun_connect2wifi();

  clientID = "esp32name-";
  clientID += WiFi.macAddress();

  client_mqtt.setServer(mqtt_broker, mqtt_port);
  client_mqtt.setCallback(callback_mqtt);
  while (!client_mqtt.connected())
  {
    Serial.printf("The client_mqtt %s connects to the public MQTT broker\n", clientID.c_str());
    if (client_mqtt.connect(clientID.c_str(), mqtt_username, mqtt_password))
    {
      Serial.println("Public EMQX MQTT broker connected");
    }
    else
    {
      Serial.print("failed with state ");
      Serial.print(client_mqtt.state());
      delay(2000);
    }
  }
  fun_subscribe();

  // Accurate time is necessary for certificate validation and writing in batches
  // We use the NTP servers in your area as provided by: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  // Check server connection
  if (client_influxdb.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client_influxdb.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client_influxdb.getLastErrorMessage());
  }

  // Enable messages batching and retry buffer
  client_influxdb.setWriteOptions(
      WriteOptions().writePrecision(WRITE_PRECISION).
      batchSize(MAX_BATCH_SIZE).bufferSize(WRITE_BUFFER_SIZE));

  // Add tags to the data point
  //sensorNetworks.addTag("device", "ESP32");
  // sensorNetworks.addTag("location", "homeoffice");
  // sensorNetworks.addTag("esp32_id", String(WiFi.BSSIDstr().c_str()));
  tA_clock_calibration = millis(); 
}

void loop()
{

  delay(100); // Wait 100ms between pings (about 20 pings/sec).
  Serial.printf("Distance: ");
  Serial.print(getSonar()); // Send ping, get distance in cm and print result
  Serial.println("cm");
  checkCondition();

  if (millis() - tA_clock_calibration > 60*60*1000)
  {
    timeSync(TZ_INFO, "pool.ntp.org", "time.nist.gov");
    tA_clock_calibration = millis(); 
  }

  // If no Wifi signal, try to reconnect it
  if (!WiFi.isConnected())
  {
    WiFi.reconnect(); 
    Serial.println("Wifi connection lost");
  }
  
  if (!client_mqtt.connected())
  {
    reconnect();
  }
  Serial.println(client_mqtt.loop());

  int n = WiFi.scanNetworks(0, 0, 0, scan_time);
  time_t tnow = time(nullptr);

  if (n != 0)
  {
    int cnt = 0;
    for (int i = 0; i < n; i++)
    {
      if (WiFi.RSSI(i) >= thr)
      {
        Point sensorNetworks("wifi_scan");
        sensorNetworks.addTag("SSID", WiFi.SSID(i));
        sensorNetworks.addTag("channel", String(WiFi.channel(i)));
        sensorNetworks.addTag("MAC", WiFi.BSSIDstr());
        sensorNetworks.addTag("location", "homeoffice");
        sensorNetworks.addField("distance", getSonar());
        sensorNetworks.setTime(tnow); // set the time

        String tmp = client_influxdb.pointToLineProtocol(sensorNetworks);        
        Serial.print("Writing: ");
        Serial.println(tmp);

        // Write point into buffer - low priority measures
        client_influxdb.writePoint(sensorNetworks);

        String topic_base = String("esp32name/wifiScan/") + String(WiFi.channel(i)) + "/";

        tmp = String(getSonar());
        String tmp1 = topic_base + WiFi.SSID(i);
        const char *payload = tmp.c_str();
        client_mqtt.publish((tmp1).c_str(), payload); 
        // delay(500); 
      }
    }    
  }
  else
  {
    Serial.println("Zero networks scanned ...");
  }
  
  // End of the iteration - force write of all the values into InfluxDB as single transaction
  if (!client_influxdb.isBufferEmpty())
  {
    Serial.println("Flushing data into InfluxDB");
    if (!client_influxdb.flushBuffer())
    {
      Serial.print("InfluxDB flush failed: ");
      Serial.println(client_influxdb.getLastErrorMessage());
      Serial.print("Full buffer: ");
      Serial.println(client_influxdb.isBufferFull() ? "Yes" : "No");
    }
  }
  checkPassword(passwordAccepted);
  controlLights();

}

void callback_mqtt(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error)
  {
    const char *channel = doc["channel"];
    const char *thr_local = doc["thr"];
    const char *time_per_scan = doc["time_per_scan"]; // in ms

    Serial.println(channel);
    Serial.println(thr);
    Serial.println(time_per_scan);

    ind_channel2scan = String(channel).toInt();
    thr = String(thr_local).toInt();
    scan_time = String(time_per_scan).toInt();
  }
}

void reconnect()
{
  while (!client_mqtt.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client_mqtt.connect(clientID.c_str()))
    {
      Serial.println("connected");
      fun_subscribe();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client_mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void fun_subscribe(void)
{
  for (int i = 0; i < topicCount; i++)
  {
    client_mqtt.subscribe(topicList[i]);
    Serial.print("Subscribed to: ");
    Serial.println(topicList[i]);
  }
}

void fun_connect2wifi()
{
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n");
  Serial.print("Connected to the Wi-Fi network with local IP: ");
  Serial.println(WiFi.localIP());
}



