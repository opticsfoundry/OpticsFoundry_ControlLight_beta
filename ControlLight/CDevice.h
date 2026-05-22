#pragma once
#include "ControlAPI.h"
#include <string>

class CDeviceSequencer;
class CDevice
{
public:
	CDeviceSequencer* MySequencer;
	unsigned long MyAddress;
	std::string MyType;
private:
	bool _ErrorOccured;
public:
	CDevice(CDeviceSequencer* _MySequencer = nullptr, unsigned long _MyAddress = 0, const std::string _MyType = "");
	virtual ~CDevice() = default;
	void NotifyError(const std::string& ErrorMessage, bool dothrow=true);
	void ClearError();
	bool ErrorOccured() { return this->_ErrorOccured; };

	//In derived classes, SetValue accesses a table of registers (or similar) with specific functions, converts data as needed and translates
	//the call into an appropriate one of SetRegisters.
	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
		AddErrorMessage("CDevice::SetValue: abstract function called");
		return false;
	};

	//In derived classes, SetRegister accesses the registers (or similar) of the specific device, and directly writes to them
	virtual bool SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
		AddErrorMessage("CDevice::SetRegister: abstract function called");
		return SetValue(SubAddress, Data, DataLength_in_bit, StartBit);
	}

public:
	//the following are convenience functions, which allow us to define nice names to the few most important functions
	//They are defined here, such that we can easily handle errors if the wrong device is used for a given function
	

	//Rack
	virtual bool SelectRackSlot(const uint8_t& SlotNr) {
		AddErrorMessage("CDevice::SelectRackSlot: abstract function called");
		return false;
	}

	virtual bool ResetI2CMultiplexer() {
		AddErrorMessage("CDevice::ResetI2CMultiplexer: abstract function called");
		return false; 
	}

	//Analog output
	virtual bool SetVoltage(double Voltage) { 
		AddErrorMessage("CDevice::SetVoltage: abstract function called");
		return false; 
	}

	//Digital output
	virtual bool SetDigitalOutput(uint8_t BitNr, bool OnOff) {
		AddErrorMessage("CDevice::SetDigitalOutput: abstract function called");
		return false;
	}

	//AD9854
	virtual bool SetStartFrequency(double Frequency) {
		AddErrorMessage("CDevice::SetStartFrequency: abstract function called");
		return false;
	}
	virtual bool SetStopFrequency(double Frequency) {
		AddErrorMessage("CDevice::SetStopFrequency: abstract function called");
		return false;
	}
	virtual bool SetModulationFrequency(double Frequency) {
		AddErrorMessage("CDevice::SetModulationFrequency: abstract function called");
		return false;
	}
	virtual bool SetPower(double Power) {
		AddErrorMessage("CDevice::SetPower: abstract function called");
		return false;
	}
	virtual bool SetAttenuation(double Attenuation) {
		AddErrorMessage("CDevice::SetAttenuation: abstract function called");
		return false;
	}


	virtual bool SetStartFrequencyTuningWord(uint64_t)
	{
		AddErrorMessage("CDevice::SetStartFrequencyTuningWord: abstract function called");
		return false;
	}

	virtual bool SetStopFrequencyTuningWord(uint64_t)
	{
		AddErrorMessage("CDevice::SetStopFrequencyTuningWord: abstract function called");
		return false;
	}

	virtual bool SetFSKMode(uint8_t mode) {
		AddErrorMessage("CDevice::SetFSKMode: abstract function called");
		return false;
	}
	virtual bool SetRampRateClock(int rate) {
		AddErrorMessage("CDevice::SetRampRateClock: abstract function called");
		return false;
	}

	virtual bool SetClearACC1(bool) {
		AddErrorMessage("CDevice::SetClearACC1: abstract function called");
		return false;
	}
	virtual bool SetTriangleBit(bool) {
		AddErrorMessage("CDevice::SetTriangleBit: abstract function called");
		return false;
	}
	virtual bool SetFSKBit(bool) {
		AddErrorMessage("CDevice::SetFSKBit: abstract function called");
		return false;
	}


	//AD9858

	virtual bool SetFrequency(double Frequency) {
		AddErrorMessage("CDevice::SetFrequency: abstract function called");
		return false;
	}
	virtual bool SetFrequencyTuningWord(uint64_t FrequencyTuningWord) {
		AddErrorMessage("CDevice::SetFrequencyTuningWord: abstract function called");
		return false;
	}
	/*virtual bool SetPower(double Power) {
		AddErrorMessage("CDevice::SetPower: abstract function called");
		return false;
	}*/
	/*virtual bool SetAttenuation(double Power) {
		AddErrorMessage("CDevice::SetAttenuation: abstract function called");
		return false;
	}*/

	//AD9959

	virtual bool Reset() {
		AddErrorMessage("CDevice::Reset: abstract function called");
		return false;
	}

	virtual bool SetFrequency(uint8_t channel, double Frequency) {
		AddErrorMessage("CDevice::SetFrequency: abstract function called");
		return false;
	}
	virtual bool SetFrequencyTuningWord(uint8_t channel, uint64_t FrequencyTuningWord) {
		AddErrorMessage("CDevice::SetFrequencyTuningWord: abstract function called");
		return false;
	}
	virtual bool SetPhase(uint8_t channel, double Phase) {
		AddErrorMessage("CDevice::SetPhase: abstract function called");
		return false;
	}
	virtual bool SetPower(uint8_t channel, double Power) {
		AddErrorMessage("CDevice::SetPower: abstract function called");
		return false;
	}
	virtual bool SetAttenuation(uint8_t channel, double Power) {
		AddErrorMessage("CDevice::SetAttenuation: abstract function called");
		return false;
	}

	virtual bool SetIOUpdateEnabled(bool _IOUpdateEnabled) {
		AddErrorMessage("CDevice::SetIOUpdateEnabled: abstract function called");
		return false;
	}



};

