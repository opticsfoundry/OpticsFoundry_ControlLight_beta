#pragma once

#include "CDevice.h"
#include <cmath>

class CAD9858;
class CDeviceAD9858 : public CDevice
{
public:
	double externalClockFrequency;
	double frequencyMultiplier;
public:
	CDeviceAD9858(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		double _externalClockFrequency,
		unsigned int _frequencyMultiplier
	);
	virtual ~CDeviceAD9858();
	virtual bool SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);

	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();

	virtual bool SetFrequency(double Frequency) {
		return SetValue(4, (uint8_t *)(&Frequency), 64, 0);
	}

	virtual bool SetFrequencyTuningWord(uint64_t FrequencyTuningWord) {
		return SetValue(0, (uint8_t*)(&FrequencyTuningWord), 64, 0);
	}

	virtual bool SetPower(double Power) {
		double Intensity = Power;
		if (Intensity < 1E-6) Intensity = 1E-6;
		double Attenuation = 10.0 * (log(0.01 * Intensity) / log(10.0));
		if (Attenuation > 0) Attenuation = 0;
		else if (Attenuation < -32) Attenuation = -32;
		return SetAttenuation(Attenuation);
	}

	virtual bool SetAttenuation(double Power) {
		return SetValue(20, (uint8_t*)(&Power), 64, 0);
	}

private:
	CAD9858* MyAD9858;
};
