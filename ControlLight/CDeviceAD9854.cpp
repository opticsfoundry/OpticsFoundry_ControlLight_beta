
#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAD9854.h"
#include "AD9852.h"


using namespace std;
#include <format>
using namespace std;
#include <string>
using namespace std;
#include <sstream>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif



CDeviceAD9854::CDeviceAD9854(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyAddress,
	unsigned int _version,
	double _externalClockFrequency,
	uint8_t _PLLReferenceMultiplier,
	unsigned int _frequencyMultiplier
) : CDevice(_MySequencer, _MyAddress, "AD9854") {
	externalClockFrequency = _externalClockFrequency;
	version = _version;
	PLLReferenceMultiplier = _PLLReferenceMultiplier;
	frequencyMultiplier = _frequencyMultiplier;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) {
		MySequencer->ParallelBusDeviceList[MyAddress] = this;
		// The AD9854 uses 4 addresses, so we need to reserve the next 3 addresses
		for (unsigned int i = 1; i < 4; i++) {
			MySequencer->ParallelBusDeviceList[MyAddress + i] = ReservedBusAddress;
		}
		MyAD9852 = new CAD9852(MyAddress, externalClockFrequency, PLLReferenceMultiplier, frequencyMultiplier, MySequencer, version > 1);
	}
	else {
		std::ostringstream oss;
		oss << "CDeviceAD9854::CDeviceAD9854: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		MyAD9852 = nullptr;
		return;
	}
}

CDeviceAD9854::~CDeviceAD9854() {
	if (MyAD9852) {
		delete MyAD9852;
		MyAD9852 = nullptr;
	}
}

bool CDeviceAD9854::SetValue(const unsigned int& SubAddress, const uint8_t* data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//ToDo: do range checks
	double* dValueptr = (double*)data;
	double dValue = *dValueptr;
	switch (SubAddress) {
	case 0:if (DataLength_in_bit == 64) MyAD9852->SetFrequency1(dValue); else return false; break;
	case 1:if (DataLength_in_bit == 64) MyAD9852->SetFrequency2(dValue); else return false; break;
	case 2:if (DataLength_in_bit == 64) MyAD9852->SetModulationFrequency(dValue); else return false; break;
	case 3:if (DataLength_in_bit == 16) MyAD9852->SetOutputShapeKeyMult((unsigned short)dValue); else return false; break; //Intensity
	case 4:if (DataLength_in_bit == 32) MyAD9852->SetRampRateClock((unsigned long)dValue); else return false; break;
	case 5:if (DataLength_in_bit == 8) MyAD9852->SetClearACC1(data[0]); else return false; break;
	case 6:if (DataLength_in_bit == 8) MyAD9852->SetClearACC2(data[0]); else return false; break;
	case 7:if (DataLength_in_bit == 16) MyAD9852->SetPhaseAdjustRegister1((unsigned short)dValue); else return false; break;
	case 8:if (DataLength_in_bit == 16) MyAD9852->SetPhaseAdjustRegister2((unsigned short)dValue); else return false; break;
	case 9:if (DataLength_in_bit == 16) MyAD9852->SetControlDAC((unsigned short)dValue); else return false; break;
	case 10:if (DataLength_in_bit == 8) MyAD9852->SetMode((unsigned char)dValue); else return false; break;
	case 11:if (DataLength_in_bit == 8) MyAD9852->SetTriangle(data[0]); else return false; break;
	case 12:if (DataLength_in_bit == 8) MyAD9852->SetOSKEnable(data[0]); else return false; break;
	case 13:if (DataLength_in_bit == 8) MyAD9852->SetOSKInternal(data[0]); else return false; break;
	case 14:if (DataLength_in_bit == 8) MyAD9852->SetOutputShapeKeyRampRate((unsigned char)dValue); else return false; break;
	case 15:if (DataLength_in_bit == 8) MyAD9852->SetShapedKeying(data[0]); else return false; break;
	case 16:if (DataLength_in_bit == 8) MyAD9852->SetFSK_BPSK_Hold(data[0]); else return false; break;
	case 17:if (DataLength_in_bit == 64) MyAD9852->SetRampTime(dValue); else return false; break;
	case 18:if (DataLength_in_bit == 48) MyAD9852->SetFrequency1AsBinary((uint64_t)dValue); else return false; break;
	case 19:if (DataLength_in_bit == 48) MyAD9852->SetFrequency2AsBinary((uint64_t)dValue); else return false; break;
	case 20:if (DataLength_in_bit == 0) MyAD9852->MasterReset(); else return false; break;
	default: return false;//To do: throw exception
	}
	MyAD9852->WriteAllToBus();
	return true;
}

bool CDeviceAD9854::SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {

	//bool SetControlBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue = false);
	//int64_t SetValue(unsigned char ValueNr, int64_t Value, bool GetValue = false);
	//double CalculateModulationFrequencyData(int64_t* DeltaFrequencyWord, unsigned int* ClockUpdateSteps);
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
	return true;
}
