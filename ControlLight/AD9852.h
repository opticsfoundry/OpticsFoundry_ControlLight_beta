#pragma once

#include <cstdint>

class CDeviceSequencer;

const unsigned int AD9852MaxValues=17;	
const unsigned int AD9852MaxBusBuffer=256;

const unsigned char AD9852ModeSingleTone=0;
const unsigned char AD9852ModeFrequencyShiftKeying=1;
const unsigned char AD9852ModeRampedFrequencyShiftKeying=2;
const unsigned char AD9852ModeChirp=3;
const unsigned char AD9852ModeBiphaseShiftKeying=4;

class CAD9852 
{
private:	
	bool SetControlBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue=false);
	int64_t SetValue(unsigned char ValueNr, int64_t Value, bool GetValue=false);
	double CalculateModulationFrequencyData(int64_t* DeltaFrequencyWord, unsigned int* ClockUpdateSteps);
	//PhaseAdjustRegister1:  Base: 0x00 Length: 2 Default: 0     ValueNr: 0 
	//PhaseAdjustRegister2:  Base: 0x02 Length: 2 Default: 0	 ValueNr: 1
	//FrequencyTuningWord1:  Base: 0x04 Length: 6 Default: 0	 ValueNr: 2
	//FrequencyTuningWord2:  Base: 0x0A Length: 6 Default: 0	 ValueNr: 3
	//DeltaFrequencyWord:    Base: 0x10 Length: 6 Default: 0     ValueNr: 4
	//UpdateClock:			 Base: 0x16 Length: 4 Default: 0x40  ValueNr: 5
	//RampRateClock:		 Base: 0x1A Length: 3 Default: 0     ValueNr: 6
	//OutputShapeKeyMult:	 Base: 0x21 Length: 2 Default: 0     ValueNr: 7
	//OutputShapeKeyRampRate:Base: 0x25 Length: 1 Default: 0     ValueNr: 8
	//ControlDAC:			 Base: 0x26 Length: 2 Default: 0     ValueNr: 9
	//ControlRegister0:		 Base: 0x1D Length: 1 Default: 0x10  ValueNr: 10
	//ControlRegister1:		 Base: 0x1E Length: 1 Default: 0x64  ValueNr: 11
	//ControlRegister2:		 Base: 0x1F Length: 1 Default: 0x01  ValueNr: 12
	//ControlRegister3:		 Base: 0x20 Length: 1 Default: 0x20  ValueNr: 13
	//UpdateRegisters: 		 Base: 0x30 Length: 1                ValueNr: 14
	//MasterReset:			 Base: 0x31 Length: 1                ValueNr: 15
	//LoadLatches:           Base: 0x32 Length: 1                ValueNr: 16 
	//NoValue:                                                   ValueNr: 99
