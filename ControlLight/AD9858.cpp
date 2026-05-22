// AD9858.cpp: implementation of the CAD9858 class.
//
//////////////////////////////////////////////////////////////////////

#include "AD9858.h"
#include "CDeviceSequencer.h"
#include <string>
#include <format>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

unsigned long AD9858MasterResetValueContents[AD9858MaxValues]={0x18,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0};
uint8_t AD9858ValueLength[AD9858MaxValues]=             {   1,   1,   1,   1,   4,   2,   4,   2,   4,   2,   4,   2,   4,   2,   1,   1,   1,   1};
uint8_t AD9858ValueBaseAddress[AD9858MaxValues]=        {0x00,0x01,0x02,0x03,0x04,0x08,0x0A,0x0E,0x10,0x14,0x16,0x1A,0x1C,0x20,0x30,0x31,0x32,0x00};
const unsigned long AD9858ValueNotInBusBuffer=99999;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAD9858::CAD9858(unsigned long aBaseAddress, double aExternalClockSpeed, double aFrequencyMultiplier, 
	CDeviceSequencer* _MyDeviceSequencer)
{
	Bus=0;
	MyDeviceSequencer = _MyDeviceSequencer;
	InputClockSpeed=2*1E6*aExternalClockSpeed; //conversion MHz to Hz
	FrequencyMultiplier=aFrequencyMultiplier;  //This is the external frequency doubler option
	ClockSpeed=InputClockSpeed;
	MaxFrequency=ClockSpeed*0.45E-6;
	//4294967296=2^32
	FrequencyScale=2*4294967296.0*((1.0/FrequencyMultiplier)*1.0E6/ClockSpeed);
	BaseAddress=aBaseAddress;
	//MultiIOAddress=Bus+(uint8_t)BaseAddress*4; //this 
	//MyMultiIO=aMyMultiIO;
	for (int i=0;i<AD9858MaxValues;i++) {
		AktValueContents[i]=AD9858MasterResetValueContents[i];
		ValueInBusBuffer[i]=AD9858ValueNotInBusBuffer;
		WritePrecision[i]=AD9858ValueLength[i];
	}
	for (unsigned int i=0;i<AD9858MaxBusBuffer;i++) BusBuffer[i]=0; 
	BusBufferStart=0;
	BusBufferEnd=0;
	BusBufferLength=0;
	UpdateRegistersModeAutomatic=true;

	AktValueContentsWritten=0;
	AktValueNrWritten=99;
	AktSubAddressWritten=0;
	PS1=false;
	PS0=false;
	/*if (aBaseAddress >= 64) {

		std::string buf = std::format("CAD9858::CAD9858 : BaseAdress too high (BaseAddress={:x} Bus={:x})",BaseAddress,Bus>>BusBitShift);
		AddErrorMessageString(buf);
	}*/
}

CAD9858::~CAD9858()
{

}

