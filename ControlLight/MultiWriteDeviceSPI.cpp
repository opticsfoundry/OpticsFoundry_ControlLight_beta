// MultiWriteDevice.cpp: implementation of the CMultiWriteDeviceSPI class.
//
//////////////////////////////////////////////////////////////////////

#include "std.h"
#include "MultiWriteDeviceSPI.h"
#include <format>

#include "CDeviceSequencer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef DebugSPI
constexpr bool DebugSPICommunication = true;
std::string DebugFilePath = "D:\\Florian\\OpticsFoundry\\OpticsFoundryControl\\Debug\\";
#endif

CMultiWriteDeviceSPI::CMultiWriteDeviceSPI(unsigned short aBus, unsigned long aBaseAddress, CDeviceSequencer* _MyDeviceSequencer)
{

	QSPIMode = false;
	MyDeviceSequencer = _MyDeviceSequencer;
	ForceWriting=false;	
	Enabled=true;
	MultiIOAddress=0;
	Bus = aBus;
	BaseAddress = aBaseAddress;
	MultiIOAddress = Bus + (unsigned char)BaseAddress;

	for (unsigned int i = 0; i < MultiWriteDeviceSPIMaxBusBuffer; i++) { 
		BusBuffer[i] = 0; 
		BusBufferSPICreateClock[i] = false;
	}
	BusBufferStart = 0;
	BusBufferEnd = 0;
	BusBufferLength = 0;
	Current_SPI_clock_type = E_SPI_clock_bit_banged;

	ControlRegisterContent = 0;
	SPI_CS_bit = 0;
	SDIO_0_bit = 0;
	SPI_SCLK_bit = 0;
	SPI_frequency_in_Hz = 0;
	SPI_mode = 0;
	SPI_CPOL = false;
	SPI_CPHA = false;
	//SetSPIFrequencyAndMode(0,/*SPI_mode*/0);
#ifdef DebugSPI
	if (DebugSPICommunication) {
		std::string filename = std::format("{}DebugSPICommunication_{}_{}.txt", DebugFilePath, Bus, BaseAddress);
		DebugFile = new std::ofstream(filename, std::ios::out);
	}
	else DebugFile = NULL;
#endif
}

CMultiWriteDeviceSPI::~CMultiWriteDeviceSPI()
{
#ifdef DebugSPI
	if (DebugFile) {
		DebugFile->close();
		delete DebugFile;
		DebugFile = NULL;
	}
#endif
}

void CMultiWriteDeviceSPI::SwitchForceWritingMode(bool OnOff) {
	ForceWriting = OnOff;
}


void CMultiWriteDeviceSPI::Enable(bool OnOff)
{
	Enabled = OnOff;
}


void CMultiWriteDeviceSPI::AssureMinimumSPIClockPeriodLength() {
	double MinimumClockHalfPeriod_in_ms = 500.0 / SPI_frequency_in_Hz;
	double BusPeriod_in_ms = 1000.0 / MyDeviceSequencer->BusFrequency_in_Hz;
	
	if (BusPeriod_in_ms < MinimumClockHalfPeriod_in_ms) {
		WriteAllToBus(/*End_SPI_clock_node*/ false);
		MyDeviceSequencer->Wait_ms(MinimumClockHalfPeriod_in_ms);
	}
}

void CMultiWriteDeviceSPI::AddToBusBuffer(unsigned short value, bool CreateSPIClock) {
	if (!Enabled) return;
	if (BusBufferLength >= MultiWriteDeviceSPIMaxBusBuffer) {
		ControlMessageBox(std::format("CMultiWriteDeviceSPI::AddToBusBuffer : Bus Buffer exceeded ({})", MultiWriteDeviceSPIMaxBusBuffer));
		return;
	}
	BusBufferSPICreateClock[BusBufferEnd] = CreateSPIClock;
	BusBuffer[BusBufferEnd] = value;	
	BusBufferLength++;
	BusBufferEnd++;
	if (BusBufferEnd >= MultiWriteDeviceSPIMaxBusBuffer) BusBufferEnd = 0;
	AssureMinimumSPIClockPeriodLength();
}

