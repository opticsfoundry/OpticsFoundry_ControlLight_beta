
#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAD9858.h"
#include "AD9858.h"


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




CDeviceAD9858::CDeviceAD9858(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyAddress,
	double _externalClockFrequency,
	unsigned int _frequencyMultiplier
) : CDevice(_MySequencer, _MyAddress, "AD9858") {
	externalClockFrequency = _externalClockFrequency;
	frequencyMultiplier = _frequencyMultiplier;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) {
		MySequencer->ParallelBusDeviceList[MyAddress] = this;
		// The AD9854 uses 4 addresses, so we need to reserve the next 3 addresses
		for (unsigned int i = 1; i < 4; i++) {
			MySequencer->ParallelBusDeviceList[MyAddress + i] = ReservedBusAddress;
		}
		MyAD9858 = new CAD9858(MyAddress, externalClockFrequency, frequencyMultiplier, MySequencer);
	}
	else {
		std::ostringstream oss;
		oss << "CDeviceAD9858::CDeviceAD9858: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		MyAD9858 = nullptr;
		return;
	}
}

CDeviceAD9858::~CDeviceAD9858() {
	if (MyAD9858) {
		delete MyAD9858;
		MyAD9858 = nullptr;
	}
}


bool CDeviceAD9858::SetValue(const unsigned int& SubAddress, const uint8_t* data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//ToDo: do range checks
	double* dValueptr = (double*)data;
	double dValue = *dValueptr;

	switch (SubAddress) {
	case 0: if (DataLength_in_bit == 64) MyAD9858->SetFrequencyTuningWord0((uint64_t)dValue); else return false; break;
	case 1: if (DataLength_in_bit == 64) MyAD9858->SetFrequencyTuningWord1((uint64_t)dValue); else return false; break;
	case 2: if (DataLength_in_bit == 64) MyAD9858->SetFrequencyTuningWord2((uint64_t)dValue); else return false; break;
	case 3: if (DataLength_in_bit == 64) MyAD9858->SetFrequencyTuningWord3((uint64_t)dValue); else return false; break;
	case 4: if (DataLength_in_bit == 64) MyAD9858->SetFrequency0(dValue); else return false; break;
	case 5: if (DataLength_in_bit == 64) MyAD9858->SetFrequency1(dValue); else return false; break;
	case 6: if (DataLength_in_bit == 64) MyAD9858->SetFrequency2(dValue); else return false; break;
	case 7: if (DataLength_in_bit == 64) MyAD9858->SetFrequency3(dValue); else return false; break;
	case 8: if (DataLength_in_bit == 64) MyAD9858->SetModulationFrequency(dValue); else return false; break;
	case 9: if (DataLength_in_bit == 64) MyAD9858->SetPhaseOffsetWord0(dValue); else return false; break;
	case 10: if (DataLength_in_bit == 64) MyAD9858->SetPhaseOffsetWord1(dValue); else return false; break;
	case 11: if (DataLength_in_bit == 64) MyAD9858->SetPhaseOffsetWord2(dValue); else return false; break;
	case 12: if (DataLength_in_bit == 64) MyAD9858->SetPhaseOffsetWord3(dValue); else return false; break;
	case 13: if (DataLength_in_bit == 8) MyAD9858->SetPhaseDetectorDividerRatioN(data[0]); else return false; break;
	case 14: if (DataLength_in_bit == 8) MyAD9858->SetPhaseDetectorDividerRatioM(data[0]); else return false; break;
	case 15: if (DataLength_in_bit == 8) MyAD9858->SetFrequencyDetectModeChargePumpCurrent(data[0]); else return false; break;
	case 16: if (DataLength_in_bit == 8) MyAD9858->SetFinalClosedLoopModeChargePumpCurrent(data[0]); else return false; break;
	case 17: if (DataLength_in_bit == 8) MyAD9858->SetWideClosedLoopModeChargePumpCurrent(data[0]); else return false; break;
	case 18: if (DataLength_in_bit == 64) MyAD9858->SetDeltaFrequencyWord((int64_t)dValue); else return false; break;
	case 19: if (DataLength_in_bit == 32) MyAD9858->SetRampRateWord((unsigned long)dValue); else return false; break;
	case 20: if (DataLength_in_bit == 64) MyAD9858->SetAttenuation(dValue); else return false; break;

	case 21: if (DataLength_in_bit == 8) MyAD9858->Set2GHzDividerDisable(data[0]); else return false; break;
	case 22: if (DataLength_in_bit == 8) MyAD9858->SetSYNCLKDisable(data[0]); else return false; break;
	case 23: if (DataLength_in_bit == 8) MyAD9858->SetMixerPowerDown(data[0]); else return false; break;
	case 24: if (DataLength_in_bit == 8) MyAD9858->SetPhaseDetectPowerDown(data[0]); else return false; break;
	case 25: if (DataLength_in_bit == 8) MyAD9858->SetPowerDown(data[0]); else return false; break;
	case 26: if (DataLength_in_bit == 8) MyAD9858->SetSDIOInputOnly(data[0]); else return false; break;
	case 27: if (DataLength_in_bit == 8) MyAD9858->SetLSBFirst(data[0]); else return false; break;
	case 28: if (DataLength_in_bit == 8) MyAD9858->SetFrequencySweepEnable(data[0]); else return false; break;
	case 29: if (DataLength_in_bit == 8) MyAD9858->SetEnableSineOutput(data[0]); else return false; break;
	case 30: if (DataLength_in_bit == 8) MyAD9858->SetChargePumpOffset(data[0]); else return false; break;
	case 31: if (DataLength_in_bit == 8) MyAD9858->SetChargePumpPolarity(data[0]); else return false; break;
	case 32: if (DataLength_in_bit == 8) MyAD9858->SetAutoClearFrequencyAccumulator(data[0]); else return false; break;
	case 33: if (DataLength_in_bit == 8) MyAD9858->SetAutoClearPhaseAccumulator(data[0]); else return false; break;
	case 34: if (DataLength_in_bit == 8) MyAD9858->SetLoadDeltaFrequencyTimer(data[0]); else return false; break;
	case 35: if (DataLength_in_bit == 8) MyAD9858->SetClearFrequencyAccumulator(data[0]); else return false; break;
	case 36: if (DataLength_in_bit == 8) MyAD9858->SetClearPhaseAccumulator(data[0]); else return false; break;
	case 37: if (DataLength_in_bit == 8) MyAD9858->SetFastLockEnable(data[0]); else return false; break;
	case 38: if (DataLength_in_bit == 8) MyAD9858->SetFTWForFastLock(data[0]); else return false; break;
	case 39: if (DataLength_in_bit == 8) MyAD9858->SetPS0(data[0]); else return false; break;
	case 40: if (DataLength_in_bit == 8) MyAD9858->SetPS1(data[0]); else return false; break;

	default: return false;//To do: throw exception
	}
	MyAD9858->WriteAllToBus();
	return true;
}

bool CDeviceAD9858::SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//Here we could give direct access to the registers, e.g. as specified in the datasheet.

	return true;
}