bool CAD9858::WriteToBus()
{	
	if (!Enabled) return false;
	if ((BusBufferLength==0) && (AktValueNrWritten==99)) return false;	
	if (AktValueNrWritten==99) {
		AktValueNrWritten=BusBuffer[BusBufferStart];
		AktValueContentsWritten=AktValueContents[AktValueNrWritten];
		int help=AD9858ValueLength[AktValueNrWritten]-WritePrecision[AktValueNrWritten];
		AktSubAddressWritten=(help>0) ? help : 0;
	    ValueInBusBuffer[AktValueNrWritten]=AD9858ValueNotInBusBuffer;
		BusBufferLength--;
		BusBufferStart++;
		if (BusBufferStart>=AD9858MaxBusBuffer) BusBufferStart=0;
	}	
	unsigned short MultiIOAddress=Bus+(uint8_t)BaseAddress*4; //this 	
	//UpdateRegisters: 		 Base: 0x30 Length: 1                ValueNr: x+128
	//MasterReset:			 Base: 0x31 Length: 1                ValueNr: 15
	//LoadLatches:           Base: 0x32 Length: 1                ValueNr: 16 
	//Load data to buffer for all other ValueNr
	
	if (AktValueNrWritten==14) {
		MultiIOAddress+=2; //Update output register
	} 
	else if (AktValueNrWritten==17) MultiIOAddress+=3;//Attenuator
	else if (AktValueNrWritten==15) ;//MultiIOAddress+=0; //Master reset
	else if (AktValueNrWritten==16) MultiIOAddress+=3; //no additional strobe
	else MultiIOAddress += 1;	//Load data to buffer
	uint8_t AktRegisterWritten=AD9858ValueBaseAddress[AktValueNrWritten]+AktSubAddressWritten;
	if (PS1) AktRegisterWritten+=0x80;	
	if (PS0) AktRegisterWritten+=0x40;			
	uint8_t AktByteWritten=(uint8_t)((AktValueContentsWritten >> (8*AktSubAddressWritten)) & 0xFF);
	AktSubAddressWritten++;
	if (AktSubAddressWritten>=AD9858ValueLength[AktValueNrWritten]) {
		//we produce a "update registers" signal during the same 
		//bus cycle as the last "Load data into IO buffer" signal and don´t need an additional
		//bus cycle just for that
		
		if ((AktValueNrWritten != 15) && (AktValueNrWritten != 16) && (AktValueNrWritten != 17)) {
			MultiIOAddress += 1; //Update output register
		}
		AktValueNrWritten=99;
	}
	MyDeviceSequencer->WriteBusAddressAndDataToBuffer(MultiIOAddress,AktRegisterWritten + (AktByteWritten<<8));
	return true;
}

void CAD9858::WriteAllToBus()
{
	while (WriteToBus()) {
	}
}

double CAD9858::SetModulationFrequency(double ModulationFrequency, bool GetValue) {  //ModulationFrequency in MHz
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord=0;
	if (GetValue) {
		 FrequencyTuningWord=SetDeltaFrequencyWord(FrequencyTuningWord,true);
		 //Convert back from 32 bit twos complement
		 //4294967296=2^32
		 FrequencyTuningWord=4294967296-FrequencyTuningWord+1;
		 ModulationFrequency=((AktValueContents[8]-AktValueContents[6])*(FrequencyTuningWord/((AktValueContents[5]+1)*ClockSpeed)));
		 return ModulationFrequency/1E6;
	} else {
		if (ModulationFrequency<0) {
			std::string buf = std::format("CAD9858::SetModulationFrequency : modulation frequency out of range (BaseAddress={:x}) fmodwanted={} Hz",BaseAddress,ModulationFrequency);
			AddErrorMessage(buf);
			ModulationFrequency=1;
		}
						
		FrequencyTuningWord=(int64_t)((AktValueContents[8]-AktValueContents[6])*((ModulationFrequency*1E6)/(0.5*ClockSpeed/(AktValueContents[5]+1))));
		if (FrequencyTuningWord>(4294967296/2)) {
			std::string buf = std::format("CAD9858::SetModulationFrequency : frequency out of range (BaseAddress={:x}) fwanted={} MHz",BaseAddress,ModulationFrequency);
			AddErrorMessage(buf);
		}
		//Calculate 32bit twos complement
		//4294967296=2^32
		FrequencyTuningWord=(0x100000000-FrequencyTuningWord) & 0xFFFFFFFF;
		SetDeltaFrequencyWord(FrequencyTuningWord,false);
		return ModulationFrequency;
	}
}

int64_t CAD9858::SetValue(uint8_t ValueNr, int64_t Value, bool GetValue)
{
	if (!Enabled) return 0;
	if (ValueNr>=AD9858MaxValues) return 0;
	if (GetValue) {
		return AktValueContents[ValueNr];
	} else {
		if (AktValueContents[ValueNr]!=Value) {
			if (BusBufferLength>=AD9858MaxBusBuffer) {
				std::string buf = std::format("CAD9858::SetValue : Bus Buffer exceeded (BaseAddress={:x})",BaseAddress);
				AddErrorMessage(buf);
				return 0;
			}			
			if (ValueInBusBuffer[ValueNr]!=AD9858ValueNotInBusBuffer) {
				AktValueContents[ValueNr]=Value;				
			} else {
				AktValueContents[ValueNr]=Value;
				BusBuffer[BusBufferEnd]=ValueNr;
				ValueInBusBuffer[ValueNr]=BusBufferEnd;
				BusBufferLength++;
				BusBufferEnd++;
				if (BusBufferEnd>=AD9858MaxBusBuffer) BusBufferEnd=0;			
				if (UpdateRegistersModeAutomatic) UpdateRegisters();
			}
		}
	}
	return 0;
}

