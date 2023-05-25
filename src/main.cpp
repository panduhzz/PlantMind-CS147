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


#define LED_PIN 4



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
"-----END CERTIFICATE-----\r\n";


// Replace with your network credentials
const char* ssid     = "Chris' Iphone";
const char* password = "chris1688";

// Replace with your unique MQTT endpoint
const char* mqtt_server = "CS147-Group14.azure-devices.net";
const char* mqtt_user = "CS147-Group14.azure-devices.net/Espressif/?api-version=2021-04-12";
const char* mqtt_password = "SharedAccessSignature sr=CS147-Group14.azure-devices.net%2Fdevices%2FEspressif&sig=DlgmSPsQ1GIS%2BX5gh%2BfnH%2FYglW%2BbU5U7mFgSrZp%2BTqE%3D&se=1686854868";

WiFiClientSecure espClient;
PubSubClient client(espClient);
DHT20 DHT;

long lastMsg = 0;
char msg[100];
int value = 0;
int messageID =0;
String deviceID = "Espressif";

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

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!DHT.begin()) {
    Serial.println("Could not find a valid sensor, check wiring!");
    while (1);
  }
  
  setup_wifi();
  syncTime();
  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
}





void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Espressif", mqtt_user, mqtt_password)) {
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

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    int status = DHT.read();
    float temp = DHT.getTemperature();
    float humidity = DHT.getHumidity();
    messageID++;
    
    //sprintf(msg, "{\"Temperature\": %.2f, \"Humidity\": %.2f}", temp, humidity);
    sprintf(msg, "{\"messageId\": %d, \"deviceId\": \"%s\", \"temperature\": %.2f, \"humidity\": %.2f}",
            messageID, deviceID, temp, humidity);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("devices/Espressif/messages/events/", msg);
  }
}
