## Overview
Alternative Sonoff firmware with the following benefits:

* Works well with Home Assistant.
* Optionally allows for a switch (e.g. wall light switch) to also control the Sonoff device.
* Basic control using simple MQTT commands (e.g. "1" to turn on, "0" to turn off).
* Trigger a Home Assistant automation with a double toggle of the switch (duel function switches).

Other features:

* Utilizes a non-blocking reconnect function allowing switch control to continue if MQTT or WiFi connection is lost.
* Automatic Recovery and Offline Mode (see below).
* Retained Statuses - The state is published as a retained message allowing your hub (e.g. Home Assistant) to grab the current state if it is restarted. Also used for recovery.
* Smart Debounce - Should the Sonoff detect a switch toggle it will monitor the switch for a further 50ms to ensure it was genuine.
* Restart the Sonoff with 6 toggles of the switch.

## Dependencies
The following libraries are required and so must be present when compiling.

* Arduino
* ESP8266WiFi
* PubSubClient

## Variables
Before flashing to your Sonoff device you will need to update the following variables:

```
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
```

## Installation & Wiring
I won't detail how to flash the Sonoff device with this firmware, there are already [plenty of tutorials on this](http://bfy.tw/DpfC).

The Sonoff device can be wired as usual however if you intend on using a switch you will need to attach some additional wires and resistors between the Sonoff and the switch. An example wiring diagram can be found in the repo (WiringExample.png).


## Transmission Codes
| Code | Message |
|---|---|
| 0 | Turn off |
| 1 | Turn on |

## Recovery and Offline Mode
There are a couple of scenarios that needed to be worked around:

**Short-Term Issues** - The ESP8266 (built into the Sonoff device) can be tricky when trying to detect WiFi disconnections. In a lot of cases my device believed it was connected to WiFi and so was trying to connect to the MQTT broker, in fact it had lost it's WiFi conection and so was stuck. 

**Long-Term Issues** - We also want to be able to carry on using our lights without any WiFi / MQTT connection. I'm not sure my girlfriend would be happy if a router / Raspberry Pi issue meant no lights :-)


**Useful for both scenarios:**
* The switch control continues to operate whilst the Sonoff is trying to reconnect to either the WiFi or MQTT broker.
* When the Sonoff is first started it grabs the retained state message and uses it to recover it's last state. *Note: Recovery won't happen if the Sonoff is still unable to connect.*

**Useful for short-term issues:**
* A restart of the Sonoff is triggered if 2 minutes has elapsed without a connection (see below). This solves most short-term issues.

**Useful for long-term issues:**
* The 2 minute restart is not triggered if no connection has been made since the Sonoff started. The Sonoff then sits in an Offline Mode until a connection is made. This avoids the lights turning off every 2 minutes during a long-term issue.

## Home Assistant Examples
**light:**
Basic control of light.
```
- platform: mqtt
  name: Landing Light
  state_topic: "switch/landing/state"
  command_topic: "switch/landing"
  payload_off: "0"
  payload_on: "1"
  retain: false
- platform: mqtt
  name: Bathroom Lights
  state_topic: "switch/bathroom/state"
  command_topic: "switch/bathroom"
  payload_off: "0"
  payload_on: "1"
  retain: false
```

**automation:**
In my setup the light switch that controls my landing light will also turn all of my downstairs lights on or off when double toggled. This is the automation script I use.
```
# Landing Switch (when downstairs off)
- alias: Landing Switch when Downstaiirs Off (excluding the stairs as they may still be shutting down)
  trigger:
    platform: mqtt
    topic: automation
    payload: "landingSwitch"
  condition:
    condition: or
    conditions:
      - condition: state
        entity_id: group.downstairsExcStairs
        state: 'off'
  action:
    service: scene.turn_on
    entity_id: scene.downstairs_general
    
# Landing Switch (when downstairs on)
- alias: Landing Switch when Downstairs On
  trigger:
    platform: mqtt
    topic: automation
    payload: "landingSwitch"
  condition: 
    condition: state
    entity_id: group.downstairs
    state: 'on'
  action:
    service: scene.turn_on
    entity_id: scene.downstairs_off
```