void CAD9858::MasterReset() {
	if (!Enabled) return;
	if (BusBufferLength>=AD9858MaxBusBuffer) {
		std::string buf = std::format("CAD9858::SetValue : Bus Buffer exceeded (BaseAddress={:x})",BaseAddress);
		AddErrorMessage(buf);
		return;
	}	

	for (int i=0;i<AD9858MaxValues;i++) {
		AktValueContents[i]=AD9858MasterResetValueContents[i];
		ValueInBusBuffer[i]=AD9858ValueNotInBusBuffer;
		WritePrecision[i]=AD9858ValueLength[i];
	}
	for (unsigned int i=0;i<AD9858MaxBusBuffer;i++) BusBuffer[i]=0; 
	AktValueContentsWritten=0;
	AktValueNrWritten=99;
	AktSubAddressWritten=0;
	PS1=false;
	PS0=false;	

	//delete also buffer
	BusBufferStart=0;
	BusBufferEnd=0;
	BusBufferLength=0;
	BusBuffer[BusBufferEnd]=15;	//MasterReset: BaseAddress*4+0
	BusBufferLength++;
	BusBufferEnd++;			
	if (BusBufferEnd>=AD9858MaxBusBuffer) BusBufferEnd=0;			
}

void CAD9858::UpdateRegisters() {
	if (!Enabled) return;
	if (BusBufferLength>=AD9858MaxBusBuffer) {
		std::string buf = std::format("CAD9858::UpdateRegisters : Bus Buffer exceeded (BaseAddress={:x})",BaseAddress);
		AddErrorMessage(buf);
		return;
	}		
}

void CAD9858::LoadLatches() {
	if (!Enabled) return;
	if (BusBufferLength>=AD9858MaxBusBuffer) {
		std::string buf = std::format("CAD9858::LoadLatches : Bus Buffer exceeded (BaseAddress={:x})",BaseAddress);
		AddErrorMessage(buf);
		return;
	}	
	BusBuffer[BusBufferEnd]=16;	
	BusBufferLength++;
	BusBufferEnd++;
	if (BusBufferEnd>=AD9858MaxBusBuffer) BusBufferEnd=0;			
}

void CAD9858::SetUpdateRegistersModeAutomatic() {
	if (!Enabled) return;
	UpdateRegistersModeAutomatic=true;
}

void CAD9858::SetUpdateRegistersModeManual() {
	if (!Enabled) return;
	UpdateRegistersModeAutomatic=false;
}

//Registers:
bool CAD9858::Set2GHzDividerDisable(bool aDisable, bool GetValue) {
	return SetControlBit(0,6,aDisable,GetValue);	
}

bool CAD9858::SetSYNCLKDisable(bool aDisable, bool GetValue) {
	return SetControlBit(0,5,aDisable,GetValue);	
}

bool CAD9858::SetMixerPowerDown(bool aPowerDown, bool GetValue) {
	return SetControlBit(0,4,aPowerDown,GetValue);	
}

bool CAD9858::SetPhaseDetectPowerDown(bool aPowerDown, bool GetValue) {
	return SetControlBit(0,3,aPowerDown,GetValue);	
}

bool CAD9858::SetPowerDown(bool aPowerDown, bool GetValue) {
	return SetControlBit(0,2,aPowerDown,GetValue);	
}

bool CAD9858::SetSDIOInputOnly(bool aValue, bool GetValue) {
	return SetControlBit(0,1,aValue,GetValue);	
}

bool CAD9858::SetLSBFirst(bool aValue, bool GetValue) {
	return SetControlBit(0,0,aValue,GetValue);	
}

bool CAD9858::SetFrequencySweepEnable(bool aValue, bool GetValue) {
	return SetControlBit(1,7,aValue,GetValue);	
}

