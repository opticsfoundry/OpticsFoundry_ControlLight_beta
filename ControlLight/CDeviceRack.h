#pragma once

#include "CDevice.h"

//This class describes the whole chain of racks to which one sequencer is connected
class CDeviceRack : public CDevice
{
public:
	uint16_t LastValue;
	//uint8_t MyRackAddress;
public:
	CDeviceRack(
		CDeviceSequencer* _MySequencer//,
		//unsigned int _MyRackAddress = 0xFE
	);
	virtual ~CDeviceRack() = default;
	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();

	virtual bool SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit);
	//virtual bool SetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool GetValue(unsigned int SubAddress, uint8_t* Data, unsigned long DataLength);
	//virtual bool Configure();

	virtual bool SelectRackSlot(const uint8_t& RackNr, const uint8_t& SlotNr) {
		//This conversion will need to be adjusted to each type of backplane or if we enable slot selection in enchained racks
		uint8_t SlotAddr = (SlotNr <= 3) ? (SlotNr + 12) : (SlotNr - 4);
		
		uint8_t Data = (SlotAddr & 0x0F) | ((RackNr & 0x07) << 4);
		return SetValue(0, &Data, 7, 0);
	}

	virtual bool ResetI2CMultiplexer() {
		//Here we assume that the bus uses a slow speed, so that the first-break-then-make logic of the arbiter has time to work.
		//We need to test how fast one can go like this. As the delay between break and set of new demux input is 20ns, and the remaining delay is monoflop controlled, it should be fine up to 50MHz.
		uint8_t Data = 1;
		bool success = SetValue(0, &Data, 1, 7);
		MySequencer->Wait_ms(0.01);
		Data = 0;
		success = success && SetValue(0, &Data, 1, 7);
		MySequencer->Wait_ms(0.01);
		return success;
	}
};
