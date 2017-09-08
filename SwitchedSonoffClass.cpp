#include <Arduino.h>
#include <PubSubClient.h>
#include "SwitchedSonoffClass.h"

switchedRelay::switchedRelay(int switchPin_, int relayPin_, char* deviceStateTopic_, char* deviceControlTopic_, char* switchSpecialPayload_, unsigned long switchIntervalTimeout_)
{
	// Switch Variables
	switchPin = switchPin_;
	switchState = 0;
	prevSwitchState = 0;
	switchIntervalTimeout = switchIntervalTimeout_; // 300
	switchCount = 0;
	lastStateChange = 0;
	debounceDelay = 50;

	// Relay Variables
	relayPin = relayPin_;
	relayState = 0;

	// MQTT Variables
	newStatusMessage = false;
	newSpecialMessage = false;
	deviceStateTopic = deviceStateTopic_;
	deviceControlTopic = deviceControlTopic_;
	switchSpecialPayload = switchSpecialPayload_;

	// Relay Setup
	pinMode(relayPin, OUTPUT);      // Initialize the relayPin pin as an output
	digitalWrite(relayPin, LOW);    // Turn device off as default

	// Switch Setup
	pinMode(switchPin, INPUT);                  // Initialize the switchPin as an input
	prevSwitchState = digitalRead(switchPin);   // Populate prevSwitchState with the startup state

	// Recovery
	recovered = false;
}

void switchedRelay::switchDevice(boolean targetState) {
	// Off
	if (targetState == 0) {
		digitalWrite(relayPin, LOW); // Turn the relay off
		newStatusMessage = true;
		newPayload = "0";
		relayState = 0; // Keep a local record of the current state
	}
	// On
	else if (targetState == 1) {
		digitalWrite(relayPin, HIGH); // Turn the relay off
		newStatusMessage = true;
		newPayload = "1";
		relayState = 1; // Keep a local record of the current state
	}
}

void switchedRelay::switchCheck() {
	// Lap the timer
	switchIntervalFinish = millis();

	// Calculate the elapsed time, set to 0 if it's the first press
	if (switchCount == 0) {
		switchIntervalElapsed = 0;
	}
	else {
		switchIntervalElapsed = switchIntervalFinish - switchIntervalStart;
	}

	// Check for a change in state, if changed loop for the deboucneDelay to check it's real
	switchState = digitalRead(switchPin);
	if (switchState != prevSwitchState) {
		unsigned long loopStart = millis();
		int supportive = 0;
		int unsupportive = 0;

		// Loop
		while ((millis() - loopStart) < debounceDelay) {
			switchState = digitalRead(switchPin);
			if (switchState != prevSwitchState) {
				supportive++;
			}
			else {
				unsupportive++;
			}
		}

		// Calculate the findings and report
		if (supportive > unsupportive) {
			switchIntervalStart = millis();
			lastStateChange = millis();
			switchCount += 1;
			prevSwitchState = switchState;
		}
	}

	// If the elapsed time is greater that the timeout and the count is higher than 0, do stuff and reset the count
	if (switchIntervalElapsed > switchIntervalTimeout && switchCount > 0) {
		// If the count is odd, toggle the light 
		if ((switchCount % 2) != 0) {
			if (relayState == 0) {
				switchDevice(1);
			}
			else {
				switchDevice(0);
			}
		}

		// If the count is 2, send the special command
		else if (switchCount == 2) {
			newSpecialMessage = true;
		}

		// If the count is 6, restart the ESP
		else if (switchCount == 6) {
			digitalWrite(relayPin, LOW);
			delay(300);
			digitalWrite(relayPin, HIGH);
			delay(300);
			digitalWrite(relayPin, LOW);
			delay(300);
			digitalWrite(relayPin, HIGH);
			delay(300);
			digitalWrite(relayPin, LOW);
			ESP.restart();
		}

		// Reset the count
		switchCount = 0;
	}
}