bool CAD9858::SetEnableSineOutput(bool aValue, bool GetValue) {
	return SetControlBit(1,6,aValue,GetValue);	
}

bool CAD9858::SetChargePumpOffset(bool aValue, bool GetValue) {
	return SetControlBit(1,5,aValue,GetValue);	
}

bool CAD9858::SetChargePumpPolarity(bool aValue, bool GetValue) {
	return SetControlBit(1,2,aValue,GetValue);	
}

bool CAD9858::SetAutoClearFrequencyAccumulator(bool aValue, bool GetValue) {
	return SetControlBit(2,7,aValue,GetValue);	
}

bool CAD9858::SetAutoClearPhaseAccumulator(bool aValue, bool GetValue) {
	return SetControlBit(2,6,aValue,GetValue);	
}

bool CAD9858::SetLoadDeltaFrequencyTimer(bool aValue, bool GetValue) {
	return SetControlBit(2,5,aValue,GetValue);	
}

bool CAD9858::SetClearFrequencyAccumulator(bool aValue, bool GetValue) {
	return SetControlBit(2,4,aValue,GetValue);	
}

bool CAD9858::SetClearPhaseAccumulator(bool aValue, bool GetValue) {
	return SetControlBit(2,3,aValue,GetValue);	
}

bool CAD9858::SetFastLockEnable(bool aValue, bool GetValue) {
	return SetControlBit(2,1,aValue,GetValue);	
}

bool CAD9858::SetFTWForFastLock(bool aValue, bool GetValue) {
	return SetControlBit(2,0,aValue,GetValue);	
}

double CAD9858::SetAttenuation(double aValue, bool GetValue) {
	if (GetValue) {
		uint8_t AktValue=SetValue(17,0,GetValue);
		return -AktValue*0.25;
	} else {
		//only debug: only one bit high
		//uint8_t DigValue=1 << ((uint8_t)(aValue));
		//if (aValue<0) DigValue=0;
		//normal code:
		if (aValue<-31.75) aValue=-31.75;
		else if (aValue>0) aValue=0;
		uint8_t DigValue=-aValue/0.25;
		SetValue(17,DigValue,GetValue);	
		return aValue;
	}
}

uint8_t CAD9858::SetPhaseDetectorDividerRatioN(uint8_t aValue, bool GetValue) {
	return SetControlBits(1,3,2,aValue,GetValue);	
}

uint8_t CAD9858::SetPhaseDetectorDividerRatioM(uint8_t aValue, bool GetValue) {
	return SetControlBits(1,0,2,aValue,GetValue);	
}

uint8_t CAD9858::SetFrequencyDetectModeChargePumpCurrent(uint8_t aValue, bool GetValue) {
	return SetControlBits(3,6,2,aValue,GetValue);	
}

uint8_t CAD9858::SetFinalClosedLoopModeChargePumpCurrent(uint8_t aValue, bool GetValue) {
	return SetControlBits(3,3,3,aValue,GetValue);	
}

uint8_t CAD9858::SetWideClosedLoopModeChargePumpCurrent(uint8_t aValue, bool GetValue) {
	return SetControlBits(3,0,3,aValue,GetValue);	
}

int64_t CAD9858::SetDeltaFrequencyWord(int64_t aDeltaFrequencyWord, bool GetValue) {
	return SetValue(4,aDeltaFrequencyWord,GetValue);
}

unsigned long CAD9858::SetRampRateWord(unsigned long aRampRateWord, bool GetValue) {	
	return (unsigned long)SetValue(5,aRampRateWord,GetValue);
}

//Frequency in MHz, ClockSpeed in Hz
double CAD9858::SetFrequency0(double Frequency, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord0=0;
	if (GetValue) {
		 FrequencyTuningWord0=SetValue(6,FrequencyTuningWord0,true);
		 return FrequencyMultiplier*((FrequencyTuningWord0*ClockSpeed)/0x100000000)/1E6;
	} else {
		if (Frequency>MaxFrequency) {
			std::string buf = std::format("CAD9858::SetFrequency0 : frequency out of range (BaseAddress={:x}) fwanted={} MHz fmax={} MHz",BaseAddress,Frequency,MaxFrequency);
			AddErrorMessage(buf);
		}		
		FrequencyTuningWord0=(int64_t)(Frequency*FrequencyScale + 0.5);
		SetValue(6,FrequencyTuningWord0,false);
		return Frequency;
	}
}