bool CMultiWriteDeviceSPI::WriteToBus(const uint8_t& minimum_spacing_in_strobe_lengths)
{
    if (!Enabled) return false;
    if (BusBufferLength == 0) return false;
	if (SPI_clock_type == E_SPI_clock_FPGA) {
		const bool clock_idle = SPI_CPOL;
		const bool clock_active = !SPI_CPOL;
		const bool bus_data15_first_part = clock_idle;
		MyDeviceSequencer->WriteBusAddressAndDataToBufferSPI(MultiIOAddress, (BusBuffer[BusBufferStart] & ~(0x8000)) | ((bus_data15_first_part) ? 0x8000 : 0), /*bus_strobe_first_part*/ 1, /*bus_strobe_second_part*/ 1, /*bus_strobe_idle_part*/1, /*bus_data15_second_part*/(BusBufferSPICreateClock[BusBufferStart]) ? clock_active : clock_idle, /*bus_data15_idle_part*/clock_idle); //ToDo: get SPI clock from a buffer, as this could be different per SPI mode and per thing to do; also: as we have transparent latches, make it possible to address additional control lines with three strobe-length command
	}
	else {
		MyDeviceSequencer->WriteBusAddressAndDataToBuffer(MultiIOAddress, BusBuffer[BusBufferStart], minimum_spacing_in_strobe_lengths);
	}
	BusBufferStart++;
	if (BusBufferStart >= MultiWriteDeviceSPIMaxBusBuffer) BusBufferStart = 0;
	BusBufferLength--;
    return true;
}

void CMultiWriteDeviceSPI::WriteAllToBus(bool End_SPI_clock_node)
{
	if (BusBufferLength == 1) {
		if ((SPI_clock_type == E_SPI_clock_FPGA) && (Current_SPI_clock_type != E_SPI_clock_FPGA)) {
			if (!BusBufferSPICreateClock[BusBufferStart]) {
				//We use an SPI card with transparent latches (faster for SPI command)
				//We are not in SPI clock generation mode, and have only one non-SPI command to send, i.e. an update of some control line. 
				//This can be done in one bus cycle, if we extend it to 3x strobe length, 
				//as we use transparent latches and the strobe has to go back to zero before moving on
				WriteToBus(/*minimum_spacing_in_strobe_lengths*/ 3);
				return;
			}
		}
	}
	if ((SPI_clock_type == E_SPI_clock_FPGA) && (Current_SPI_clock_type != E_SPI_clock_FPGA)) {
		SetSPIChipSelect(true, /*write_immediately*/ false);
		//enter bit-banged FPGA SPI mode 
		const bool clock_idle = SPI_CPOL;
		const bool bus_data15_first_part = clock_idle;
		MyDeviceSequencer->WriteBusAddressAndDataToBufferSPI(MultiIOAddress, (ControlRegisterContent & ~(0x8000)) | ((bus_data15_first_part) ? 0x8000 : 0), /*bus_strobe_first_part*/ 0, /*bus_strobe_second_part*/ 1, /*bus_strobe_idle_part*/1, /*bus_data15_second_part*/clock_idle, /*bus_data15_idle_part*/clock_idle);
		Current_SPI_clock_type = E_SPI_clock_FPGA;
	}
	while (WriteToBus(/*minimum_spacing_in_strobe_lengths*/ 3)) {  //ToDo: find out why 2 doesn't work here; seems to be a speed limit of the electronics; If solved: if we use transparent latches and E_SPI_clock_bit_banged, this needs to be 3
	}
	if ((SPI_clock_type == E_SPI_clock_FPGA) && (End_SPI_clock_node) && (Current_SPI_clock_type == E_SPI_clock_FPGA)) {
		SetSPIChipSelect(false, /*write_immediately*/ false);
		//leave bit-banged FPGA SPI mode 
		const bool clock_idle = SPI_CPOL;
		const bool bus_data15_first_part = clock_idle;
		MyDeviceSequencer->WriteBusAddressAndDataToBufferSPI(MultiIOAddress, (ControlRegisterContent & ~(0x8000)) | ((bus_data15_first_part) ? 0x8000 : 0), /*bus_strobe_first_part*/ 1, /*bus_strobe_second_part*/ 0, /*bus_strobe_idle_part*/0, /*bus_data15_second_part*/clock_idle, /*bus_data15_idle_part*/clock_idle);
		Current_SPI_clock_type = E_SPI_clock_bit_banged;
	}
}

