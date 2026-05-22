#pragma once

#include <cstdint>

class CDeviceSequencer;

const unsigned int AD9858MaxValues=18;	
const unsigned int AD9858MaxBusBuffer=256;

const uint8_t AD9858ModeSingleTone=0;
const uint8_t AD9858ModeFrequencyShiftKeying=1;
const uint8_t AD9858ModeRampedFrequencyShiftKeying=2;
const uint8_t AD9858ModeChirp=3;
const uint8_t AD9858ModeBiphaseShiftKeying=4;

class CAD9858 
{
private:
	unsigned long ValueInBusBuffer[AD9858MaxValues]; //stores Bus buffer address of next value to be written for future update
	//Ring buffer for bus writing
	//stores ValueNr of next Value to be written
	uint8_t BusBuffer[AD9858MaxBusBuffer]; 
	unsigned long BusBufferStart;
	unsigned long BusBufferEnd;
	unsigned long BusBufferLength;

	int64_t AktValueContentsWritten;
	uint8_t AktValueNrWritten;
	uint8_t AktSubAddressWritten;		
	uint8_t WritePrecision[AD9858MaxValues];

	
	int64_t AktValueContents[AD9858MaxValues]; //keeps track of Value, contains value after bus buffer has been finished to be written out
	double ClockSpeed;
	double InputClockSpeed;
	double MaxFrequency;	
	double FrequencyScale;
	bool UpdateRegistersModeAutomatic;
	unsigned long BaseAddress;
	unsigned short Bus;
public:
	double FrequencyMultiplier;
	bool PS0;
	bool PS1;
	bool Enabled;
public:		
	CAD9858(unsigned long aBaseAddress, double aExternalClockSpeed, double aFrequencyMultiplier, CDeviceSequencer* _MyDeviceSequencer);
	~CAD9858();
	double SetModulationFrequency(double ModulationFrequency, bool GetValue=false);  //ModulationFrequency in MHz
	void MasterReset();
	void SetUpdateRegistersModeAutomatic();
	void SetUpdateRegistersModeManual();
	bool Set2GHzDividerDisable(bool aDisable, bool GetValue=false);
	bool SetSYNCLKDisable(bool aDisable, bool GetValue=false);
	bool SetMixerPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetPhaseDetectPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetSDIOInputOnly(bool aValue, bool GetValue=false);
	bool SetLSBFirst(bool aValue, bool GetValue=false);
	bool SetFrequencySweepEnable(bool aValue, bool GetValue=false);
	bool SetEnableSineOutput(bool aValue, bool GetValue=false);
	bool SetChargePumpOffset(bool aValue, bool GetValue=false);
	bool SetChargePumpPolarity(bool aValue, bool GetValue=false);
	bool SetAutoClearFrequencyAccumulator(bool aValue, bool GetValue=false);
	bool SetAutoClearPhaseAccumulator(bool aValue, bool GetValue=false);
	bool SetLoadDeltaFrequencyTimer(bool aValue, bool GetValue=false);
	bool SetClearFrequencyAccumulator(bool aValue, bool GetValue=false);
	bool SetClearPhaseAccumulator(bool aValue, bool GetValue=false);
	bool SetFastLockEnable(bool aValue, bool GetValue=false);
	bool SetFTWForFastLock(bool aValue, bool GetValue=false);
	uint8_t SetPhaseDetectorDividerRatioN(uint8_t aValue, bool GetValue=false);
	uint8_t SetPhaseDetectorDividerRatioM(uint8_t aValue, bool GetValue=false);
	uint8_t SetFrequencyDetectModeChargePumpCurrent(uint8_t aValue, bool GetValue=false);
	uint8_t SetFinalClosedLoopModeChargePumpCurrent(uint8_t aValue, bool GetValue=false);
	uint8_t SetWideClosedLoopModeChargePumpCurrent(uint8_t aValue, bool GetValue=false);
	int64_t SetDeltaFrequencyWord(int64_t aDeltaFrequencyWord, bool GetValue=false);
	unsigned long SetRampRateWord(unsigned long aRampRateWord, bool GetValue=false);	
	double SetAttenuation(double aValue, bool GetValue=false);
	double SetFrequency0(double Frequency, bool GetValue=false);
	double SetFrequency1(double Frequency, bool GetValue=false);
	double SetFrequency2(double Frequency, bool GetValue=false);
	double SetFrequency3(double Frequency, bool GetValue=false);
	int64_t SetFrequencyTuningWord0(const uint64_t& FrequencyTuningWord, bool GetValue = false);
	int64_t SetFrequencyTuningWord1(const uint64_t& FrequencyTuningWord, bool GetValue = false);
	int64_t SetFrequencyTuningWord2(const uint64_t& FrequencyTuningWord, bool GetValue = false);
	int64_t SetFrequencyTuningWord3(const uint64_t& FrequencyTuningWord, bool GetValue = false);

	unsigned short SetPhaseOffsetWord0(unsigned short aPhaseOffsetWord, bool GetValue=false);
	unsigned short SetPhaseOffsetWord1(unsigned short aPhaseOffsetWord, bool GetValue=false);
	unsigned short SetPhaseOffsetWord2(unsigned short aPhaseOffsetWord, bool GetValue=false);
	unsigned short SetPhaseOffsetWord3(unsigned short aPhaseOffsetWord, bool GetValue=false);
	bool SetPS0(bool aPS0, bool GetValue=false);
	bool SetPS1(bool aPS1, bool GetValue=false);
	void Enable(bool OnOff);
	uint8_t SetFrequencyWritePrecision(uint8_t Precision, bool GetValue=false);	
	bool WriteToBus();
	void WriteAllToBus();
	virtual bool HasSomethingToWriteToBus() { //inline code for speed
		if (!Enabled) return false;
		if ((BusBufferLength==0) && (AktValueNrWritten==99)) return false;	
		return true;
	}
	void FrequencyRamp(double StartFrequency, double StopFrequency, double RampTime); //Frequencies in MHz, RampTime in ms
	void FrequencyModulation(double StartFrequency, double StopFrequency, double ModulationFrequency); //Frequencies in MHz
private:
	bool SetControlBit(uint8_t RegisterNr, uint8_t BitNr, bool Value, bool GetValue);
	uint8_t SetControlBits(uint8_t RegisterNr, uint8_t LowestBitNr, uint8_t NrBits, uint8_t Value, bool GetValue);
	int64_t SetValue(uint8_t ValueNr, int64_t Value, bool GetValue);
	void LoadLatches();
	void UpdateRegisters();
	CDeviceSequencer* MyDeviceSequencer;
};

