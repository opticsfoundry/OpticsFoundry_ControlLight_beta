#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceAnalogOut.h"


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



// CDeviceAnalogOut is a single 16bit analog output, not a board with several of those outputs
CDeviceAnalogOut::CDeviceAnalogOut(
	CDeviceSequencer* _sequencer,
	unsigned int _MyAddress,
	bool _signedValue,
	double _minVoltage,
	double _maxVoltage) : CDevice(_sequencer, _MyAddress, "AnalogOut")
{
	signedValue = _signedValue;
	minVoltage = _minVoltage;
	maxVoltage = _maxVoltage;
	LastValue = 0;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) MySequencer->ParallelBusDeviceList[MyAddress] = this;
	else {
		std::ostringstream oss;
		oss << "CDeviceAnalogOut::CDeviceAnalogOut: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		return;
	}
}


bool CDeviceAnalogOut::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (SubAddress > 0) return false;
	if (DataLength_in_bit != 16) return false;
	if (StartBit > 0) return false;
	if (Data == nullptr) return false;
	LastValue = ((uint16_t*)(Data))[0];
	MySequencer->WriteBusAddressAndDataToBuffer(MyAddress, LastValue);
	return true;
}

bool CDeviceAnalogOut::SetVoltage(double Voltage) {
	if (Voltage < minVoltage) Voltage = minVoltage;
	if (Voltage > maxVoltage) Voltage = maxVoltage;
	uint16_t value;
	if (signedValue) {
		value = (unsigned short)((Voltage + 10.0) * 65535.0 / 20.0); //-10 to 10V
	}
	else {
		value = (unsigned short)((Voltage + 0.0) * 65535.0 / 10.0); //0 to 10V
	}
	return SetValue(0, (uint8_t*)(&value), 16, 0);
}
