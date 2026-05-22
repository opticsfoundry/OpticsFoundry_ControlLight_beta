#pragma once

#include "CDevice.h"


class CDeviceDigitalOut : public CDevice
{
public:
	unsigned int numberChannels;
	uint16_t LastValue;
public:
	CDeviceDigitalOut(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		unsigned int _numberChannels
	);
	virtual ~CDeviceDigitalOut() = default;
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();

	virtual bool SetDigitalOutput(uint8_t BitNr, bool OnOff) {
		uint8_t Data = (OnOff) ? 1 : 0;
		return SetValue(0, (uint8_t*) &Data, 1, BitNr);
	}
};
