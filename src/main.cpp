#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "secrets.h"

#define POWER_LED 19
#define WIFI_LED 18
#define REQUEST_LED 17
#define POWER_BUTTON 16

// Set WiFi Credentials
const char ssid[] = SECRET_SSID;
const char password[] = SECRET_PASS;

// Set MQTT
const char broker[] = "temphumidity.lan";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

int last_button_state;

bool power_state = false;
bool wifi_state = false;

unsigned long last_time = 0;
unsigned long delay_time = 10000;

Adafruit_AHTX0 aht;
WiFiClient client;

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void reconnect() {
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("connected");
            mqttClient.subscribe("temphumidity");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(9600);

    pinMode(POWER_BUTTON, INPUT);
    pinMode(POWER_LED, OUTPUT);
    pinMode(REQUEST_LED, OUTPUT);
    pinMode(WIFI_LED, OUTPUT);

    digitalWrite(POWER_LED, LOW);
    digitalWrite(WIFI_LED, LOW);
    digitalWrite(REQUEST_LED, LOW);

    while (!aht.begin()) {
        Serial.println("Could not find AHT? Check wiring");
        delay(10);
    }
    Serial.println("AHT10 found");

    WiFiClass::mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.begin(ssid, password);

    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print("..");

    while (WiFiClass::status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }

    Serial.print("\nIP address: ");
    Serial.println(WiFi.localIP());

    mqttClient.setServer(broker, 1883);
    mqttClient.setCallback(callback);
}

void loop() {
    int button_state = digitalRead(POWER_BUTTON);

    if (last_button_state != button_state) {
        delay(200);
        power_state = !power_state;
    }

    if (power_state) {
        digitalWrite(POWER_LED, HIGH);
    } else {
        digitalWrite(POWER_LED, LOW);
    }

    if (WiFiClass::status() == WL_CONNECTED) {
        wifi_state = true;
        digitalWrite(WIFI_LED, HIGH);
    } else {
        wifi_state = false;
        digitalWrite(WIFI_LED, LOW);
    }

    if (!mqttClient.connected()) {
        reconnect();
    }

    if (power_state && wifi_state && mqttClient.connected() && ((millis() - last_time) > delay_time)) {
        Serial.println("Sending Request");
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);

        JsonDocument doc;
        char request[1000];

        doc["temp"] = temp.temperature;
        doc["humidity"] = humidity.relative_humidity;

        serializeJson(doc, request);

        mqttClient.publish("temphumidity", request);

        Serial.print("Temperature: "); Serial.println(temp.temperature);
        Serial.print("Humidity: "); Serial.println(humidity.relative_humidity);

        digitalWrite(REQUEST_LED, HIGH);
        delay(100);
        digitalWrite(REQUEST_LED, LOW);

        last_time = millis();
    }
}
