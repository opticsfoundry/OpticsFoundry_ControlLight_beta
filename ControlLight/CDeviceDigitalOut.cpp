#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceDigitalOut.h"


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


//This is a whole board of up to 16 digital outputs
CDeviceDigitalOut::CDeviceDigitalOut(
	CDeviceSequencer* _MySequencer,
	unsigned int _MyAddress,
	unsigned int _numberChannels
) : CDevice(_MySequencer, _MyAddress, "DigitalOut") {
	numberChannels = _numberChannels;
	LastValue = 0;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) MySequencer->ParallelBusDeviceList[MyAddress] = this;
	else {
		std::ostringstream oss;
		oss << "CDeviceDigitalOut::CDeviceDigitalOut: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		return;
	}
}


bool CDeviceDigitalOut::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (SubAddress > 0) return false;
	if (DataLength_in_bit > 15) return false;
	if ((StartBit + DataLength_in_bit) > 16) return false;
	if (Data == nullptr) return false;
	if (DataLength_in_bit == 1)
	{
		bool On = Data[0] > 0;
		if (On) {
			LastValue |= (1 << StartBit);
		}
		else {
			LastValue &= ~(1 << StartBit);
		}
	}
	else if (DataLength_in_bit == 16) {
		uint16_t* data = (uint16_t*)(Data);
		LastValue = data[0];
	}
	else {
		//Old code without data bitmask: this assumes the user does take care of bitmasking.
		//uint16_t* data = (uint16_t*)(Data);
		//LastValue &= ~(0xFFFF << StartBit);
		//LastValue |= (data[0] << StartBit);
		
		uint16_t* data = (uint16_t*)(Data);
		uint16_t dataMask = static_cast<uint16_t>((1u << DataLength_in_bit) - 1u);
		uint16_t shiftedDataMask = static_cast<uint16_t>(dataMask << StartBit);
		LastValue &= ~shiftedDataMask;
		LastValue |= static_cast<uint16_t>((data[0] & dataMask) << StartBit);
	}
	MySequencer->WriteBusAddressAndDataToBuffer(MyAddress, LastValue);
	return true;
}