public:		
	unsigned short SetPhaseAdjustRegister1(unsigned short aPhaseAdjustRegister1, bool GetValue=false);
	unsigned short SetPhaseAdjustRegister2(unsigned short aPhaseAdjustRegister2, bool GetValue=false);
	double SetFrequency1(double aFrequencyTuningWord1, bool GetValue=false);
	double SetFrequency2(double aFrequencyTuningWord2, bool GetValue=false);
	double SetFrequency1AsBinary(const uint64_t& FrequencyTuningWord, bool GetValue = false);
	double SetFrequency2AsBinary(const uint64_t& FrequencyTuningWord, bool GetValue = false);
	void UpdateFrequencyValues();
	//void SetWaveformFrequenciesDDSAD9852(unsigned int DDSNumber, double StartFrequency, double StopFrequency, double ModulationFrequency);
	int64_t SetDeltaFrequencyWord(int64_t aDeltaFrequencyWord, bool GetValue=false);
	unsigned long SetUpdateClock(unsigned long aUpdateClock, bool GetValue=false);
	unsigned long SetRampRateClock(unsigned long aRampRateClock, bool GetValue=false);
	unsigned short SetOutputShapeKeyMult(unsigned short aOutputShapeKeyMult, bool GetValue=false);
	unsigned char SetOutputShapeKeyRampRate(unsigned char aOutputShapeKeyRampRate, bool GetValue=false);
	unsigned short SetControlDAC(unsigned short aControlDAC, bool GetValue=false);
	
	bool SetComparatorPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetControlDACPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetDACPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetDigitalPowerDown(bool aPowerDown, bool GetValue=false);
	bool SetPLLRange(bool aPLLRange, bool GetValue=false);
	bool SetBypassPLL(bool aByPassPLL, bool GetValue=false);
	unsigned char SetPLLReferenceMultiplier(unsigned char aPLLReferenceMultiplier, bool GetValue=false);
	bool SetClearACC1(bool aClearACC1, bool GetValue=false);
	bool SetClearACC2(bool aClearACC2, bool GetValue=false);
	bool SetTriangle(bool aTriangle, bool GetValue=false);
	unsigned char GetLastFSKMode();
	unsigned char SetMode(unsigned char aMode, bool GetValue=false);
	bool SetModeSingleTone(bool Set, bool GetValue=false);
	bool SetModeFrequencyShiftKeying(bool Set, bool GetValue=false);
	bool SetModeRampedFrequencyShiftKeying(bool Set, bool GetValue=false);
	bool SetModeModeChirp(bool Set, bool GetValue=false);
	bool SetModeBiphaseShiftKeying(bool Set, bool GetValue=false);

	bool SetExternalUpdateClock(bool aExternalUpdateClock, bool GetValue=false);
	bool SetBypassInverseSinc(bool aBypassInverseSinc, bool GetValue=false);
	bool SetOSKEnable(bool aOSKEnable, bool GetValue=false);
	bool SetOSKInternal(bool aOSKInternal, bool GetValue=false);
	bool SetLSBFirst(bool aLSBFirst, bool GetValue=false);
	bool SetSDOActive(bool aSDOActive, bool GetValue=false);

	bool SetShapedKeying(bool aShapedKeying, bool GetValue=false);	
	bool SetFSK_BPSK_Hold(bool aFSK_BPSK_Hold, bool GetValue=false);
	bool ShapedKeying;
	bool FSK_BPSK_Hold;
	
	void MasterReset();
	void LoadLatches();
	void UpdateRegisters();
	void SetUpdateRegistersModeAutomatic();
	void SetUpdateRegistersModeManual();
	void SetModeRampedFrequencyShiftKeyingConfiguration();
	bool UpdateRegistersModeAutomatic;

	int64_t AktValueContents[AD9852MaxValues]; //keeps track of Value, contains value after bus buffer has been finished to be written out
	
	double ClockSpeed;
	double InputClockSpeed;
	bool PLLEnabled;
	unsigned char PLLReferenceMultiplier;
	double FrequencyMultiplier;
	double MaxFrequency;	
	double FrequencyScale;
private:	
	unsigned long ValueInBusBuffer[AD9852MaxValues]; //stores Bus buffer address of next value to be written for future update
	//Ring buffer for bus writing
	//stores ValueNr of next Value to be written
	unsigned char BusBuffer[AD9852MaxBusBuffer]; 
	unsigned long BusBufferStart;
	unsigned long BusBufferEnd;
	unsigned long BusBufferLength;

	int64_t AktValueContentsWritten;
	unsigned char AktValueNrWritten;
	unsigned char AktSubAddressWritten;		
	unsigned char WritePrecision[AD9852MaxValues];

	double RequestedStartFrequency;
	double RequestedStopFrequency;
	double RequestedModulationFrequency;
	double SortedStartFrequency;
	double SortedStopFrequency;
	double SortedModulationFrequency;
	double LastStartFrequency;
	double LastStopFrequency;
	double LastModulationFrequency;
	unsigned char LastFSKMode;
	bool ForceUpdate;
	bool IndependentUpdate;
	unsigned char AD9852ValueLength[AD9852MaxValues];
	unsigned char AD9852ValueBaseAddress[AD9852MaxValues];
public:	
	void Enable(bool OnOff);
	bool Enabled;
	unsigned char SetFrequencyWritePrecision(unsigned char Precision, bool GetValue=false);	
	void InitPLL();
	bool WriteToBus();
	void WriteAllToBus();
	virtual bool HasSomethingToWriteToBus() { //inline code for speed
		if (!Enabled) return false;
		if ((BusBufferLength==0) && (AktValueNrWritten==99)) return false;	
		return true;
	}
	unsigned long BaseAddress;
	unsigned short Bus;
	CAD9852(unsigned long aBaseAddress, double aExternalClockSpeed, 
		unsigned char aPLLReferenceMultiplier, double aFrequencyMultiplier, 
		CDeviceSequencer* _MyDeviceSequencer, bool aIndependentUpdate);
	virtual ~CAD9852();
	void FrequencyRamp(double StartFrequency, double StopFrequency, double RampTime); //Frequencies in MHz, RampTime in ms
	void FrequencyModulation(double StartFrequency, double StopFrequency, double ModulationFrequency); //Frequencies in MHz
	double SetModulationFrequency(double ModulationFrequency, bool GetValue=false);
	double SetRampTime(double RampTime, bool GetValue=false);
private:
	CDeviceSequencer* MyDeviceSequencer;
};
