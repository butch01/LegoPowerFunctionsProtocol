/*
 * LegoPowerFunctionsProtocol.h
 *
 *  Created on: 10.07.2016
 *      Author: butch
 */

#ifndef LEGOPOWERFUNCTIONSPROTOCOL_H_
#define LEGOPOWERFUNCTIONSPROTOCOL_H_

#include "arduino.h"

#define RED 0
#define BLUE 1

/**
 * This class is processing the lego power function protocol logically. Receiving infrared is out of scope here.
 * LRC checking is out of scope here. It needs to be done in IR receiving part. We need to have valid data here.
 */
class LegoPowerFunctionsProtocol {
public:

	/**
	 * constructor
	 */
	LegoPowerFunctionsProtocol(int channel);



	/* forward new protocol data to protocol parser. Process it. Then you can call the other functions.
	 * return codes:
	 * 0: successfully processed
	 * 1: channel does not match
	 */
	char updateDataAndProcess(unsigned int data);


	byte getChannel(unsigned int data);





	// low level lego fields -> just get it
	byte getChannel();
	byte getEscape();
	byte getMode();
	int  getDataLowLevel();

	char getRedSpeed();
	long getRedSpeedLastChanged();

	char getBlueSpeed();
	long getBlueSpeedLastChanged();

	char getSpeedBySubChannelID(char subChannelId); // 0=red, 1=blue

	long getLastReceivedTime();	// get last received time

	void setTimeoutForSubChannel(int subChannel, long timeoutInMs);	// set different values for timeout of red or blue subchannel
	long getDeltaToTimeout(int subChannel);	// returns timeout for subChannel. <=0 timeout has been reached so many ms ago, >0 -> so many ms left until timeout is reached
	bool isTimeoutReached (int subChannel); // returns true / false, if the timeout for subChannel has been reached yet.


private:

	// vars
	byte channel;			// channel which this protocol handler is listening to.
	byte escape;			// escape bit
	byte mode;				// mode (only available if combo pwm is NOT set by escape bit).
							// -> values: -2: default / not set, -1: Combo PWM Mode, 000: Extended Mode, 001: Combo Direct Mode, 01x Reserved: 1xx Single Output Mode


	int currentData;			// current data, which contains the processed field data.
	char speedRed;				// processed red speed
	bool speedRedBrake;			// true if brake, then float is given
	char speedBlue;				// processed blue speed
	bool speedBlueBrake;		// true if brake, then float is given

	long lastDataReceived; 		// time since start when the last data was processed by this protocol parser
	long timeout[2];			// individual timeout for RED and BLUE


	// functions
	void processComboDirect(int data);
	void ComboDirectSpeedAnalyse(byte* bits, char* speed, bool* brake);	// change the speed
};

#endif /* LEGOPOWERFUNCTIONSPROTOCOL_H_ */
