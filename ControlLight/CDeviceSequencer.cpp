#include "ControlAPI.h"
#include "CDevice.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include "EthernetControllerFirefly.h"
#include "AD9852.h"
#include "AD9858.h"
#include "CDeviceRack.h"
#ifdef WIN32
#include <tchar.h>
#endif

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

//#define DebugSequencer

#ifdef DebugSequencer
std::string DebugFilePath = "D:\\Florian\\OpticsFoundry\\OpticsFoundryControl\\Debug\\";
#endif


CDeviceSequencer::CDeviceSequencer(
	unsigned int _id,
	std::string _type,
	std::string _ip,
	unsigned int _port,
	bool _master,
	unsigned int _startDelay,
	double _clockFrequency,
	unsigned int _FPGAClockToBusClockRatio,
	bool _useExternalClock,
	bool _useStrobeGenerator,
	bool _useEdgeTriggeredLatches,
	bool _connect) {
	MySequencer = this;
	// Initialize the device
	MyEthernetMultiIOControllerFirefly = std::make_unique<CEthernetControllerFirefly>(this);
	for (unsigned int i = 0; i < MaxParallelBusDevices; i++)
		ParallelBusDeviceList[i] = nullptr;
	for (unsigned int i = 0; i < MaxSerialBusDevices; i++)
		SerialBusDeviceList[i] = nullptr;
	//The Sequencer can also take a few configuration commands.
	//By registering it just as any other device on the bus, we can access those virtual configuration registers.
	ParallelBusDeviceList[MaxParallelBusDevices - 1] = this;
	MyAddress = MaxParallelBusDevices - 1;
	id = _id;
	type = _type;
	ip = _ip;
	port = _port;
	master = _master;
	startDelay = _startDelay;
	clockFrequency = _clockFrequency;
	FPGAClockToBusClockRatio = _FPGAClockToBusClockRatio;
	if (FPGAClockToBusClockRatio < 2) FPGAClockToBusClockRatio = 2;
	DefaultFPGAClockToBusClockRatio = FPGAClockToBusClockRatio;
	CurrentFPGAClockToBusClockRatio = FPGAClockToBusClockRatio;
	useExternalClock = _useExternalClock;
	useStrobeGenerator = _useStrobeGenerator;
	connect = _connect;
	currentBuffer = 0;
	for (int n = 0; n < MaxBuffer; n++) Buffer[n] = nullptr;
	LastCommandWasSpecialCommand = true;
	LastCommandWasSPICommand = false;
	DoUseEdgeTriggeredLatches = _useEdgeTriggeredLatches;
	UpdateClockRatio();

	AbsoluteTime_in_ms = 0;
	BufferPosition = 0;
	bool success = MyEthernetMultiIOControllerFirefly->ConnectSocket(ip.c_str(), port, FPGAClockToBusClockRatio, clockFrequency, useExternalClock, useStrobeGenerator, /* ExternalTrigger*/ !master);
	if (!success) {
		NotifyError("Failed to connect to sequencer");
	}

	//Sequencer0->SwitchDebugMode(On);
	//Timestamp.StartDebug(DebugFilePath + "TimingDebug.dat");
	//NI653xEthernet->Debug(DebugFilePath + "DebugNI653xEthernet.dat");
#ifdef DebugSequencer
	MyEthernetMultiIOControllerFirefly->DebugBuffer(DebugFilePath + "DebugNI653xEthernetBuffer.dat");
#endif
	MyEthernetMultiIOControllerFirefly->MeasureEthernetBandwidth(1024 * 128, 20);

	MyDeviceRack = std::make_unique<CDeviceRack>(this);
}

