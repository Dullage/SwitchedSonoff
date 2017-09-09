#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SwitchedRelayClass.h"

#define sonoffRelayPin 12
#define sonoffGPIOPin 14
#define sonoffLED 13


// START OF USER VARIABLES

// Send 0 or 1 to this topic to control the Sonoff.
char* deviceControlTopic = "switch/bathroom_mirror"; 

// The Sonoff will publish its state (0 or 1) on this topic.
char* deviceStateTopic = "switch/bathroom_mirror/state"; 

// This message will be sent to the topic "automation" when the switch is toggled twice. 
char* deviceAutomationPayload = "mainBathroomLights";

// The amount of time (in milliseconds) to wait for the switch to be toggled again. 
// 300 works well for me and is barely noticable. Set to 0 if you don't intend to use this functionality.
int specialFunctionTimeout = 300; 

// Wifi Variables
char* SSID = "<REDACTED>";
char* WiFiPassword = "<REDACTED>";

// MQTT Variables
const char* mqtt_server = "<REDACTED>";
int mqtt_port = <REDACTED>;
const char* mqttUser = "<REDACTED>";
const char* mqttPass = "<REDACTED>";

// END OF USER VARIABLES


WiFiClient espClient;
PubSubClient client(espClient);

// Reconnect Variables
unsigned long reconnectStart = 0;
unsigned long lastReconnectMessage = 0;
unsigned long messageInterval = 1000;
int currentReconnectStep = 0;
boolean offlineMode = true;

switchedRelay sonoff(sonoffGPIOPin, sonoffRelayPin, deviceStateTopic, deviceControlTopic, deviceAutomationPayload, specialFunctionTimeout);

void reconnect() {
  // IF statements used to complete process in single loop if possible

  // 0 - Turn the LED on and log the reconnect start time
  if (currentReconnectStep == 0) {
    digitalWrite(sonoffLED, LOW);
    reconnectStart = millis();
    currentReconnectStep++;
  }

  // 1 - Check WiFi Connection
  if (currentReconnectStep == 1) {
    if (WiFi.status() != WL_CONNECTED) {
      if ((millis() - lastReconnectMessage) > messageInterval) {
        Serial.print("Awaiting WiFi Connection (");
        Serial.print((millis() - reconnectStart) / 1000);
        Serial.println("s)");
        lastReconnectMessage = millis();
      }
    }
    else {
      Serial.println("WiFi connected!");
      Serial.print("SSID: ");
      Serial.print(WiFi.SSID());
      Serial.println("");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println("");

      lastReconnectMessage = 0;
      currentReconnectStep = 2;
    }
  }

  // 2 - Check MQTT Connection
  if (currentReconnectStep == 2) {
    if (!client.connected()) {
      if ((millis() - lastReconnectMessage) > messageInterval) {
        Serial.print("Awaiting MQTT Connection (");
        Serial.print((millis() - reconnectStart) / 1000);
        Serial.println("s)");
        lastReconnectMessage = millis();

        String clientId = "ESP8266Client-";
        clientId += String(random(0xffff), HEX);
        client.connect(clientId.c_str(), mqttUser, mqttPass);
      }

      // Check the MQTT again and go forward if necessary
      if (client.connected()) {
        Serial.println("MQTT connected!");
        Serial.println("");

        lastReconnectMessage = 0;
        currentReconnectStep = 3;
      }

      // Check the WiFi again and go back if necessary
      else if (WiFi.status() != WL_CONNECTED) {
        currentReconnectStep = 1;
      }

      // If we're not in offlineMode and we've been trying to connect for more than 2 minutes then restart the ESP
      else if (offlineMode == false && (millis() - reconnectStart) > 120000) {
        ESP.restart();
      }
    }
    else {
      Serial.println("MQTT connected!");
      Serial.println("");

      lastReconnectMessage = 0;
      currentReconnectStep = 3;
    }
  }

  // 3 - All connected, turn the LED back on, subscribe to the MQTT topics and then set offlineMode to false
  if (currentReconnectStep == 3) {
    digitalWrite(sonoffLED, HIGH);  // Turn the LED off

    client.subscribe(sonoff.deviceControlTopic);
    client.subscribe(sonoff.deviceStateTopic);

    if (offlineMode == true) {
      offlineMode = false;
      Serial.println("Offline Mode Deactivated");
      Serial.println("");
    }

    currentReconnectStep = 0; // Reset
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("------------------- MQTT Recieved -------------------");

  if (((String(sonoff.deviceControlTopic).equals(topic)) && (length == 1)) | (String(sonoff.deviceStateTopic).equals(topic) && sonoff.recovered == false)) {
    if ((char)payload[0] == '0') {
      sonoff.switchDevice(0);
    }
    else if ((char)payload[0] == '1') {
      sonoff.switchDevice(1);
    }

    sonoff.recovered = true;
  }
}

void mqttBus() {
  if (sonoff.newStatusMessage == true) {
    client.publish(sonoff.deviceStateTopic, sonoff.newPayload, true);
    sonoff.newStatusMessage = false;
  }
  if (sonoff.newSpecialMessage == true) {
    client.publish("automation", sonoff.switchSpecialPayload);
    sonoff.newSpecialMessage = false;
  }
}

void setup() {
  // Serial
  Serial.begin(115200);
  Serial.println("");
  Serial.println("");

  // LED
  pinMode(sonoffLED, OUTPUT);     // Initialize the sonoffLED pin as an output
  digitalWrite(sonoffLED, LOW);   // Turn the LED on to indicate no connection

                    // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WiFiPassword);

  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  else {
    client.loop();
    mqttBus();
  }

  sonoff.switchCheck();
}
