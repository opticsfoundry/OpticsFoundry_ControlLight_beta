#pragma once

#include "CDevice.h"



class CAD9959;
class CDeviceAD9959 : public CDevice
{
public:
	double externalClockFrequency;
	double frequencyMultiplier;
	bool AD9958;
	double version;

public:
	CDeviceAD9959(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		double _externalClockFrequency,
		unsigned int _frequencyMultiplier,
		bool _AD9958, 
		double _version
	);
	virtual ~CDeviceAD9959();
	virtual bool SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);

	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();
	virtual bool Reset();
	virtual bool SetFrequency(uint8_t channel, double Frequency);
	virtual bool SetFrequencyTuningWord(uint8_t channel, uint64_t FrequencyTuningWord);
	virtual bool SetPhase(uint8_t channel, double Phase);
	virtual bool SetPower(uint8_t channel, double Power);
	virtual bool SetAttenuation(uint8_t channel, double Power);
	virtual bool SetIOUpdateEnabled(bool _IOUpdateEnabled);
private:
	CAD9959* MyAD9959;
};