CDeviceSequencer::~CDeviceSequencer() {
	//the -1 is needed because ParallelBusDeviceList[MaxParallelBusDevices-1] points to the sequencer itself
	for (unsigned int i = 0; i < MaxParallelBusDevices - 1; i++)
		if (ParallelBusDeviceList[i]) {
			//some devices use more than one address. Only delete them once.
			if (ParallelBusDeviceList[i] != ReservedBusAddress && ParallelBusDeviceList[i] != MyDeviceRack.get()) {
				delete ParallelBusDeviceList[i];
				ParallelBusDeviceList[i] = nullptr;
			}
		}
	for (unsigned int i = 0; i < MaxSerialBusDevices; i++) {
		if (SerialBusDeviceList[i]) {
			if (SerialBusDeviceList[i] != ReservedBusAddress) {
				delete SerialBusDeviceList[i];
				SerialBusDeviceList[i] = nullptr;
			}
		}
	}
	for (int n = 0; n < MaxBuffer; n++) {
		if (Buffer[n]) delete[] Buffer[n];
		Buffer[n] = nullptr;
	}
}


double CDeviceSequencer::GetBusFrequency_in_Hz() {
	return MyEthernetMultiIOControllerFirefly->GetFPGAClockFrequency_in_Hz() / CurrentFPGAClockToBusClockRatio;
}

void CDeviceSequencer::UpdateClockRatio() {
	if (DoUseEdgeTriggeredLatches) {
		CurrentFPGAClockToBusClockRatio = ((2 * (FPGAClockToBusClockRatio)) / 3);
	}
	else {
		CurrentFPGAClockToBusClockRatio = FPGAClockToBusClockRatio;
	}
	//	MyEthernetMultiIOControllerFirefly->SetFPGAClockToBusClockRatio(FPGAClockToBusClockRatio);
	BusFrequency_in_Hz = GetBusFrequency_in_Hz();
}

void CDeviceSequencer::UseEdgeTriggeredLatches(bool _UseEdgeTriggeredLatches) {
	DoUseEdgeTriggeredLatches = _UseEdgeTriggeredLatches;
	UpdateClockRatio();
}

void CDeviceSequencer::SetFPGAClockToBusClockRatio(const unsigned int _FPGAClockToBusClockRatio, const bool UpdateStrobeDuration) {
	FPGAClockToBusClockRatio = (_FPGAClockToBusClockRatio > 0) ? _FPGAClockToBusClockRatio : DefaultFPGAClockToBusClockRatio;
	if (FPGAClockToBusClockRatio < 2) FPGAClockToBusClockRatio = 2;
	UpdateClockRatio();
	if (UpdateStrobeDuration) {
		uint8_t StrobeDuration = ((FPGAClockToBusClockRatio) / 3) - 1;
		MyEthernetMultiIOControllerFirefly->SetStrobeOptions(/* StrobeChoice: Use FPGA strobe generator */ 1, StrobeDuration, StrobeDuration);
		Wait_ms(0.0001);//we need to wait for the strobe change to have effect
	}
}

void CDeviceSequencer::Initialize(unsigned long _PCBufferSize_in_u64) {
	PCBufferSize_in_u64 = _PCBufferSize_in_u64;
	for (int n = 0; n < MaxBuffer; n++) {
		if (Buffer[n]) delete[] Buffer[n];
		Buffer[n] = new uint32_t[2 * PCBufferSize_in_u64];
		for (uint32_t i = 0; i < 2 * PCBufferSize_in_u64; i++) {
			Buffer[n][i] = 0;
		}
	}
	currentBuffer = 0;
	BufferPosition = 0;
	AbsoluteTime_in_ms = 0;
	Delay_in_ms = 0;
	MaxTimeDebt_in_ms = 9999999999; //1ms
	TimeDebt_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;
	LastBusData = 0;
	LastCommandWasSpecialCommand = true;
}

bool CDeviceSequencer::IsSequencerConnected() {
	return MyEthernetMultiIOControllerFirefly->CheckReady();
}

void CDeviceSequencer::SwitchDebugMode(bool OnOff, const std::string& FileName) {
	MyEthernetMultiIOControllerFirefly->SwitchDebugMode(OnOff, FileName);
}

void CDeviceSequencer::TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool OnOff) {
	MyEthernetMultiIOControllerFirefly->TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(OnOff);
}