void CMultiWriteDeviceSPI::SetControlRegister(unsigned char start_bit, unsigned char nr_bits, unsigned short value, bool write_immediately, bool CreateSPIClock) {
	unsigned short mask = (1 << nr_bits) - 1;
	unsigned short shifted_value = (value & mask) << start_bit;
	ControlRegisterContent = (ControlRegisterContent & ~(mask << start_bit)) | shifted_value;
	if (write_immediately) AddToBusBuffer(ControlRegisterContent, CreateSPIClock);
}

void CMultiWriteDeviceSPI::ConfigureSPI(unsigned char _SPI_CS_bit, unsigned char  _SDIO_0_bit, unsigned char _SDIO_1_bit, unsigned char _SDIO_2_bit, unsigned char _SDIO_3_bit, unsigned char _SPI_SCLK_bit, E_SPI_clock_type _SPI_clock_type) {
	SPI_CS_bit = _SPI_CS_bit;
	SDIO_0_bit = _SDIO_0_bit;
	SDIO_1_bit = _SDIO_1_bit;
	SDIO_2_bit = _SDIO_2_bit;
	SDIO_3_bit = _SDIO_3_bit;
	SPI_SCLK_bit = _SPI_SCLK_bit;
	SPI_clock_type = _SPI_clock_type;
}

void CMultiWriteDeviceSPI::SetQSPIMode(bool OnOff) {
	QSPIMode = OnOff;
}

void CMultiWriteDeviceSPI::SetSPIClock(bool clock, bool write_immediately) {
	SetControlRegister(SPI_SCLK_bit, 1, (clock) ? 1 : 0, write_immediately);
}

void CMultiWriteDeviceSPI::SetSPIChipSelect(bool cs, bool write_immediately) {
	SetControlRegister(SPI_CS_bit, 1, (cs) ? 1 : 0, write_immediately);
}
void CMultiWriteDeviceSPI::SetSPIDataOut(bool data) {
	SetControlRegister(SDIO_0_bit, 1, (data) ? 1 : 0);
}


/*
SPI is defined by two parameters:

CPOL (clock polarity) -> idle level of SCLK
CPHA (clock phase) -> which edge is used to sample

This gives 4 modes:

Mode	CPOL	CPHA	Sample edge
0	0	0	rising edge
1	0	1	falling edge
2	1	0	falling edge
3	1	1	rising edge
							*/
void CMultiWriteDeviceSPI::SetSPIFrequencyAndMode(double _SPI_frequency_in_Hz, const uint8_t _SPI_mode) {
	SPI_mode = _SPI_mode;
	SPI_CPHA = (_SPI_mode & 1) > 0;
	SPI_CPOL = (_SPI_mode & 2) > 0;
	SPI_frequency_in_Hz = _SPI_frequency_in_Hz;
}

std::string format_binary_64(uint64_t data, unsigned int number_of_bits_out) {
	std::string result;
	for (int i = number_of_bits_out - 1; i >= 0; --i) {
		result += (data & (1ULL << i)) ? '1' : '0';
		if (i % 4 == 0 && i != 0) result += ' ';
	}
	return result;
}


