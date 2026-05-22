#pragma once

#include "CDevice.h"

class CDeviceAnalogOut :public CDevice
{
public:
	bool signedValue;
	double minVoltage;
	double maxVoltage;
	uint16_t LastValue;
public:
	CDeviceAnalogOut(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		bool _signedValue,
		double _minVoltage,
		double _maxVoltage);
	virtual ~CDeviceAnalogOut() = default;
	bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();
	virtual bool SetVoltage(double Voltage);
};