void CDeviceSequencer::StartAssemblingSequence() {
	//There can be multiple sequences in the buffer. This function starts the very first of these sequences.
	BufferPosition = 0;
	LastBusData = 0;
	LastCommandWasSpecialCommand = true;
	StartAssemblingNextSequence();
}

void CDeviceSequencer::StartAssemblingNextSequence() {
	//ToDo (optional): To be very precise we could add the small delay between the trigger and the first command.
	if (BufferPosition > 0) {
		//We need to finish the last sequence with a stop command to make sure it stops
		MyEthernetMultiIOControllerFirefly->AddCommandStop();
	}
	AbsoluteTime_in_ms = 0;
	Delay_in_ms = 0;
	TimeDebt_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;

	MyEthernetMultiIOControllerFirefly->AddSequencePreamble();
}

//Why does declaring it inline not work for this function? It would be useful to do so, as it's called very often from just one place.
void CDeviceSequencer::AddCommandToSequence(const uint32_t& high_word, const uint32_t& low_word) {
	if ((!LastCommandWasSpecialCommand) || (Delay_in_ms > 0)) {
		//this is a special command. We need to first send out LastBusData and then wait as needed before executing it.
		AddBusCommandToSequence(LastBusData);
	}
	if (BufferPosition >= PCBufferSize_in_u64) {
		NotifyError("Buffer overflow"); //ToDo: throw exception
		return;
	}
	Buffer[currentBuffer][BufferPosition * 2] = low_word;
	Buffer[currentBuffer][BufferPosition * 2 + 1] = high_word;
	BufferPosition++;
	LastCommandWasSpecialCommand = true;
	//Most special commands take only 3 FPGA clock cycles.
	//ToDo: some commands can take more, e.g. the LongWait command. Would need table of commands with their wait time to calculate this better. For now: don't use those commands.
	double added_time = 3 / clockFrequency * 1000.0;
	CurrentTimeDebt_in_ms += added_time;
	TimeDebt_in_ms += added_time;
}

inline void CDeviceSequencer::AddBusCommandAndWait(uint32_t busdata, uint32_t delay) {
	//MyEthernetMultiIOControllerFirefly->AddCommandStep( busdata, delay);
	//this is the most often occuring command. For efficiency I program it here and don't go through MyEthernetMultiIOControllerFirefly.
	//this also makes sure that it's not confused with a special command.


	/* core.sv: 
	CMD_STEP:begin
                            wait_time[30:0] <= command[35:5];
                            wait_time[47:31] <= 0;
`ifdef USE_AD9854_AS_VCO
                            if (send_AD9854_ftw) begin
                                bus_data[7:0] <= command[43:36];
                                bus_data[15:8] <= AD9854FTW[7:0];
                                bus_data[27:16] <= command[63:52];
                                ad9854_ftw_byte_shift_state <= AD9854_BYTE_SHIFT_STEP_1;
                            end else begin
                                bus_data <= command[63:36];
                            end  
`else
                            bus_data <= command[63:36];
`endif                      
                            bus_data15_used_as_strobe <= 0;
                            if (bus_clock) bus_clock <= 0; else bus_clock <= 1;                   
                            strobe_generator_state <= DELAY_CYCLE;   
                            address <= address + 1; 
                        end
						*/

	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	uint8_t command = 1; //CMD_STEP

	uint32_t low_buffer = ( ( (delay - 1) & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & busdata) << 4) | ((delay >> 27) & delay_mask_high);

	if (BufferPosition >= PCBufferSize_in_u64 / 2) {
		AddErrorMessage("Buffer overflow"); //ToDo: throw exception
		return;
	}
	Buffer[currentBuffer][BufferPosition * 2] = low_buffer;
	Buffer[currentBuffer][BufferPosition * 2 + 1] = high_buffer;
	BufferPosition++;
}

