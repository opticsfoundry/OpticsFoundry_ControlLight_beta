#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "CDeviceRack.h"


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

//This class describes the whole chain of racks to which one sequencer is connected
//All racks have a copy of an arbiter circuit to select one slot as input slot
//i.e. sending data to once, configures all racks in the chain and selects exactly one slot
CDeviceRack::CDeviceRack(
	CDeviceSequencer* _MySequencer//,
	//unsigned int _MyRackAddress
) : CDevice(_MySequencer, 0xFE, "Rack") {
	//MyRackAddress = _MyRackAddress;
	LastValue = 0;
	if (MySequencer->ParallelBusDeviceList[MyAddress] == nullptr) MySequencer->ParallelBusDeviceList[MyAddress] = this;
	else {
		std::ostringstream oss;
		oss << "CDeviceRack::CDeviceRack: Sequencer[" << MySequencer->id << "] Parallel port address " << MyAddress << " is already in use.";
		std::string msg = oss.str();
		NotifyError(msg);
		return;
	}
}


bool CDeviceRack::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (SubAddress > 0) return false;
	if (DataLength_in_bit == 0) return false;
	if (DataLength_in_bit > 16) return false;
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
		uint16_t* data = (uint16_t*)(Data);
		uint16_t dataMask = static_cast<uint16_t>((1u << DataLength_in_bit) - 1u);
		uint16_t shiftedDataMask = static_cast<uint16_t>(dataMask << StartBit);
		LastValue &= ~shiftedDataMask;
		LastValue |= static_cast<uint16_t>((data[0] & dataMask) << StartBit);
	}

	//Reduce bus speed, as arbiter is slower than a normal dig out port to allow first-break then-make behavior.
	MySequencer->SetFPGAClockToBusClockRatio(/* FPGAClockToBusClockRatio */ 50,  /*UpdateStrobeDuration*/ true); //this command waits for the strobe generator parameter update to have effect
	MySequencer->WriteBusAddressAndDataToBuffer(MyAddress, LastValue);
	MySequencer->Wait_ms(0.0005); //we need to wait till the rack arbiter has updated before we can shorten the strobe length again.
	MySequencer->SetFPGAClockToBusClockRatio(/* FPGAClockToBusClockRatio == 0: use default ratio */ 0, /*UpdateStrobeDuration*/ true);
	return true;
}
