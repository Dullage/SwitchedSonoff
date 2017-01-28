## Overview
Allows WiFi and Rocker Switch control of 1 or more relays using an ESP8266 controller and an MQTT broker. Designed to work well with Home Assistant https://home-assistant.io/.

Personally I use this code to control the mains lighting in our house.

As much as I have tried to reduce duplication by creating a class there is still a number of steps that need to be taken when adding more switch / relay combination. For each switch / relay combination you'll need to:

1. Create an instance of the switchedRelay class.
2. Add MQTT subscriptions in the reconnect() function.
3. Add a section in the callback() function to listen for device messages.
4. Add a section in the mqttBus() function to publish device messages.
5. Add a device specific switchDevice() funciton in the loop() function.

*Note: Each step is clearly commented in SwitchedRelay.ino*

## Features
* Works well with Home Assistant.
* Basic control using simple MQTT commands (e.g. "1" to turn on, "0" to turn off).
* Trigger a Home Assistant automation with a double toggle of the switch.
* Restart the ESP with 6 toggles of the switch.
* Plug in as many switches and relays as your ESP will support.
* Utilizes a non-blocking reconnect function allowing switch control to continue if MQTT or WiFi connection is lost.
* Automatic Recovery and Offline Mode (see below)
* Retained Statuses - The state is published as a retained message allowing your hub (e.g. Home Assistant) to grab the current state if it is restarted. Also used for recovery.
* Smart Debounce - Should the ESP detect a switch toggle it will monitor the switch for a further 50ms to ensure it was genuine.

## Dependencies
The following libraries are required and so must be present when compiling.

* Arduino
* ESP8266WiFi
* PubSubClient

## Transmission Codes
| Code | Message |
|---|---|
| 0 | Turn off |
| 1 | Turn on |

## Recovery and Offline Mode
There are a couple of scenarios that needed to be worked around:

**Short-Term Issues** - The ESP8266 can be tricky when trying to detect WiFi disconnections. In a lot of cases my device believed it was connected to WiFi and so was trying to connect to the MQTT broker, in fact it had lost it's WiFi conection and so was stuck. 

**Long-Term Issues** - We also want to be able to carry on using our lights without any WiFi / MQTT connection. I'm not sure my girlfriend would be happy if a router / Raspberry Pi issue meant no lights :-)


**Useful for both scenarios:**
* The switch control continues to operate whilst the ESP is trying to reconnect to either the WiFi or MQTT broker.
* When the ESP is first started it grabs the retained state message for each relay and uses it to recover it's last state. *Note: Recovery won't happen if the ESP is still unable to connect.*

**Useful for short-term issues:**
* A restart of the ESP is triggered if 2 minutes has elapsed without a connection (see below). This solves most short-term issues.

**Useful for long-term issues:**
* The 2 minute restart is not triggered if no connection has been made since the ESP started. The ESP then sits in an Offline Mode until a connection is made. This avoids the lights turning off every 2 minutes during a long-term issue.

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