inline void CDeviceSequencer::AddBusCommandAndWaitSPI(uint32_t data, uint32_t delay, bool bus_strobe_first_part, bool bus_strobe_second_part, bool bus_strobe_idle_part, bool bus_data15_second_part,  bool bus_data15_idle_part) {
	const uint32_t delay_mask = 0x03FFFFFF; //26 bit
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F; //5 bit
	uint8_t command = 30; //CMD_STEP_SPI

	if (delay > 0x03FFFFFF) {
		AddErrorMessage("CDeviceSequencer::AddBusCommandAndWaitSPI : delay too long.");
		return;
	}

	uint32_t low_buffer = (((delay - 1) & delay_mask) << 5) |
		((bus_data15_second_part ? 1u : 0u) << 31) |
		(command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) |
		((bus_data15_idle_part ? 1u : 0u) << 0) |
		((bus_strobe_first_part ? 1u : 0u) << 1) |
		((bus_strobe_second_part ? 1u : 0u) << 2) |
		((bus_strobe_idle_part ? 1u : 0u) << 3);

	if (BufferPosition >= PCBufferSize_in_u64 / 2) {
		AddErrorMessage("Buffer overflow"); //ToDo: throw exception
		return;
	}
	Buffer[currentBuffer][BufferPosition * 2] = low_buffer;
	Buffer[currentBuffer][BufferPosition * 2 + 1] = high_buffer;
	BufferPosition++;
}

void CDeviceSequencer::AddBusCommandToSequenceSPI(const uint32_t& content, bool bus_strobe_first_part, bool bus_strobe_second_part, bool bus_strobe_idle_part, bool bus_data15_second_part, bool bus_data15_idle_part) {
	if (!LastCommandWasSPICommand) AddBusCommandToSequence(0, /*OnlyWriteLargeDelays*/ true); //we might have to add some wait before starting SPI mode
	LastCommandWasSPICommand = true;
	uint32_t spacing = CurrentFPGAClockToBusClockRatio;
	AddBusCommandAndWaitSPI(content, spacing, bus_strobe_first_part, bus_strobe_second_part, bus_strobe_idle_part, bus_data15_second_part, bus_data15_idle_part);
}

void CDeviceSequencer::AddBusCommandToSequence(const uint32_t& content, bool OnlyWriteLargeDelays) {
	if (LastCommandWasSPICommand) {
		LastCommandWasSPICommand = false;
		LastBusData = content;
	}
	//static uint32_t LeftoverSpacing = 0;
	if (BufferPosition >= PCBufferSize_in_u64) {
		AddErrorMessage("Buffer overflow"); 
		return;
	}
	uint64_t spacing64 = Delay_in_ms * clockFrequency / 1000.0;
	if (spacing64 > 60*60*1000*1000*100) {
		AddErrorMessage("CDeviceSequencer::AddBusCommandToSequence : delay longer than one hour. This looks like a bug."); 
		return;
	}
	
	constexpr uint32_t maxspacing = 0x7FFFFFFF;
	while (spacing64 > maxspacing) { //delay can be 31 bit long, which equals 21 seconds at 100MHz FPGA clock
		if (LastCommandWasSpecialCommand) {
			AddBusCommandAndWait(0, maxspacing);
			LastBusData = content;
			LastCommandWasSpecialCommand = false;
		}
		else {
			AddBusCommandAndWait(LastBusData, maxspacing);
			LastBusData = content;
		}
		spacing64 -= maxspacing;
	}
	uint32_t spacing = spacing64;
	if (spacing == 0) {
		spacing = CurrentFPGAClockToBusClockRatio;
		double added_time = CurrentFPGAClockToBusClockRatio / clockFrequency * 1000.0;
		CurrentTimeDebt_in_ms += added_time;
		TimeDebt_in_ms += added_time;
	}
	AdvanceTime();
	if (LastCommandWasSpecialCommand) {
		//at the beginning of a special command, the LastBusData is written out and the remaining wait time is waited.
		//Therefore nothing to write out now.
		//We just put this bus data onto the todo list for next time.
		
		if (spacing > (OnlyWriteLargeDelays) ? 2 : 0) AddBusCommandAndWait(0, spacing);
		
		LastBusData = content;
		//LeftoverSpacing = spacing;
	}
	else {
		//Challenge: each bus data command is accompanied by the wait time till the next command, after the bus was updated, not before.
		// This is the inverse logic from here.
		// Solution: we store the bus data that should be put out in LastBusData.
		// Then we here execute the last bus data command and the wait time till this one.
		// Finally we store the requested bus data update as to be done next time.
		AddBusCommandAndWait(LastBusData, /*LeftoverSpacing + */spacing);
		LastBusData = content;
		//LeftoverSpacing = 0;
	}
	LastCommandWasSpecialCommand = false;
	LastCommandWasSPICommand = false;
}

