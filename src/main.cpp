#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <DHT20.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define LED_PIN 15

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT20 DHT;

// The root certificate for validation
static const char* root_ca = \
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDdzCCAl+gAwIBAgIEAgAAuTANBgkqhkiG9w0BAQUFADBaMQswCQYDVQQGEwJJ\r\n"
"RTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJlclRydXN0MSIwIAYD\r\n"
"VQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTAwMDUxMjE4NDYwMFoX\r\n"
"DTI1MDUxMjIzNTkwMFowWjELMAkGA1UEBhMCSUUxEjAQBgNVBAoTCUJhbHRpbW9y\r\n"
"ZTETMBEGA1UECxMKQ3liZXJUcnVzdDEiMCAGA1UEAxMZQmFsdGltb3JlIEN5YmVy\r\n"
"VHJ1c3QgUm9vdDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKMEuyKr\r\n"
"mD1X6CZymrV51Cni4eiVgLGw41uOKymaZN+hXe2wCQVt2yguzmKiYv60iNoS6zjr\r\n"
"IZ3AQSsBUnuId9Mcj8e6uYi1agnnc+gRQKfRzMpijS3ljwumUNKoUMMo6vWrJYeK\r\n"
"mpYcqWe4PwzV9/lSEy/CG9VwcPCPwBLKBsua4dnKM3p31vjsufFoREJIE9LAwqSu\r\n"
"XmD+tqYF/LTdB1kC1FkYmGP1pWPgkAx9XbIGevOF6uvUA65ehD5f/xXtabz5OTZy\r\n"
"dc93Uk3zyZAsuT3lySNTPx8kmCFcB5kpvcY67Oduhjprl3RjM71oGDHweI12v/ye\r\n"
"jl0qhqdNkNwnGjkCAwEAAaNFMEMwHQYDVR0OBBYEFOWdWTCCR1jMrPoIVDaGezq1\r\n"
"BE3wMBIGA1UdEwEB/wQIMAYBAf8CAQMwDgYDVR0PAQH/BAQDAgEGMA0GCSqGSIb3\r\n"
"DQEBBQUAA4IBAQCFDF2O5G9RaEIFoN27TyclhAO992T9Ldcw46QQF+vaKSm2eT92\r\n"
"9hkTI7gQCvlYpNRhcL0EYWoSihfVCr3FvDB81ukMJY2GQE/szKN+OMY3EU/t3Wgx\r\n"
"jkzSswF07r51XgdIGn9w/xZchMB5hbgF/X++ZRGjD8ACtPhSNzkE1akxehi/oCr0\r\n"
"Epn3o0WC4zxe9Z2etciefC7IpJ5OCBRLbf1wbWsaY71k5h+3zvDyny67G7fyUIhz\r\n"
"ksLi4xaNmjICq44Y3ekQEe5+NauQrz4wlHrQMz2nZQ/1/I6eYs9HRCwBXbsdtTLS\r\n"
"R9I4LtD+gdwyah617jzV/OeBHRnDJELqYzmp\r\n"
"-----END CERTIFICATE-----";


// Replace with your network credentials
const char* ssid     = "Chris' Iphone";
const char* password = "chris1688";

// Website Connection
const char* mqtt_server = "PlantMind.azure-devices.net";
const char* mqtt_user = "PlantMind.azure-devices.net/esp32/api-version=2021-04-12";
//set SharedAccessSignature for 504 hours (3 weeks)
const char* mqtt_password = "SharedAccessSignature sr=PlantMind.azure-devices.net%2Fdevices%2Fesp32&sig=geD1KQfWHAFdMX7OcbA1epbSdOHbdgfYI3DwflJjMN0%3D&se=1688049518";
//





const int relayPin = 2;
const int sensorPin = A0;
float soilMoisture = 0;



const int relayAddress = 0x20; // I2C address of the relay module

int UVOUT = 32; //Output from the sensor
int REF_3V3 = 33; //3.3V power on the ESP32 board

unsigned long lastMsg = 0; // global variable
char msg[100];
int value = 0;
int messageID =0;
String deviceID = "esp32";

void syncTime() {
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "UTC", 1);
    tzset();
    delay(1000);
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        delay(2000);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    if (retry < retry_count) {
        Serial.println("Time synchronized");
    } else {
        Serial.println("Failed to synchronize time");
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//Takes an average of readings on a given pin
//Returns the average
int averageAnalogRead(int pinToRead)
{
  byte numberOfReadings = 8;
  unsigned int runningValue = 0; 
 
  for(int x = 0 ; x < numberOfReadings ; x++)
    runningValue += analogRead(pinToRead);
  runningValue /= numberOfReadings;
 
  return(runningValue);
}

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!DHT.begin()) {
    Serial.println("Could not find a valid sensor, check wiring!");
    while (1);
  }
  WiFi.mode(WIFI_MODE_STA);
  setup_wifi();
  syncTime();


  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
  pinMode(relayPin, OUTPUT);
  //sets pin 2 to output
  pinMode(sensorPin, INPUT);
  //sets pin A0 to input
  digitalWrite(relayPin, HIGH);
  //turns on relay, in turn turning on pump
  //minMode for UV sensor
  pinMode(UVOUT, INPUT);
  pinMode(REF_3V3, INPUT);
  delay(2000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("esp32", mqtt_user, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //Serial.println();
  //Serial.print("MOISTURE LEVEL: ");
  //prints "MOISTURE LEVEL: "
  soilMoisture = analogRead(sensorPin);
  //Serial.print(soilMoisture);
  //Serial.println();
  long now = millis();
  //sends message every 2 seconds
  /*
  if(sensorValue<3000)
  {
  digitalWrite(relayPin, HIGH);
  Serial.println("Pump on");
  }
  else
  {
  digitalWrite(relayPin, LOW);
  //if moisture level is not above 750, turns on pump
  Serial.println("Pump off");
  }
  Serial.println();
  */
  //prints new line for spacing
  delay(5000);

  if (now - lastMsg > 2000) {
    lastMsg = now;
    int status = DHT.read();
    float temp = DHT.getTemperature();
    float humidity = DHT.getHumidity();
    
    //client.disconnect();
    //delay(500);

    // Read UV level
    int uvIntensity = averageAnalogRead(UVOUT);
    int refLevel = averageAnalogRead(REF_3V3);

    // Reconnect to MQTT server
    //reconnect();
  
    //Use the 3.3V power pin as a reference to get a very accurate output value from sensor
    float outputVoltage = 3.3 / refLevel * uvIntensity;
  
    float uvLevel = (mapfloat(outputVoltage, 0.99, 2.8, 0.0, 15.0)) / 25; //Convert the voltage to a UV intensity level
    if (uvLevel < 0) {
      uvLevel = uvLevel * -1;
    }
  //sets "sensorValue" to input value from A0
    if(soilMoisture < 3000) {
      digitalWrite(LED_PIN, HIGH);
    } else if (soilMoisture > 3000) {
      digitalWrite(LED_PIN, LOW);
    }
    delay(1000);
    messageID++;
    
    //sprintf(msg, "{\"Temperature\": %.2f, \"Humidity\": %.2f}", temp, humidity);
    sprintf(msg, "{\"messageId\": %d, \"deviceId\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f, \"soilMoisture\": %.2f, \"uvLevel\": %.2f}",
            messageID, deviceID, temp, humidity, soilMoisture, uvLevel);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("devices/esp32/messages/events/", msg);
  }
}