//Frequency in MHz, ClockSpeed in Hz
double CAD9858::SetFrequency1(double Frequency, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord1=0;
	if (GetValue) {
		 FrequencyTuningWord1=SetValue(8,FrequencyTuningWord1,true);
		 return FrequencyMultiplier*((FrequencyTuningWord1*ClockSpeed)/0x100000000)/1E6;
	} else {
		if (Frequency>MaxFrequency) {
			std::string buf = std::format("CAD9858::SetFrequency1 : frequency out of range (BaseAddress={:x}) fwanted={} MHz fmax={} MHz",BaseAddress,Frequency,MaxFrequency);
			AddErrorMessage(buf);
		}		
		FrequencyTuningWord1=(int64_t)(Frequency*FrequencyScale + 0.5);
		SetValue(8,FrequencyTuningWord1,false);
		return Frequency;
	}
}

//Frequency in MHz, ClockSpeed in Hz
double CAD9858::SetFrequency2(double Frequency, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord2=0;
	if (GetValue) {
		 FrequencyTuningWord2=SetValue(10,FrequencyTuningWord2,true);
		 return FrequencyMultiplier*((FrequencyTuningWord2*ClockSpeed)/0x100000000)/1E6;
	} else {
		if (Frequency>MaxFrequency) {
			std::string buf = std::format("CAD9858::SetFrequency2 : frequency out of range (BaseAddress={:x}) fwanted={} MHz fmax={} MHz",BaseAddress,Frequency,MaxFrequency);
			AddErrorMessage(buf);
		}		
		FrequencyTuningWord2=(int64_t)(Frequency*FrequencyScale + 0.5);
		SetValue(10,FrequencyTuningWord2,false);
		return Frequency;
	}
}

//Frequency in MHz, ClockSpeed in Hz
double CAD9858::SetFrequency3(double Frequency, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord3=0;
	if (GetValue) {
		 FrequencyTuningWord3=SetValue(12,FrequencyTuningWord3,true);
		 return FrequencyMultiplier*((FrequencyTuningWord3*ClockSpeed)/0x100000000)/1E6;
	} else {
		if (Frequency>MaxFrequency) {			

			std::string buf = std::format("CAD9858::SetFrequency3 : frequency out of range (BaseAddress={:x}) fwanted={} MHz fmax={} MHz",BaseAddress,Frequency,MaxFrequency);
			AddErrorMessage(buf);
		}		
		FrequencyTuningWord3=(int64_t)(Frequency*FrequencyScale + 0.5);
		SetValue(12,FrequencyTuningWord3,false);
		return Frequency;
	}
}

int64_t CAD9858::SetFrequencyTuningWord0(const uint64_t& FrequencyTuningWord, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord0 = 0;
	if (GetValue) {
		FrequencyTuningWord0 = SetValue(6, FrequencyTuningWord0, true);
		return FrequencyTuningWord0;
	}
	else {

		FrequencyTuningWord0 = FrequencyTuningWord;
		SetValue(6, FrequencyTuningWord0, false);
		return FrequencyTuningWord0;
	}
}

int64_t CAD9858::SetFrequencyTuningWord1(const uint64_t& FrequencyTuningWord, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord1 = 0;
	if (GetValue) {
		FrequencyTuningWord1 = SetValue(8, FrequencyTuningWord1, true);
		return FrequencyTuningWord1;
	}
	else {

		FrequencyTuningWord1 = FrequencyTuningWord;
		SetValue(8, FrequencyTuningWord1, false);
		return FrequencyTuningWord1;
	}
}

int64_t CAD9858::SetFrequencyTuningWord2(const uint64_t& FrequencyTuningWord, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord2 = 0;
	if (GetValue) {
		FrequencyTuningWord2 = SetValue(10, FrequencyTuningWord2, true);
		return FrequencyTuningWord2;
	}
	else {
		FrequencyTuningWord2 = FrequencyTuningWord;
		SetValue(10, FrequencyTuningWord2, false);
		return FrequencyTuningWord2;
	}
}