void CDeviceSequencer::GetBufferLength(uint32_t& FilledBufferLength, uint32_t& MaxBufferLength) {
	FilledBufferLength = BufferPosition;
	MaxBufferLength = PCBufferSize_in_u64;
}

static std::string AppendSequencerNumberToFilename(const std::string& FileName, unsigned int SequencerNumber) {
	if (FileName.empty()) return "";

	const size_t lastSeparator = FileName.find_last_of("/\\");
	const size_t extensionPosition = FileName.find_last_of('.');
	const bool hasExtension = extensionPosition != std::string::npos
		&& (lastSeparator == std::string::npos || extensionPosition > lastSeparator);
	const std::string suffix = "_" + std::to_string(SequencerNumber);

	if (hasExtension) {
		return FileName.substr(0, extensionPosition) + suffix + FileName.substr(extensionPosition);
	}
	return FileName + suffix + ".txt";
}

void CDeviceSequencer::SendSequence(const std::string& FileName) {
	AddBusCommandToSequence(0); //just in case there is still something in LastBusData.
	MyEthernetMultiIOControllerFirefly->SendSequenceToFPGA(
		Buffer[currentBuffer],
		AppendSequencerNumberToFilename(FileName, id));
	currentBuffer++;
	if (currentBuffer >= MaxBuffer) {
		currentBuffer = 0;
	}
}

void CDeviceSequencer::SendStartSequenceCommand() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->Start();
	}
}

bool CDeviceSequencer::IsSequenceRunning(bool& running, unsigned long long& DataPointsWritten) {
	if (master) {
		return MyEthernetMultiIOControllerFirefly->AttemptGetAktWaveformPoint(DataPointsWritten, running);
	}
	else {
		DataPointsWritten = 0;
		running = false;
		return false;
	}
}

bool CDeviceSequencer::WaitTillEndOfSequence(double timeout_in_s) {
	if (master) {
		return MyEthernetMultiIOControllerFirefly->WaitTillEndOfSequence(timeout_in_s);
	}
	return false;
}

bool CDeviceSequencer::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned long& EndTimeOfCycle, double timeout_in_s) {
	if (master) {
		return MyEthernetMultiIOControllerFirefly->WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s);
	}
	else {
		buffer = nullptr;
		buffer_length = 0;
		EndTimeOfCycle = 0;
		return false;
	}
}

void CDeviceSequencer::StartAssemblingCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->StartAssemblingCPUCommandSequence();
	}
}

void CDeviceSequencer::AddCPUCommand(const char* command) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->AddCPUCommand(command);
	}
}

void CDeviceSequencer::ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->ExecuteCPUCommandSequence(ethernet_check_period_in_ms);
	}
}

void CDeviceSequencer::StopCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->StopCPUCommandSequence();
	}
}

void CDeviceSequencer::InterruptCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->InterruptCPUCommandSequence();
	}
}

void CDeviceSequencer::GetCPUCommandErrorMessages() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->GetCPUCommandErrorMessages();
	}
}

void CDeviceSequencer::PrintCPUCommandErrorMessages() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->PrintCPUCommandErrorMessages();
	}
}

void CDeviceSequencer::PrintCPUCommandSequence() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->PrintCPUCommandSequence();
	}
}

void CDeviceSequencer::SetPeriodicTrigger_ms(double periodicTriggerPeriod_in_ms, double periodicTriggerAllowedWaitTime_in_ms) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->SetPeriodicTrigger_ms(periodicTriggerPeriod_in_ms, periodicTriggerAllowedWaitTime_in_ms);
	}
}

