/*
 * LegoPowerFunctionsProtocol.cpp
 *
 *  Created on: 10.07.2016
 *      Author: butch
 */

#include "LegoPowerFunctionsProtocol.h"
#include "arduino.h"

#define ESCAPE_MODE 0
#define ESCAPE_COMBO_PWM 1

#define IS_DEBUG_ALL 0
#define IS_DEBUG_PROTOCOL 0
#define IS_DEBUG_TIMEOUT 0

#if IS_DEBUG_ALL
	IS_DEBUG_TIMEOUT 1
	IS_DEBUG_PROTOCOL 1
#endif


#define LEGO_TIMEOUT_DEFAULT 750	// default value for timeout in milliseconds

/**
 * constructor.
 * channel - channel which this protocol handler is listening to.
 * Possible values: 0-3
 */
LegoPowerFunctionsProtocol::LegoPowerFunctionsProtocol(int channel)
{
	// keep channel from constructor
	this -> channel = channel;

	// initialize variables
	this -> currentData = 0;
	this -> speedRed = 0;
	this -> speedRedBrake = false;

	this -> speedBlue = 0;
	this -> speedBlueBrake = false;


	this -> lastDataReceived = millis();
	this -> escape = 0;
	this -> mode = -2;
	this -> timeout[0] = LEGO_TIMEOUT_DEFAULT;	// default values
	this -> timeout[1] = LEGO_TIMEOUT_DEFAULT;

}

/**
 * sets the timeout for RED or BLUE in ms.
 */
void LegoPowerFunctionsProtocol::setTimeoutForSubChannel(int subChannel, long timeoutInMs)
{
	// we have only 2 subchannels. So do it only for id 0 and 1
	if (subChannel == 0 || subChannel == 1)
	{
		this -> timeout[subChannel] = timeoutInMs;
		#if IS_DEBUG_TIMEOUT
			Serial.print("set timeout: subChannel= ");
			Serial.print(subChannel);
			Serial.print(" value=");
			Serial.println(timeoutInMs);
		#endif
	}
}


/**
 * subChannel - BLUE or RED
 * return 	- timeout delta for subChannel in ms.
 * <=0 		- so many ms left until timeout is reached
 * >0 		- timeout has been reached so many ms ago
 */
long LegoPowerFunctionsProtocol::getDeltaToTimeout(int subChannel)
{
	// we have only 2 subchannels. So do it only for id 0 and 1
	long currentTime = millis();
	long delta = currentTime - this -> lastDataReceived - this -> timeout[subChannel];

	#if IS_DEBUG_TIMEOUT
		Serial.print("LegoPowerFunctionsProtocol::getDeltaToTimeout: subChannel= ");
		Serial.print(subChannel);
		Serial.print(" currentTime(");
		Serial.print(currentTime);
		Serial.print(") - lastDataReceived(");
		Serial.print(lastDataReceived);
		Serial.print(") - timeout(");
		Serial.print(timeout[subChannel]);
		Serial.print(") = ");
		Serial.println(delta);
	#endif

	if (subChannel == 0 || subChannel == 1)
	{
		return delta;
	}
	else
	{
		Serial.println("ERROR: LegoPowerFunctionsProtocol::getDeltaToTimeout: requesting wrong subChannel. Valid are only 0 and 1 ");
		return 0;
	}
}

/**
 * subChannel - BLUE or RED
 * returns true / false, if the timeout for subChannel has been reached yet.
 */
bool LegoPowerFunctionsProtocol::isTimeoutReached (int subChannel)
{
	// we have only 2 subchannels. So do it only for id 0 and 1
	if (subChannel == 0 || subChannel == 1)
	{
		if (getDeltaToTimeout(subChannel) <= 0)
		{
			// still time until timeout will be reached
			return false;
		}
		else
		{
			// timeout has already been reached
			return true;
		}
	}
	else
	{
		Serial.println("ERROR: LegoPowerFunctionsProtocol::isTimeoutReached: requesting wrong subChannel. Valid are only 0 and 1 ");
		return false;
	}

}

/**
 * this is a wrapper function for getting getRedSpeed() and getBlueSpeed()
 * blue is identified by 1
 * red is identified by 0
 */
char LegoPowerFunctionsProtocol::getSpeedBySubChannelID(char subChannelId)
{
	if (subChannelId == 0)
	{
		return getRedSpeed();
	}
	else
	{

		if (subChannelId == 1)
		{
			return getBlueSpeed();
		}
		else
		{
			// return speed 0 as default.
			return 0;
		}
	}
}


/**
 * if you received a new data packet via infrared you need to put it into this function to be processed.
 * After this function has been called you can call the other functions to get the (accumulated) fields.
 * The channel of the data needs to be
 */
