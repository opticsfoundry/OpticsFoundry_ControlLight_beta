
#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAD9959.h"
#include "AD9959.h"


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




CDeviceAD9959::CDeviceAD9959(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyAddress,
	double _externalClockFrequency,
	unsigned int _frequencyMultiplier,
	bool _AD9958
) : CDevice(_MySequencer, _MyAddress, "AD9959") {
	externalClockFrequency = _externalClockFrequency;
	frequencyMultiplier = _frequencyMultiplier;
	AD9958 = _AD9958;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) {
		MySequencer->ParallelBusDeviceList[MyAddress] = this;
		MyAD9959 = new CAD9959(/*bus*/0, MyAddress, externalClockFrequency, frequencyMultiplier, AD9958, MySequencer);
	}
	else {
		std::ostringstream oss;
		oss << "CDeviceAD9959::CDeviceAD9959: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		MyAD9959 = nullptr;
		return;
	}
}

CDeviceAD9959::~CDeviceAD9959() {
	if (MyAD9959) {
		delete MyAD9959;
		MyAD9959 = nullptr;
	}
}


bool CDeviceAD9959::SetValue(const unsigned int& SubAddress, const uint8_t* data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//ToDo: do range checks
	double* dValueptr = (double*)data;
	double dValue = *dValueptr;

	switch (SubAddress) {
	case 0:MyAD9959->MasterReset(); break;

	case 1:if (DataLength_in_bit == 64) MyAD9959->SetFrequencyCh0(dValue); break;
	case 2:if (DataLength_in_bit == 32) MyAD9959->SetFrequencyTuningWordCh0((uint32_t)dValue); break;
	case 3:if (DataLength_in_bit == 64) MyAD9959->SetIntensityCh0(dValue); break;
	case 4:if (DataLength_in_bit == 64) MyAD9959->SetPhaseOffsetCh0(dValue); break;

	case 5:if (DataLength_in_bit == 64) MyAD9959->SetFrequencyCh1(dValue); break;
	case 6:if (DataLength_in_bit == 32) MyAD9959->SetFrequencyTuningWordCh1((uint32_t)dValue); break;
	case 7:if (DataLength_in_bit == 64) MyAD9959->SetIntensityCh1(dValue); break;
	case 8:if (DataLength_in_bit == 64) MyAD9959->SetPhaseOffsetCh1(dValue); break;

	case 9:if (DataLength_in_bit == 64) MyAD9959->SetFrequencyCh2(dValue); break;
	case 10:if (DataLength_in_bit == 32) MyAD9959->SetFrequencyTuningWordCh2((uint32_t)dValue); break;
	case 11:if (DataLength_in_bit == 64) MyAD9959->SetIntensityCh2(dValue); break;
	case 12:if (DataLength_in_bit == 64) MyAD9959->SetPhaseOffsetCh2(dValue); break;

	case 13:if (DataLength_in_bit == 64) MyAD9959->SetFrequencyCh3(dValue); break;
	case 14:if (DataLength_in_bit == 32) MyAD9959->SetFrequencyTuningWordCh3((uint32_t)dValue); break;
	case 15:if (DataLength_in_bit == 64) MyAD9959->SetIntensityCh3(dValue); break;
	case 16:if (DataLength_in_bit == 64) MyAD9959->SetPhaseOffsetCh3(dValue); break;

	case 17: if (DataLength_in_bit == 1) MyAD9959->SetIOUpdateEnabled(data[0] == 1); else return false; break;
	
	default: return false;//To do: throw exception
	}
	MyAD9959->WriteAllToBus();
	return true;
}

bool CDeviceAD9959::SetRegister(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//Here we could give direct access to the registers, e.g. as specified in the datasheet.
	MyAD9959->SetRegisterBits(
		/*RegisterNr*/SubAddress,
		/*BitNr*/StartBit,
		/*BitLength*/DataLength_in_bit,
		/*Value*/Data[0],
		/*GetValue*/false,
		/*DoIOUpdate*/true);
	return true;
}


bool CDeviceAD9959::SetFrequency(uint8_t channel, double Frequency) {
	MyAD9959->SetFrequency(channel + 1, Frequency);
	MyAD9959->WriteAllToBus();
	//MySequencer->AdvanceTime();
	return true;
}

bool CDeviceAD9959::SetFrequencyTuningWord(uint8_t channel, uint64_t FrequencyTuningWord) {
	MyAD9959->SetFrequencyTuningWord(channel, FrequencyTuningWord);
	MyAD9959->WriteAllToBus();
	//MySequencer->AdvanceTime();
	return true;
}

bool CDeviceAD9959::SetPhase(uint8_t channel, double Phase) {
	MyAD9959->SetPhaseOffset(channel + 1, Phase);
	MyAD9959->WriteAllToBus();
	//MySequencer->AdvanceTime();
	return true;
}

bool CDeviceAD9959::SetPower(uint8_t channel, double Power) {
	MyAD9959->SetAmplitude(channel + 1, Power);
	MyAD9959->WriteAllToBus();
	//MySequencer->AdvanceTime();
	return true;
}

bool CDeviceAD9959::SetAttenuation(uint8_t channel, double Attenuation) {
	MyAD9959->SetAttenuation(channel + 1, Attenuation);
	MyAD9959->WriteAllToBus();
	//MySequencer->AdvanceTime();
	return true;
}

bool CDeviceAD9959::Reset() {
	MyAD9959->MasterReset();
	MyAD9959->WriteAllToBus();
	return true;
}

bool CDeviceAD9959::SetIOUpdateEnabled(bool _IOUpdateEnabled)
{
	MyAD9959->SetIOUpdateEnabled(_IOUpdateEnabled);
	MyAD9959->WriteAllToBus();
	return true;
}