//I keep this code around for reference, as it works well
void CMultiWriteDeviceSPI::WriteSPIBitBangedMode0Simple(unsigned int number_of_bits_out, uint64_t data) {  //
	// data bit 0 = LSB
	// data bit number_of_bits_out - 1 = MSB
	// SPI must send MSB first

	//ToDo: take SPI_mode and SPI_frequency_in_Hz into account
	// MySequencer->SetStrobeDurationInFPGAClockPeriods(/*StrobeDurationInFPGAClockPeriods  == 0: use default ratio*/ 0, /*UpdateStrobeDuration*/ true);//this command waits for the strobe generator parameter update to have effect


	if (number_of_bits_out == 0 || number_of_bits_out > 64) {
		return; // or throw
	}

	if (QSPIMode && (number_of_bits_out % 4 != 0)) {
		return; // or throw
	}

	uint64_t data_sent = 0;

	auto GetBitMSBFirst = [&](unsigned int bit_index) -> unsigned int {
		// Input convention:
		// bit 0 = LSB
		// bit number_of_bits_out - 1 = MSB
		//
		// Output convention:
		// send MSB first
		return static_cast<unsigned int>(
			(data >> (number_of_bits_out - 1 - bit_index)) & 0x1
			);
		};

	SetSPIChipSelect(false, /*write_immediately*/ true); //ToDo: check that we can also use this code with write_immediately = false in this command. Would safe one bus period.
	SetSPIClock(false, /*write_immediately*/ true);

	if (QSPIMode) {
		unsigned int bit_to_send_0;
		unsigned int bit_to_send_1;
		unsigned int bit_to_send_2;
		unsigned int bit_to_send_3;

		for (unsigned int i = 0; i < number_of_bits_out; i += 4) {
			bit_to_send_3 = GetBitMSBFirst(i + 0);
			bit_to_send_2 = GetBitMSBFirst(i + 1);
			bit_to_send_1 = GetBitMSBFirst(i + 2);
			bit_to_send_0 = GetBitMSBFirst(i + 3);

			data_sent <<= 1;
			data_sent |= bit_to_send_3;

			data_sent <<= 1;
			data_sent |= bit_to_send_2;

			data_sent <<= 1;
			data_sent |= bit_to_send_1;

			data_sent <<= 1;
			data_sent |= bit_to_send_0;

			SetControlRegister(SDIO_0_bit, 1, bit_to_send_0, /*write_immediately*/ false);
			SetControlRegister(SDIO_1_bit, 1, bit_to_send_1, /*write_immediately*/ false);
			SetControlRegister(SDIO_2_bit, 1, bit_to_send_2, /*write_immediately*/ false);
			SetControlRegister(SDIO_3_bit, 1, bit_to_send_3, /*write_immediately*/ true);

			SetSPIClock(true);
			SetSPIClock(false, /*write_immediately*/ false);
		}
	}
	else {
		unsigned int bit_to_send = 0x0;

		for (unsigned int i = 0; i < number_of_bits_out; i++) {
			data_sent <<= 1;

			bit_to_send = GetBitMSBFirst(i);

			SetSPIDataOut(bit_to_send > 0);
			data_sent |= bit_to_send;

			SetSPIClock(true);
			SetSPIClock(false, /*write_immediately*/ false);
		}
	}

	SetSPIChipSelect(true, /*write_immediately*/ true);

#ifdef DebugSPI
	if (DebugFile) {
		unsigned long data_low = data_sent & 0xFFFFFFFF;
		unsigned long data_high = (data_sent >> 32) & 0xFFFFFFFF;
		std::string buf = std::format(
			"Wrote {} bits, data = {:08X} {:08X} = first bit sent {} last bit sent",
			number_of_bits_out,
			data_high,
			data_low,
			format_binary_64(data_sent, number_of_bits_out)
		);
		*DebugFile << buf << std::endl;
	}
#endif
}

