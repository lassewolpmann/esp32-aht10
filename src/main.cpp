#include <Arduino.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define POWER_LED 19
#define WIFI_LED 18
#define REQUEST_LED 17
#define POWER_BUTTON 16

const char* ssid = "A17";
const char* password = "kaukonen";
const char* serverName = "http://192.168.1.201:5173/api/data";

int last_button_state;

bool power_state = false;
bool wifi_state = false;

unsigned long last_time = 0;
unsigned long delay_time = 10000;

Adafruit_AHTX0 aht;
WiFiClient client;
HTTPClient http;

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

    if (power_state && wifi_state && ((millis() - last_time) > delay_time)) {
        Serial.println("Sending Request");
        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);

        http.begin(client, serverName);
        http.addHeader("Content-Type", "application/json");

        JsonDocument doc;
        String request;

        doc["temp"] = temp.temperature;
        doc["humidity"] = humidity.relative_humidity;

        serializeJson(doc, request);

        int httpResponseCode = http.POST(request);

        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode == 200) {
            digitalWrite(REQUEST_LED, HIGH);
            delay(100);
            digitalWrite(REQUEST_LED, LOW);
        }

        http.end();
        last_time = millis();
    }
}