void CDeviceSequencer::GetNextCycleNumber(long& NextCycleNumber) {
	if (master) {
		MyEthernetMultiIOControllerFirefly->GetNextCycleNumber(NextCycleNumber);
	}
}

void CDeviceSequencer::ResetCycleNumber() {
	if (master) {
		MyEthernetMultiIOControllerFirefly->ResetCycleNumber();
	}
}

bool CDeviceSequencer::TransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently) {
	return MyEthernetMultiIOControllerFirefly->TransmitI2CPort(I2C_port, I2C_destination, I2C_address, send_length, send_data, receive_length, receive_data, I2C_clock_frequency_in_Hz, I2C_success, fail_silently);
}

bool CDeviceSequencer::SetPSOptions(uint8_t options) {
	return MyEthernetMultiIOControllerFirefly->SetPSOptions(options);
}

void CDeviceSequencer::SetSequencerDigitalOut(uint8_t dig_out_pattern) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetCoreOption_dig_out(dig_out_pattern);
}

void CDeviceSequencer::SetSequencer_PL_to_PS_command(uint8_t PL_to_PS_command) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetCoreOption_PL_to_PS(PL_to_PS_command);
}

void CDeviceSequencer::SwitchSequencerBuzzer(bool OnOff) {
	MyEthernetMultiIOControllerFirefly->AddCommandSwitchBuzzer(OnOff);
}


bool CDeviceSequencer::Wait_ms(double time_in_ms) {
	Delay_in_ms += time_in_ms;
	if (TimeDebt_in_ms > 0) {
		//let's pay off some or all of the time debt we acquired when commands took time beyond one cycle, e.g. DDS commands.
		if (Delay_in_ms > TimeDebt_in_ms) {
			Delay_in_ms -= TimeDebt_in_ms;
			CurrentTimeDebt_in_ms = 0;
			TimeDebt_in_ms = 0;
		}
		else
		{
			TimeDebt_in_ms -= Delay_in_ms;
			CurrentTimeDebt_in_ms -= Delay_in_ms;
			Delay_in_ms = 0;
		}
		if (TimeDebt_in_ms > MaxTimeDebt_in_ms) return false;//ToDo throw error
	}
	return true;
}

double CDeviceSequencer::GetTime_ms() {
	return AbsoluteTime_in_ms + Delay_in_ms;
}

void CDeviceSequencer::AdvanceTime() {
	
	AbsoluteTime_in_ms += Delay_in_ms + CurrentTimeDebt_in_ms;
	Delay_in_ms = 0;
	CurrentTimeDebt_in_ms = 0;
}

//the following are commands to control the sequencer itself
bool CDeviceSequencer::SetValue(const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	return false;
}

void CDeviceSequencer::SequencerStartAnalogInAcquisition(const uint8_t& analog_in_type, const uint8_t& SPI_CS, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms) {
	MyEthernetMultiIOControllerFirefly->StartAnalogInAcquisition(analog_in_type, SPI_CS, ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms);
}

void CDeviceSequencer::SequencerWriteInputMemory(unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
	MyEthernetMultiIOControllerFirefly->AddCommandWriteInputBuffer(input_buf_mem_data, write_next_address, input_buf_mem_address);
}

void CDeviceSequencer::SequencerWriteSystemTimeToInputMemory() {
	MyEthernetMultiIOControllerFirefly->AddCommandWriteSystemTimeToInputMemory();
}

void CDeviceSequencer::SequencerCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift) {
	MyEthernetMultiIOControllerFirefly->AddCommandCalcAD9854FrequencyTuningWord(ftw0,bit_shift);
}

void CDeviceSequencer::SequencerSwitchDebugLED(unsigned int OnOff) {
	MyEthernetMultiIOControllerFirefly->SwitchDebugLED(OnOff);
}