//Generalized code, implementing any SPI mode
void CMultiWriteDeviceSPI::WriteSPIBitBanged(unsigned int number_of_bits_out, uint64_t data) {
	// data bit 0 = LSB
	// data bit number_of_bits_out - 1 = MSB
	// SPI must send MSB first

	if (Current_SPI_clock_type == E_SPI_clock_FPGA) {
		WriteSPIBitBangedFPGAClock(number_of_bits_out, data);
		return;
	}


	if (number_of_bits_out == 0 || number_of_bits_out > 64) {
		return; // or throw
	}

	if (QSPIMode && (number_of_bits_out % 4 != 0)) {
		return; // or throw
	}

	const bool clock_idle = SPI_CPOL;
	const bool clock_active = !SPI_CPOL;

	auto GetBitMSBFirst = [&](unsigned int bit_index) -> unsigned int {
		return static_cast<unsigned int>(
			(data >> (number_of_bits_out - 1 - bit_index)) & 0x1
			);
		};

	auto WriteSingleSPIDataOut = [&](unsigned int bit_value, bool write_immediately) {
		// Assumption: normal SPI MOSI is SDIO_0.
		// If SetSPIDataOut() uses a different bit internally, replace SDIO_0_bit here.
		SetControlRegister(SDIO_0_bit, 1, bit_value ? 1 : 0, write_immediately);
		};

	auto WriteQSPIDataOut = [&](unsigned int bit0,
		unsigned int bit1,
		unsigned int bit2,
		unsigned int bit3,
		bool write_immediately) {
			SetControlRegister(SDIO_0_bit, 1, bit0 ? 1 : 0, false);
			SetControlRegister(SDIO_1_bit, 1, bit1 ? 1 : 0, false);
			SetControlRegister(SDIO_2_bit, 1, bit2 ? 1 : 0, false);
			SetControlRegister(SDIO_3_bit, 1, bit3 ? 1 : 0, write_immediately);
		};

	SetSPIClock(clock_idle);
	SetSPIChipSelect(false, /*write_immediately*/ true);

	if (QSPIMode) {
		for (unsigned int i = 0; i < number_of_bits_out; i += 4) {
			unsigned int bit_to_send_3 = GetBitMSBFirst(i + 0);
			unsigned int bit_to_send_2 = GetBitMSBFirst(i + 1);
			unsigned int bit_to_send_1 = GetBitMSBFirst(i + 2);
			unsigned int bit_to_send_0 = GetBitMSBFirst(i + 3);

			if (!SPI_CPHA) {
				// CPHA = 0:
				// First bit group: data must be written before sample edge.
				// Later bit groups: previous trailing edge + next data setup are combined.

				if (i == 0) {
					WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, true);
				}
				else {
					SetSPIClock(clock_idle, false);
					WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, true);
				}

				// Leading edge = sample edge
				SetSPIClock(clock_active);
			}
			else {
				// CPHA = 1:
				// Data may change on the leading edge.
				// Therefore data update + leading edge can be combined.
				// Trailing edge = sample edge.

				WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, false);

				// Leading edge + data update
				SetSPIClock(clock_active);

				// Trailing edge = sample edge
				SetSPIClock(clock_idle);
			}
		}
	}
	else {
		for (unsigned int i = 0; i < number_of_bits_out; i++) {
			unsigned int bit_to_send = GetBitMSBFirst(i);

			if (!SPI_CPHA) {
				// CPHA = 0:
				// First bit: data setup must be written before sample edge.
				// Later bits: previous trailing edge + next data setup are combined.

				if (i == 0) {
					WriteSingleSPIDataOut(bit_to_send, true);
				}
				else {
					SetSPIClock(clock_idle, false);
					WriteSingleSPIDataOut(bit_to_send, true);
				}

				// Leading edge = sample edge
				SetSPIClock(clock_active);
			}
			else {
				// CPHA = 1:
				// Data changes on leading edge.
				// Sample happens on trailing edge.

				WriteSingleSPIDataOut(bit_to_send, false);

				// Leading edge + data update
				SetSPIClock(clock_active);

				// Trailing edge = sample edge
				SetSPIClock(clock_idle);
			}
		}
	}

	// Finish final half-cycle for CPHA = 0.
	if (!SPI_CPHA) {
		SetSPIClock(clock_idle);
	}

	SetSPIChipSelect(true, /*write_immediately*/ true);

#ifdef DebugSPI
	if (DebugFile) {
		unsigned long data_low = data & 0xFFFFFFFF;
		unsigned long data_high = (data >> 32) & 0xFFFFFFFF;
		std::string buf = std::format(
			"Wrote {} bits, data = {:08X} {:08X} = first bit sent {} last bit sent",
			number_of_bits_out,
			data_high,
			data_low,
			format_binary_64(data, number_of_bits_out)
		);
		*DebugFile << buf << std::endl;
	}
#endif
}