int64_t CAD9858::SetFrequencyTuningWord3(const uint64_t& FrequencyTuningWord, bool GetValue) {
	if (!Enabled) return 0;
	int64_t FrequencyTuningWord3 = 0;
	if (GetValue) {
		FrequencyTuningWord3 = SetValue(12, FrequencyTuningWord3, true);
		return FrequencyTuningWord3;
	}
	else {
		FrequencyTuningWord3 = FrequencyTuningWord;
		SetValue(12, FrequencyTuningWord3, false);
		return FrequencyTuningWord3;
	}
}



unsigned short CAD9858::SetPhaseOffsetWord0(unsigned short aPhaseOffsetWord, bool GetValue) {
	return (unsigned short)SetValue(7,aPhaseOffsetWord,GetValue);
}

unsigned short CAD9858::SetPhaseOffsetWord1(unsigned short aPhaseOffsetWord, bool GetValue) {
	return (unsigned short)SetValue(9,aPhaseOffsetWord,GetValue);
}

unsigned short CAD9858::SetPhaseOffsetWord2(unsigned short aPhaseOffsetWord, bool GetValue) {
	return (unsigned short)SetValue(11,aPhaseOffsetWord,GetValue);
}

unsigned short CAD9858::SetPhaseOffsetWord3(unsigned short aPhaseOffsetWord, bool GetValue) {
	return (unsigned short)SetValue(13,aPhaseOffsetWord,GetValue);
}

bool CAD9858::SetPS0(bool aPS0, bool GetValue) {
	if (!Enabled) return aPS0;
	if (GetValue) return PS0;
	else {
		PS0=aPS0;		
		LoadLatches();
		return aPS0;
	}
}

bool CAD9858::SetPS1(bool aPS1, bool GetValue) {
	if (!Enabled) return aPS1;
	if (GetValue) return PS1;
	else {
		PS1=aPS1;		
		LoadLatches();
		return aPS1;
	}
}

bool CAD9858::SetControlBit(uint8_t RegisterNr, uint8_t BitNr, bool Value, bool GetValue)
{
	if (!Enabled) return false;
	if (GetValue) {
		return ((AktValueContents[RegisterNr] & (1 << BitNr)) > 0);
	} else {
		unsigned long NewValue;
		uint8_t help=(uint8_t)AktValueContents[RegisterNr];
		if (Value) {
			NewValue = help | (1 << BitNr);
		} else {
			NewValue = help &  ~(1 << BitNr);
		}
		SetValue(RegisterNr, NewValue, false);
		return Value;
	}
}

uint8_t CAD9858::SetControlBits(uint8_t RegisterNr, uint8_t LowestBitNr, uint8_t NrBits, uint8_t Value, bool GetValue)
{
	if (!Enabled) return false;
	uint8_t mask=0xFF >> (8-NrBits);
	if (GetValue) {
		return ((AktValueContents[RegisterNr] >> LowestBitNr) & mask);
	} else {
		unsigned long NewValue;
		uint8_t help=(uint8_t)AktValueContents[RegisterNr];
		uint8_t shiftedmask=mask<<LowestBitNr;
		uint8_t invertedmask=~shiftedmask;
		NewValue = (help & invertedmask) | ((Value & mask)<<LowestBitNr);
		SetValue(RegisterNr, NewValue, false);
		return Value;
	}
}

//Precision x corresponds to frequency precision y on 150 MHz signal
// x = y = 150MHz/(256^x)
// 1 = 0.5 MHz
// 2 = 2.2 kHz
// 3 = 8 Hz
// 4 = 0.03 Hz
// 5 = 0.13 mHz
// 6 = 0.5 microHz
uint8_t CAD9858::SetFrequencyWritePrecision(uint8_t Precision, bool GetValue)
{
	if (!Enabled) return Precision;
	if (GetValue) {
		return WritePrecision[6];
	} else {
		if (Precision>4) Precision=4;
		WritePrecision[6]=Precision;
		WritePrecision[8]=Precision;	
		WritePrecision[10]=Precision;
		WritePrecision[12]=Precision;
		return Precision;
	}
}

