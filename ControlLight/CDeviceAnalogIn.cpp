
#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAnalogIn.h"


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




CDeviceAnalogIn::CDeviceAnalogIn(
	CDeviceSequencer* _MySequencer,
	unsigned int _chipSelect,
	bool _signedValue,
	double _minVoltage,
	double _maxVoltage) : CDevice(_MySequencer, _chipSelect) {
	signedValue = _signedValue;
	minVoltage = _minVoltage;
	maxVoltage = _maxVoltage;
	AnalogInType = 0;
	SPI_CS = 0;
	ChannelNumber = 0;
	NumberOfDataPoints = 0;
	DelayBetweenDataPoints_in_ms = 0;
	if (MySequencer->SerialBusDeviceList[MyAddress] == nullptr) MySequencer->SerialBusDeviceList[MyAddress] = this;
	else {
		std::ostringstream oss;
		oss << "CDeviceAD9858::CDeviceAD9858: Sequencer[" << MySequencer->id << "] Serial port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		return;
	}
}

bool CDeviceAnalogIn::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	//Commands to set parameters of the sequencer and put special commands into the sequence
	switch (SubAddress) {

	case 0: if (DataLength_in_bit == 8) AnalogInType = Data[0]; else return false; break;
	case 1: if (DataLength_in_bit == 8) SPI_CS = Data[0]; else return false; break;
	case 2: if (DataLength_in_bit == 8) ChannelNumber = Data[0]; else return false; break;
	case 3: if (DataLength_in_bit == 32) NumberOfDataPoints = ((uint32_t*)Data)[0]; else return false; break;
	case 4: if (DataLength_in_bit == 64) DelayBetweenDataPoints_in_ms = ((double*)Data)[0]; else return false; break;
	case 5: MySequencer->SequencerStartAnalogInAcquisition(AnalogInType, SPI_CS, ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms); break;
	default: return false;//To do: throw exception
	}
	return true;
}