void CDeviceSequencer::SequencerIgnoreTCPIP(bool OnOff) {
	MyEthernetMultiIOControllerFirefly->IgnoreTCPIP(OnOff);
}
void CDeviceSequencer::SequencerAddMarker(unsigned char marker) {
	MyEthernetMultiIOControllerFirefly->AddMarker(marker);
}

void CDeviceSequencer::SequencerSetLoopCount(unsigned int loop_count) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetLoopCount(loop_count);
}

void CDeviceSequencer::SequencerJumpBackward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr, bool loop_count_greater_zero) {
	MyEthernetMultiIOControllerFirefly->AddCommandJumpBackward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr, loop_count_greater_zero);
}

void CDeviceSequencer::SequencerJumpForward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr) {
	MyEthernetMultiIOControllerFirefly->AddCommandJumpForward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr);
}

void CDeviceSequencer::SequencerTransmitI2C(uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t *data_out) {
	MyEthernetMultiIOControllerFirefly->AddCommandTransmitI2C(I2C_port, I2C_length_out, I2C_length_in, data_out);
}

void CDeviceSequencer::SequencerTransmitSPI(const uint8_t chip_select, const uint16_t number_of_bits_out, const uint8_t *data_out, const uint8_t number_of_bits_in, const bool start_now) {
	MyEthernetMultiIOControllerFirefly->AddCommandTransmitSPI(chip_select, number_of_bits_out, data_out, number_of_bits_in, start_now);
}

void CDeviceSequencer::SequencerRepeatedOutIn(const uint16_t number_of_datapoints, const double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand) {
	MyEthernetMultiIOControllerFirefly->AddCommandRepeatedOutIn(number_of_datapoints, delay_between_datapoints_in_ms, RepeatedOutInCommand);
}

void CDeviceSequencer::SequencerSetSPITiming(uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetSPITiming(SPI_delay_CS_low_start_wait, SPI_delay_write, SPI_delay_pause_before_read, SPI_delay_read, SPI_delay_CS_low_end_wait);
}

void CDeviceSequencer::SequencerSetSPIMode(uint8_t SPI_mode) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetSPIMode(SPI_mode);
}

void CDeviceSequencer::SequencerSetI2CParameters(uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read) {
	MyEthernetMultiIOControllerFirefly->AddCommandSetI2CParameters(I2C_0_Destination, I2C_delay_start_stop, I2C_delay_data_setup, I2C_delay_clock_high, I2C_delay_clock_low, I2C_delay_pause_before_read);
}


void CDeviceSequencer::SequencerSelectRackSlot(uint8_t rack_nr, uint8_t slot_nr) {
	MyDeviceRack->SelectRackSlot(rack_nr, slot_nr);
}

void CDeviceSequencer::SequencerResetI2CMultiplexer() {
	MyDeviceRack->ResetI2CMultiplexer();
}

bool CDeviceSequencer::SetValue_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address >= MaxParallelBusDevices) return false;
	if (GetParallelBusDevice(Address)) { //make sure ParallelBusDeviceList[Address] is neither nullptr not 1
		bool success = ParallelBusDeviceList[Address]->SetValue(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetRegister_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address >= MaxParallelBusDevices) return false;
	if (GetParallelBusDevice(Address)) { //make sure ParallelBusDeviceList[Address] is neither nullptr not 1
		bool success = ParallelBusDeviceList[Address]->SetRegister(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetValueSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address >= MaxSerialBusDevices) return false;
	if (GetSerialBusDevice(Address)) { //make sure SerialBusDeviceList[Address] is neither nullptr not 1
		bool success = SerialBusDeviceList[Address]->SetValue(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}

bool CDeviceSequencer::SetRegisterSerialDevice_Sequencer(const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
	if (Address >= MaxSerialBusDevices) return false;
	if (GetSerialBusDevice(Address)) { //make sure SerialBusDeviceList[Address] is neither nullptr not 1
		bool success = SerialBusDeviceList[Address]->SetRegister(SubAddress, Data, DataLength_in_bit, StartBit);
		if (success) AdvanceTime();
		return success;
	}
	return false;
}
