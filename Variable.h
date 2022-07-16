/*
 * Variable.h
 *
 *  Created on: 25 ���. 2019 �.
 *      Author: Q-1
 */

#ifndef VARIABLE_H_
#define VARIABLE_H_

/*
 * * ���������� ���������� � ��������� ���� �� ������� ��������� ������ ��� ����������
 * * Error: 'ackPayload' does not name a type	sloeber.ino.cpp	/TX	line 17	C/C++ Problem
 * * Error: 'RcData' does not name a type	sloeber.ino.cpp	/TX	line 17	C/C++ Problem
 * * Error: 'data' was not declared in this scope	pong.h	/TX	line 193	C/C++ Problem
 */

struct RcData {
	byte axis1; // Aileron (Steering for car)
	byte axis2; // Elevator
	byte axis3; // Throttle
	byte axis4; // Rudder
	boolean mode1 = false; // Mode1 (toggle speed limitation)
	boolean mode2 = false; // Mode2 (toggle acc. / dec. limitation)
	byte channel = 0; // Momentary push button
	byte pot1; // Potentiometer
};

// This struct defines data, which are embedded inside the ACK payload
struct ackPayload {
	float vcc; // vehicle vcc voltage
	float batteryVoltage; // vehicle battery voltage
	boolean batteryOk; // the vehicle battery voltage is OK!
	byte channel = 0; //110; // the channel number
};

RcData data;

ackPayload payload;

#endif /* VARIABLE_H_ */
