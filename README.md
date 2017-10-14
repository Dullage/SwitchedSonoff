## What's New?

* **You can now configure the Sonoff over Wifi!**
* Removed dependancy on SwitchedRelay class.
* Simpler wiring diagram.
* Tidied code.


## Overview
Alternative Sonoff firmware with the following benefits:

* Works well with Home Assistant.
* Optionally allows for a switch (e.g. wall light switch) to also control the Sonoff device.
* Basic control using simple MQTT commands (e.g. "1" to turn on, "0" to turn off).
* Trigger a Home Assistant automation with a double toggle of the switch (duel function switches).
* Configuration over WiFi, no need to change any code, just flash the Sonoff and you're away.

Other features:

* Utilizes a non-blocking reconnect function allowing switch control to continue if MQTT or WiFi connection is lost.
* Automatic Recovery and Offline Mode (see below).
* Retained Statuses - The state is published as a retained message allowing your hub (e.g. Home Assistant) to grab the current state if it is restarted. Also used for recovery.
* Smart Debounce - Should the Sonoff detect a switch toggle it will monitor the switch for a further 50ms to ensure it was genuine.
* Restart the Sonoff with 6 toggles of the switch.

## Dependencies
The following libraries are required and so must be present when compiling.

* FS
* ArduinoJson
* Arduino
* PubSubClient
* DNSServer
* ESP8266WebServer
* WiFiManager

## Device Variables

There is no need to change any variable in the code. Wifi, MQTT and Other settings can be managed in the web portal.

To access the portal:

1. Power the Sonoff.
2. Hold the button on the Sonoff for more than 2 seconds (you should see the LED blink a few times). 
3. On your phone or computer search for and connect to a WiFi access point called "Sonoff".
4. You should be automatically directed to the captive portal. If not, open a browser and naviagte to 192.168.4.1.
5. Press "Configure WiFi".
6. Select your WiFi network and then complete the rest of the fields.
7. Once finsihed, press "Save".
8. The Sonoff should now connect to your WiFi and MQTT server.

![(ConfigurationScreenshot.png)](https://raw.githubusercontent.com/Dullage/SwitchedSonoff/master/ConfigurationScreenshot.png)

**Control Topic** - Send 0 or 1 to this topic to control the Sonoff.

**State Topic** - The Sonoff will publish its state (0 or 1) on this topic.

**Double Click Payload** - This message will be sent to the topic "automation" when the switch is toggled twice.

**Double Click Timeout** - The amount of time (in milliseconds) to wait for the switch to be toggled again. 300 works well for me and is barely noticable. Set to 0 if you don't intend to use this functionality.

**MQTT Server IP Address** - Your MQTT servers IP address.

**MQTT Server Port** - Your MQTT servers port.

**MQTT User Name** - Your MQTT servers user name.

**MQTT Password** - Your MQTT servers password.


## Installation & Wiring
I won't detail how to flash the Sonoff device with this firmware, there are already [plenty of tutorials on this](http://bfy.tw/DpfC).

*Note: If you flash the Sonoff sucessfully but nothing happens, try flashing the Sonoff with the 'Flash Mode' set to 'DOUT'.*

The Sonoff device can be wired as usual however if you intend on using a switch you will need to attach some additional wires and resistors between the Sonoff and the switch. An example wiring diagram can be found in the repo: ![(WiringExample.png)](https://raw.githubusercontent.com/Dullage/SwitchedSonoff/master/WiringExample.png).


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
