#pragma once

#include "CDevice.h"
#include <cmath>

class CAD9852;
class CDeviceAD9854 : public CDevice
{
public:
	double externalClockFrequency;
	uint8_t PLLReferenceMultiplier;
	double frequencyMultiplier;
	unsigned int version;
public:
	CDeviceAD9854(
		CDeviceSequencer* _MySequencer,
		unsigned int _MyAddress,
		unsigned int _version,
		double _externalClockFrequency,
		uint8_t _PLLReferenceMultiplier,
		unsigned int _frequencyMultiplier
	);
	virtual ~CDeviceAD9854();
	virtual bool SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);

	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();



	virtual bool SetStartFrequency(double Frequency) {
		return SetValue(0, (uint8_t *)&Frequency, 64, 0);
	}

	virtual bool SetFrequency(double Frequency) {
		return SetValue(0, (uint8_t*)&Frequency, 64, 0);
	}

	virtual bool SetStopFrequency(double Frequency) {
		return SetValue(1, (uint8_t*)&Frequency, 64, 0);
	}

	virtual bool SetModulationFrequency(double Frequency) {
		return SetValue(3, (uint8_t*)&Frequency, 64, 0);
	}

	virtual bool SetPower(double Power) {
		if (Power > 100) Power = 100;
		if (Power < 0) Power = 0;
		//this command programs the output voltage. The voltage is the squareroot of the intensity.
		// 0x0FC0 is the highest recommended voltage. The /10.0 is due to the % scale of the intensity.
		unsigned int Voltage = (unsigned int)((0x0FC0 / 10.0) * sqrt(Power));
		return SetValue(3, (uint8_t*)&Voltage, 16, 0);

	}
	virtual bool SetAttenuation(double Attenuation) {
		//Attenuation is given in negative dB values. 0dB is max intensity
		//AD9852 requires Intensity given linearly between 0 and 0x0FC0
		//AD9852 needs voltage amplitude. We speak of power attenuation 
		//--> factor 2 difference in dB scale
		if (Attenuation > 0) Attenuation = 0;
		double Voltage = (unsigned int)(0x0FC0 * pow(10.0, Attenuation / 20.0));
		return SetValue(3, (uint8_t*)&Voltage, 16, 0);
	}

	virtual bool SetStartFrequencyTuningWord(uint64_t FrequencyTuningWord)
	{
		return SetValue(18, (uint8_t*)&FrequencyTuningWord, 64, 0);
	}

	virtual bool SetFrequencyTuningWord(uint64_t FrequencyTuningWord)
	{
		return SetValue(18, (uint8_t*)&FrequencyTuningWord, 64, 0);
	}

	virtual bool SetStopFrequencyTuningWord(uint64_t FrequencyTuningWord)
	{
		return SetValue(19, (uint8_t*)&FrequencyTuningWord, 64, 0);
	}

	virtual bool SetFSKMode(uint8_t mode) {
		return SetValue(10, (uint8_t*)&mode, 8, 0);
	}

	virtual bool SetRampRateClock(uint8_t rate) {
		return SetValue(4, (uint8_t*)&rate, 32, 0);
	}

	virtual bool SetClearACC1(bool on) {
		uint8_t onoff = (on) ? 1 : 0;
		return SetValue(5, (uint8_t*)&onoff, 8, 0);
	}
	virtual bool SetTriangleBit(bool on) {
		uint8_t onoff = (on) ? 1 : 0;
		return SetValue(11, (uint8_t*)&onoff, 8, 0);
	}
	virtual bool SetFSKBit(bool on) {
		uint8_t onoff = (on) ? 1 : 0;
		return SetValue(16, (uint8_t*)&onoff, 8, 0);
	}



private:
	CAD9852* MyAD9852; //We use the CAD9852 class from the full control system, nearly as is. This is to safe work and remain compatible.
};