//Generalized code, implementing any SPI mode
void CMultiWriteDeviceSPI::WriteSPIBitBangedFPGAClock(unsigned int number_of_bits_out, uint64_t data) {
	// data bit 0 = LSB
	// data bit number_of_bits_out - 1 = MSB
	// SPI must send MSB first



	if (number_of_bits_out == 0 || number_of_bits_out > 64) {
		return; // or throw
	}

	if (QSPIMode && (number_of_bits_out % 4 != 0)) {
		return; // or throw
	}

	const bool clock_idle = SPI_CPOL;
	const bool clock_active = !SPI_CPOL;

	auto GetBitMSBFirst = [&](unsigned int bit_index) -> unsigned int {
		return static_cast<unsigned int>(
			(data >> (number_of_bits_out - 1 - bit_index)) & 0x1
			);
		};

	auto WriteSingleSPIDataOut = [&](unsigned int bit_value, bool write_immediately) {
		// Assumption: normal SPI MOSI is SDIO_0.
		// If SetSPIDataOut() uses a different bit internally, replace SDIO_0_bit here.
		SetControlRegister(SDIO_0_bit, 1, bit_value ? 1 : 0, write_immediately, /* CreateSPIClock */ true);
		};

	auto WriteQSPIDataOut = [&](unsigned int bit0,
		unsigned int bit1,
		unsigned int bit2,
		unsigned int bit3,
		bool write_immediately) {
			SetControlRegister(SDIO_0_bit, 1, bit0 ? 1 : 0, false);
			SetControlRegister(SDIO_1_bit, 1, bit1 ? 1 : 0, false);
			SetControlRegister(SDIO_2_bit, 1, bit2 ? 1 : 0, false);
			SetControlRegister(SDIO_3_bit, 1, bit3 ? 1 : 0, write_immediately, /* CreateSPIClock */ true);
		};

	//Clock idle and CS are updated in WriteAllToBus when entering bit-banged FPGA clock mode, no need to do it here
	//SetSPIClock(clock_idle, /*write_immediately*/ false);
	//SetSPIChipSelect(false, /*write_immediately*/ false);

	//ToDo: carefully walk through all four SPI modes and check that data and clock timing is as intended
	if (QSPIMode) {
		for (unsigned int i = 0; i < number_of_bits_out; i += 4) {
			unsigned int bit_to_send_3 = GetBitMSBFirst(i + 0);
			unsigned int bit_to_send_2 = GetBitMSBFirst(i + 1);
			unsigned int bit_to_send_1 = GetBitMSBFirst(i + 2);
			unsigned int bit_to_send_0 = GetBitMSBFirst(i + 3);

			if (!SPI_CPHA) {
				// CPHA = 0:
				// First bit group: data must be written before sample edge.
				// Later bit groups: previous trailing edge + next data setup are combined.

				if (i == 0) {
					WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, true);
				}
				else {
					//SetSPIClock(clock_idle, false);
					WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, true);
				}

				// Leading edge = sample edge
				//SetSPIClock(clock_active);
			}
			else {
				// CPHA = 1:
				// Data may change on the leading edge.
				// Therefore data update + leading edge can be combined.
				// Trailing edge = sample edge.

				WriteQSPIDataOut(bit_to_send_0, bit_to_send_1, bit_to_send_2, bit_to_send_3, true);

				// Leading edge + data update
				//SetSPIClock(clock_active);

				// Trailing edge = sample edge
				//SetSPIClock(clock_idle);
			}
		}
	}
	else {
		for (unsigned int i = 0; i < number_of_bits_out; i++) {
			unsigned int bit_to_send = GetBitMSBFirst(i);

			if (!SPI_CPHA) {
				// CPHA = 0:
				// First bit: data setup must be written before sample edge.
				// Later bits: previous trailing edge + next data setup are combined.

				if (i == 0) {
					WriteSingleSPIDataOut(bit_to_send, true);
				}
				else {
					//SetSPIClock(clock_idle, false);
					WriteSingleSPIDataOut(bit_to_send, true);
				}

				// Leading edge = sample edge
				//SetSPIClock(clock_active);
			}
			else {
				// CPHA = 1:
				// Data changes on leading edge.
				// Sample happens on trailing edge.

				WriteSingleSPIDataOut(bit_to_send, true);

				// Leading edge + data update
				//SetSPIClock(clock_active);

				// Trailing edge = sample edge
				//SetSPIClock(clock_idle);
			}
		}
	}

	// Finish final half-cycle for CPHA = 0.
	if (!SPI_CPHA) {
		//SetSPIClock(clock_idle);
	}

	//Clock idle and CS are updated in WriteAllToBus when entering bit-banged FPGA clock mode, no need to do it here
	//SetSPIChipSelect(true, /*write_immediately*/ false);

#ifdef DebugSPI
	if (DebugFile) {
		unsigned long data_low = data & 0xFFFFFFFF;
		unsigned long data_high = (data >> 32) & 0xFFFFFFFF;
		std::string buf = std::format(
			"Wrote {} bits, data = {:08X} {:08X} = first bit sent {} last bit sent",
			number_of_bits_out,
			data_high,
			data_low,
			format_binary_64(data, number_of_bits_out)
		);
		*DebugFile << buf << std::endl;
	}
#endif
}