char LegoPowerFunctionsProtocol::updateDataAndProcess(unsigned int data)
{

	// remember the receive time of the package
	long receiveTime = millis();

	#if IS_DEBUG_PROTOCOL
		Serial.print("LegoPowerFunctionsProtocol::updateDataAndProcess: got data: ");
		Serial.print(data, BIN);
		Serial.print(" " );
		Serial.println(data, HEX);
	#endif

	// check the channel
	if (this -> channel != getChannel(data))
	{
		#if IS_DEBUG_PROTOCOL
			Serial.print("in LegoPowerFunctionsProtocol::updateDataAndProcess: Channel does not match: Listening on ");
			Serial.print(getChannel());
			Serial.print(" but got data has channel ");
			Serial.println(getChannel(data));
		#endif
		// channel does not match -> set returncode 1
		return 1;
	}

	// channel matches -> process

	// get escape bit.
	this -> escape = bitRead(data, 14);

	#if IS_DEBUG_PROTOCOL
		Serial.print("in LegoPowerFunctionsProtocol::updateDataAndProcess: Escape= ");
		Serial.println(getEscape());
	#endif

	// process the mode if escape is  on mode
	if (this -> escape == ESCAPE_COMBO_PWM)
	{
		this -> mode = -1;
	}
	else
	{
		this -> mode = (0x700 & data) >> 8;
	}

	if (this -> mode == 1)
	{
		processComboDirect(data);
	}

	// update the time when we received this package
	this -> lastDataReceived = receiveTime;

	// return without error
	return 0;

}


/**
 * processes the combo direct mode.
 * if digital move is set, it will be set to max speed (7). negative values for backwards.
 *
 */
void LegoPowerFunctionsProtocol::processComboDirect(int data)
{
	// get the 2 bits for output A (red)
	byte red = (byte) ((0x30 & data) >> 4);
	// altering directly this -> speedRed and this -> speedRedBrake
	ComboDirectSpeedAnalyse(&red, &speedRed, &speedRedBrake);

	byte blue =  (byte) ((0xC0 & data) >> 6);
	ComboDirectSpeedAnalyse(&blue, &speedBlue, &speedBlueBrake);


	#if IS_DEBUG_PROTOCOL
		Serial.print("LegoPowerFunctionsProtocol::processComboDirect:  red: O= ");
			Serial.print(red, BIN);
			Serial.print(" S= " );
			Serial.print(speedRed, DEC);
			Serial.print(" B= " );
			Serial.println(speedRedBrake);

		Serial.print("LegoPowerFunctionsProtocol::processComboDirect:  blue: O= ");
			Serial.print(blue, BIN);
			Serial.print(" S= " );
			Serial.print(speedBlue, DEC);
			Serial.print(" B= " );
			Serial.println(speedBlueBrake);
	#endif
}


/**
 * This function is getting the extracted bits for speed of ONE output in ComboDirectMode.
 * It changes the referenced speed and the referenced brake status.
 * All values are referenced by pointers here.
 */
void LegoPowerFunctionsProtocol::ComboDirectSpeedAnalyse(byte* bits, char* speed, bool* brake)
{

	#if IS_DEBUG_ALL
		Serial.print("LegoPowerFunctionsProtocol::ComboDirectSpeedAnalyse: *bits=");
		Serial.print(*bits, DEC);
		Serial.print(" ");
		Serial.println(*bits, BIN);
	#endif

	switch (*bits)
		{
			case 0:
				*speed = 0;
				*brake = false;
				break;

			case 1:
				*speed = 7;
				*brake = false;
				break;

			case 2:
				*speed = -7;
				*brake = false;
				break;

			case 3:
				*speed = 0;
				*brake = true;
				break;
		}
}


/**
 * returns the channel of the given data as byte
 */
byte LegoPowerFunctionsProtocol::getChannel(unsigned int data)
{
	// get channel bytes of nibble 1 and bitshift it
	int channel = (0x3000 & data) >> 12;
	return (byte) channel;
}


/**
 * returns the channel which the protocol handler is listening for
 */
byte LegoPowerFunctionsProtocol::getChannel()
{
	return this -> channel;
}

/**
 * returns the time we last got a valid packet received.
 */
long LegoPowerFunctionsProtocol::getLastReceivedTime()
{
	return this -> lastDataReceived;
}


/**
 * returns the escape byte
 * is 0 if mode is set
 * is 1 if combo pwm is set
 */
byte LegoPowerFunctionsProtocol::getEscape()
{
	return this -> escape;
}


/**
 * returns the speed of blue channel.
 * returns 0 for float. (I do not differ here between "break and then float" and "float")
 * returns negative values for backwards
 * returns positive values for forwards
 * if one of the PWM modes is used speed is between 1 (slowest) and 7 (fastest). + / - used for direction
 * if mode is using direct (digital) input, speed is set to max speed (7 or -7)
 * if mode is using toggle / inc / dec the processed, aggregated speed is given back
 */
char LegoPowerFunctionsProtocol::getBlueSpeed()
{
	return this -> speedBlue;
}

/**
 * returns the speed of blue channel.
 * returns 0 for float. (I do not differ here between "break and then float" and "float")
 * returns negative values for backwards
 * returns positive values for forwards
 * if one of the PWM modes is used speed is between 1 (slowest) and 7 (fastest). + / - used for direction
 * if mode is using direct (digital) input, speed is set to max speed (7 or -7)
 * if mode is using toggle / inc / dec the processed, aggregated speed is given back
 */
char LegoPowerFunctionsProtocol::getRedSpeed()
{
	return this -> speedRed;
}
