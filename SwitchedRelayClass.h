#ifndef _SWITCHEDRELAYCLASS_h
#define _SWITCHEDRELAYCLASS_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

class switchedRelay {
public:
	switchedRelay(int switchPin_, int relayPin_, char* deviceStateTopic_, char* deviceControlTopic_, char* switchSpecialPayload_, unsigned long switchIntervalTimeout_);
	void switchDevice(boolean targetState);
	void switchCheck();
	const char* deviceStateTopic;
	const char* deviceControlTopic;
	const char* switchSpecialPayload;
	boolean newStatusMessage;
	boolean newSpecialMessage;
	char* newPayload;
	unsigned long switchIntervalTimeout;
	boolean recovered;
private:
	int switchPin;
	int relayPin;
	boolean switchState;
	boolean prevSwitchState;
	unsigned long switchIntervalStart, switchIntervalFinish, switchIntervalElapsed;
	int switchCount;
	unsigned long lastStateChange;
	unsigned long debounceDelay;
	boolean relayState;
};

#endif