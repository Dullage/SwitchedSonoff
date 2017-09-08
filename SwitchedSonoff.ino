#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "SwitchedRelayClass.h"

// Wifi
WiFiClient espClient;
char* SSID = "<REDACTED>";
char* WiFiPassword = "<REDACTED>";

// MQTT
const char* mqtt_server = "<REDACTED>";
int mqtt_port = <REDACTED>;
const char* mqttUser = "<REDACTED>";
const char* mqttPass = "<REDACTED>";
PubSubClient client(espClient);

// Reconnect Variables
unsigned long reconnectStart = 0;
unsigned long lastReconnectMessage = 0;
unsigned long messageInterval = 1000;
int currentReconnectStep = 0;
boolean offlineMode = true;

/*
Step 1 - Create an instance of the switchedRelay class for each switched relay combination

Variables (in the order they should be declared):
- switchPin_ (The GPIO pin used for the switch)
- relayPin_ (The GPIO pin used for the relay)
- deviceStateTopic_ (The MQTT topic used to publish the state)
- deviceControlTopic_ (The MQTT topic used to control the relay)
- switchSpecialPayload_ (The MQTT payload used to trigger a Home Assistant automation when the switch is double toggled)
- switchIntervalTimeout_ (This is how long to wait before considering a toggle a toggle. It is used so that you can trigger the 
                          automation message without the relay toggling. 300 works well for me and is barely noticable. Set 
						  to 0 if you don't intend to use this functionality.)
*/

switchedRelay landing(4, 14, "switch/landing/state", "switch/landing", "landingSwitch", 300);
switchedRelay bathroom(5, 12, "switch/bathroom/state", "switch/bathroom", "bathroomSwitch", 0);

/* 
End of Step 1
*/

void reconnect() {
	// IF statements used to complete process in single loop if possible

	// 0 - Turn the LED on and log the reconnect start time
	if (currentReconnectStep == 0) {
		digitalWrite(LED_BUILTIN, LOW);
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
		digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off

		/* 
		Step 2 - Subscribe to the control and state topics for each of the combinations.

		Note: We subscribe to the state topics for recovery purposes
		*/

		client.subscribe(landing.deviceControlTopic);
		client.subscribe(landing.deviceStateTopic);
		client.subscribe(bathroom.deviceControlTopic);
		client.subscribe(bathroom.deviceStateTopic);
		
		/*
		End of Step 2
		*/

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

	/*
	Step 3 - Setup a callback for each of the topics. Use the templates provided, simply change the class instance name.

	Note: The first device should start with an 'if', all others should be 'else if'.
	*/

	// landing
	if (((String(landing.deviceControlTopic).equals(topic)) && (length == 1)) | (String(landing.deviceStateTopic).equals(topic) && landing.recovered == false)) {
		if ((char)payload[0] == '0') {
			landing.switchDevice(0);
		}
		else if ((char)payload[0] == '1') {
			landing.switchDevice(1);
		}

		landing.recovered = true;
	}

	// bathroom
	else if (((String(bathroom.deviceControlTopic).equals(topic)) && (length == 1)) | (String(bathroom.deviceStateTopic).equals(topic) && bathroom.recovered == false)) {
		if ((char)payload[0] == '0') {
			bathroom.switchDevice(0);
		}
		else if ((char)payload[0] == '1') {
			bathroom.switchDevice(1);
		}

		bathroom.recovered = true;
	}

	/*
	End of Step 3
	*/
}

void mqttBus() {
	/*
	Step 4 - Using the template below create a set for each device.

	Note: Use 'if's for each so that multiple messages can be picked up in the same loop.
	*/

	// landing
	if (landing.newStatusMessage == true) {
		client.publish(landing.deviceStateTopic, landing.newPayload, true);
		landing.newStatusMessage = false;
	}
	if (landing.newSpecialMessage == true) {
		client.publish("automation", landing.switchSpecialPayload);
		landing.newSpecialMessage = false;
	}

	// bathroom
	if (bathroom.newStatusMessage == true) {
		client.publish(bathroom.deviceStateTopic, bathroom.newPayload, true);
		bathroom.newStatusMessage = false;
	}
	if (bathroom.newSpecialMessage == true) {
		client.publish("automation", bathroom.switchSpecialPayload);
		bathroom.newSpecialMessage = false;
	}

	/*
	End of Step 4
	*/
}

void setup() {
	// Serial
	Serial.begin(115200);
	Serial.println("");
	Serial.println("");

	// LED
	pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
	digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on to indicate no connection

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

	/*
	Step 5 - Add a switchCheck(); function call for each device.
	*/
	
	landing.switchCheck();
	bathroom.switchCheck();

	/*
	End of Step 5
	*/
}
