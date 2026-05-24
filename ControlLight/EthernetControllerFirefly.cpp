// OvenControl.cpp: implementation of the CEthernetControllerFirefly class.
//
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include <afxwin.h>
#endif
#include "EthernetControllerFirefly.h"
#include "std.h"
#include "ControlAPI.h"
#include "CDeviceSequencer.h"
#include <thread>
#include <iostream>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <limits>
#include <vector>


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

constexpr uint32_t MaxReconnectAttempts = 100;

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

const uint32_t MaxFPGAProgramLength = 0x100000; //*8 to convert to bytes
const unsigned long MaxSequencerCommandListSize = 1024*16;
constexpr bool MinimizeEthernetCommunicationDirectionChanges = true;

CEthernetControllerFirefly::CEthernetControllerFirefly(CDeviceSequencer* _MySequencer) :
	CNetworkClient(/*ethernet communication mode */ 3, /* FastWrite*/false) {
	FPGAClockToBusClockRatio = 50;
	FPGAClockFrequencyInHz = 100000000;
	FPGAUseExternalClock = false;
	MySequencer = _MySequencer;
	previous_command_buffer = NULL;
	previous_command_buffer_length = 0;
	DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible = false;
	SequencerCommandListSize = 0;
	SequencerCommandList = nullptr;
	Connected = false;
	DebugBufferOn = false;
	DebugBufferFile = NULL;
	ExternalTrigger0 = false;
	ExternalTrigger1 = false;
	ExternalClock0 = false;
	ExternalClock1 = false;
	PeriodicTriggerPeriod_in_ms = 0;
	PeriodicTriggerAllowedWait_in_ms = 0;
	FPGAUseStrobeGenerator = false;
	//FPGABuffer = new uint32_t[MaxFPGAProgramLength * 4];
	//FPGAAbsoluteTime = new uint32_t[MaxFPGAProgramLength];
	//FPGABufferUsed = 0;
	core_option_LED = false;
	core_option_SPI_CS = 0;
	core_option_dig_out = 0;
	core_option_PL_to_PS = 0;
	SetPeriodicTriggerAtBeginningOfNextSequence = false;
	WaitForPeriodicTriggerAtBeginningOfSequence = false;
	ChangePeriodicTriggerPeriodWhileCycling = false;
	LastPeriodicTriggerPeriod_in_ms = 0;

	previous_receive_data_ptr = nullptr;
	receive_data_length = 0;
	previous_input_buffer_ptr = nullptr;
	previous_command_buffer_ptr = nullptr;

	MyMultiIO = 0;

	StartTickCounts = Clock::now();
}

CEthernetControllerFirefly::~CEthernetControllerFirefly()
{
	if (DebugBufferFile) {
		DebugBufferFile->close();
		delete DebugBufferFile;
		DebugBufferFile = nullptr;
	}
	if (Connected) CloseConnection();
	//if (FPGABuffer) delete FPGABuffer;
	//if (FPGAAbsoluteTime) delete FPGAAbsoluteTime;
	if (SequencerCommandList) {
		delete[] SequencerCommandList;
		SequencerCommandList = nullptr;
	}
	if (previous_command_buffer_ptr) {
		delete[] previous_command_buffer_ptr;
		previous_command_buffer_ptr = nullptr;
	}
	if (previous_receive_data_ptr) {
		delete[] previous_receive_data_ptr;
		previous_receive_data_ptr = nullptr;
	}
	if (previous_input_buffer_ptr) {
		delete[] previous_input_buffer_ptr;
		previous_input_buffer_ptr = nullptr;
	}
}

bool CEthernetControllerFirefly::ConnectSocket(const std::string& host, unsigned port, unsigned int aFPGAClockToBusClockRatio, double aFPGAClockFrequencyInHz, bool aFPGAUseExternalClock, bool aFPGAUseStrobeGenerator, bool aExternalTrigger) {
	Connected = CNetworkClient::ConnectSocket(host, port, "EthernetMultiIOController");
	//BusFrequency = aBusFrequency;
	FPGAClockToBusClockRatio = aFPGAClockToBusClockRatio;
	FPGAClockFrequencyInHz = aFPGAClockFrequencyInHz;
	FPGAUseExternalClock = aFPGAUseExternalClock;
	FPGAUseStrobeGenerator = aFPGAUseStrobeGenerator;
	ExternalTrigger0 = aExternalTrigger;
	return Connected;
}

double CEthernetControllerFirefly::GetFPGAClockFrequency_in_Hz() {
	//return BusFrequency;
	return FPGAClockFrequencyInHz;
}

void CEthernetControllerFirefly::AddSequencerCommandToBuffer(uint32_t* buffer, uint32_t n, uint32_t high_buffer, uint32_t low_buffer) {

	buffer[n * 2 + 0] = low_buffer;
	buffer[n * 2 + 1] = high_buffer;

	if (DebugBufferFile) {
		(*DebugBufferFile) << std::format("{:8d} CMD buffer = {:08X} {:08X}", n, high_buffer, low_buffer) << endl;
	}
}

void CEthernetControllerFirefly::ClearSequencerCommandList() {
	//SequencerCommandListSize = 0;
}

void CEthernetControllerFirefly::AddSequencerCommand(uint32_t high_word, uint32_t low_word, const uint8_t duration_in_FPGA_clock_cycles) {
	/*if (SequencerCommandListSize >= MaxSequencerCommandListSize) {
		AddErrorMessage("EthernetMultiIOControllerFirefly.cpp: AddSequencerCommand(): sequencer command list has too little memory, increase MaxSequencerCommandListSize.");
		return MaxSequencerCommandListSize - 1;
	}
	SequencerCommandList[SequencerCommandListSize * 2] = low_word;
	SequencerCommandList[SequencerCommandListSize * 2 + 1] = high_word;
	unsigned int CurrentCommandNr = SequencerCommandListSize;
	SequencerCommandListSize++;
	return CurrentCommandNr;*/
	if (DebugBufferFile) {
		std::string buf = std::format("data = {:08X} {:08X}", high_word, low_word);
		(*DebugBufferFile) << buf << endl;
	}
	MySequencer->AddCommandToSequence(high_word, low_word, duration_in_FPGA_clock_cycles);
}



void CEthernetControllerFirefly::DebugBuffer(const std::string& filename) {
	DebugBufferFile = new ofstream(filename, ios::out);
	DebugBufferOn = true;
}

/*
Assumptions for removal of unused bus entries:
Buffer[2 * i] = Data;
Buffer[2 * i + 1] = DirectionOutBit | StrobeBit | Address;
Address consists of 8 low lying main address bits and three (in principle up to six) high lying sub address bits
const unsigned int StrobeBit = 0x0100;

const unsigned int BusBitShift = 8;  //Attention: the subbus bits will get shifted by an additional two bits in CMultiIO::GetBusContents to make space for direction and strobe bits
const unsigned int Bus0 = 0x0 << BusBitShift;
...
const unsigned int BusSequencerSpecialCommand = 0x7 << BusBitShift;
const unsigned int Lock = 0x8 << BusBitShift;
*/


constexpr uint8_t CMD_STOP = 0;
constexpr uint8_t CMD_STEP = 1;
constexpr uint8_t CMD_STEP_AND_ENTER_FAST_MODE = 2;
constexpr uint8_t CMD_SET_OPTIONS = 3;
constexpr uint8_t CMD_LOAD_REG_LOW = 4;
constexpr uint8_t CMD_LOAD_REG_HIGH = 5;
constexpr uint8_t CMD_LATCH_STATE = 6;
constexpr uint8_t CMD_RESET_WAIT_CYCLES = 7;
constexpr uint8_t CMD_LONG_WAIT = 8;
constexpr uint8_t CMD_SET_STROBE_OPTIONS = 9;
constexpr uint8_t CMD_SET_INPUT_BUF_MEM = 10;
constexpr uint8_t CMD_WAIT_FOR_TRIGGER = 11;
constexpr uint8_t CMD_SET_LOOP_COUNT = 12;
constexpr uint8_t CMD_CONDITIONAL_JUMP_FORWARD = 13;
constexpr uint8_t CMD_CONDITIONAL_JUMP_BACKWARD = 14;
constexpr uint8_t CMD_I2C_OUT = 15;
constexpr uint8_t CMD_SPI_OUT_IN = 16;
constexpr uint8_t CMD_INPUT_REPEATED_OUT_IN = 17;
constexpr uint8_t CMD_SET_PERIODIC_TRIGGER_PERIOD = 18;
constexpr uint8_t CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME = 19;
constexpr uint8_t CMD_WAIT_FOR_PERIODIC_TRIGGER = 20;
constexpr uint8_t CMD_WAIT_FOR_WAIT_CYCLE_NR = 21;
constexpr uint8_t CMD_DIG_IN = 22;
constexpr uint8_t CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT = 23;
constexpr uint8_t CMD_ANALOG_IN_OUT = 24;
constexpr uint8_t CMD_PL_TO_PS_COMMAND = 25;
constexpr uint8_t CMD_LOAD_COMMAND_BUFFER = 26;
constexpr uint8_t CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM = 27;
constexpr uint8_t CMD_CALC_AD9854_FREQUENCY_TUNING_WORD = 28;
constexpr uint8_t CMD_LOAD_EXTENDED_COMMAND = 29;
constexpr uint8_t CMD_STEP_SPI = 30;

constexpr uint8_t NrCommands = 31;
const std::string CommandNames[NrCommands] = { "CMD_STOP", "CMD_STEP", "CMD_STEP_AND_ENTER_FAST_MODE", "CMD_SET_OPTIONS", "CMD_LOAD_REG_LOW", "CMD_LOAD_REG_HIGH", "CMD_LATCH_STATE", "CMD_RESET_WAIT_CYCLES", "CMD_LONG_WAIT", "CMD_SET_STROBE_OPTIONS", "CMD_SET_INPUT_BUF_MEM", "CMD_WAIT_FOR_TRIGGER", "CMD_SET_LOOP_COUNT", "CMD_CONDITIONAL_JUMP_FORWARD", "CMD_CONDITIONAL_JUMP_BACKWARD", "CMD_I2C_OUT", "CMD_SPI_OUT_IN", "CMD_INPUT_REPEATED_OUT_IN", "CMD_SET_PERIODIC_TRIGGER_PERIOD", "CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME", "CMD_WAIT_FOR_PERIODIC_TRIGGER", "CMD_WAIT_FOR_WAIT_CYCLE_NR", "CMD_DIG_IN", "CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT", "CMD_ANALOG_IN_OUT", "CMD_PL_TO_PS_COMMAND", "CMD_LOAD_COMMAND_BUFFER", "CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM", "CMD_CALC_AD9854_FREQUENCY_TUNING_WORD", "CMD_LOAD_EXTENDED_COMMAND", "CMD_STEP_SPI" };
constexpr bool CommandUsesBuffer[NrCommands] = { false   , false     , false                         , true             , false             , false              , false            , false                  , true           , true                    , true                   , true                  , true                , true                          , true                           , true         , true            , true                       , true                             , true                                        , false                          , true                        , true        , false                                  , true               , true                  , false                , false												, true , false                       , false };

constexpr uint8_t EXTENDED_CMD_STOP = 0;
constexpr uint8_t EXTENDED_CMD_LOAD_SPI_TIMING = 1;
constexpr uint8_t EXTENDED_CMD_SET_SPI_MODE = 2;
constexpr uint8_t EXTENDED_CMD_SET_I2C_PARAMETERS = 3;
constexpr uint8_t NrExtendedCommands = 4;
const std::string ExtendedCommandNames[NrExtendedCommands] = { "EXTENDED_CMD_STOP", "EXTENDED_CMD_LOAD_SPI_TIMING", "EXTENDED_CMD_SET_SPI_MODE", "EXTENDED_CMD_SET_I2C_PARAMETERS" };


void CEthernetControllerFirefly::StartAnalogInAcquisition(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {
	//if (channel_nr < 2) {
	//	StartXADCAnalogInAcquisition(channel_nr, number_of_datapoints, delay_between_datapoints_in_ms);
//	} else {
		StartSPIAnalogInAcquisition(analog_in_type, SPI_CS, channel_nr /* - 2*/, number_of_datapoints, delay_between_datapoints_in_ms);
	//}
}

void CEthernetControllerFirefly::AddSequencerCommandToSequenceList(uint32_t high_buffer, uint32_t low_buffer, const uint8_t duration_in_FPGA_clock_cycles) {
	const unsigned int command = low_buffer & 0x1F;
	if (command < NrCommands) {
		if (CommandUsesBuffer[command]) {
			uint32_t low_command_buffer = (low_buffer & 0xFFFFFFE0) | (0x1F & CMD_LOAD_COMMAND_BUFFER);
			AddSequencerCommand(high_buffer, low_command_buffer);
		}
	}
	AddSequencerCommand(high_buffer, low_buffer, duration_in_FPGA_clock_cycles);
}

void CEthernetControllerFirefly::StartXADCAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {

	//only for debugging
	//for (int i = 0; i < 15; i++) {
	//	AddCommandAnalogInOut(/* adc_register_address */ 3, /* adc_write_enable */ 0, /* adc_programming_out */ 0, /* dont_execute_now*/ 0, /* only_read_write*/ 0,/*wait_time*/ 150);
	//}
	/*
	* const u8 command_mask = 0x1F;  //5 bit
	CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command[15:15];
                            adc_programming_out <= command[31:16];
                            wait_time[29:0] <= command[63:34];
                            wait_time[47:30] <= 0;
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                        end*/
	uint8_t command = CMD_ANALOG_IN_OUT;
	uint32_t adc_register_address = 3;
	uint32_t adc_write_enable = 0;
	uint32_t adc_programming_out = 0;
	uint32_t wait_time = 0;
	uint8_t start_now = 0;
	uint8_t adc_conversion = 1;
	uint32_t low_buffer = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (0x1F & command);
	uint32_t high_buffer = ((wait_time & 0x3FFF) << (34 - 32)) | ((start_now & 0x01) << (32 - 32)) | ((adc_conversion & 0x01) << (33 - 32));


	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	/*

	CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
							INPUT_REPEAT_repeats <= command[27:8];
							INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
							INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
							INPUT_REPEAT_state <= INPUT_REPEAT_START;
							INPUT_REPEAT_command <= command[59:57];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end

					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
					if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
					if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;

	*/
	command = CMD_INPUT_REPEATED_OUT_IN;
	uint32_t INPUT_REPEAT_repeats = number_of_datapoints;
	uint32_t INPUT_REPEAT_wait = floor(delay_between_datapoints_in_ms * FPGAClockFrequencyInHz / 1000);
	uint32_t INPUT_REPEAT_command = 4; //XADC analog in
	uint32_t INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 0;

	low_buffer = (INPUT_REPEAT_repeats << 8) | command;
	high_buffer = INPUT_REPEAT_wait | (INPUT_REPEAT_trigger_secondary_interrupt_when_finished << (56-32)) | (INPUT_REPEAT_command << (57-32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
	//uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	//if (DelayMultiplier < 2) DelayMultiplier = 2;
	//AddCommandStep(0, DelayMultiplier*3-1); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the two commands above only consume 2 FPGA cycles, but are accounted for with 2 bus cycles by COutput
}

/*
//Vitis command:
CMD_SET_INPUT_BUF_MEM: begin
							//output reg [11:0] input_buf_mem_address = 0;
							//output reg [31:0] input_buf_mem_data = 0;
							if (command_buffer[7:7] == 1) input_buf_mem_address <= command_buffer[20:8];
							//else input_buf_mem_address <= input_buf_mem_address + 1;
							input_buf_mem_data <= command_buffer[63:32];
							//input_buf_mem_write_next_cycle <= 2;
							INPUT_MEM_state <= INPUT_MEM_WRITE;
							wait_time <= 1;
							address <= address + 1;
						end
*/

//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandWriteInputBuffer(unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
	uint8_t command = CMD_SET_INPUT_BUF_MEM;
	uint32_t low_buffer = ((input_buf_mem_address & 0x7FF) << 8) | (((write_next_address) ? 0 : 1) << 7) | (0x1F & command);
	uint32_t high_buffer = input_buf_mem_data;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}


/*
CMD_SET_LOOP_COUNT: begin
	loop_count <= command_buffer[63:32];
	address <= address + 1;
	wait_time <= 1;
end
*/
void CEthernetControllerFirefly::AddCommandSetLoopCount(unsigned int loop_count) {
	unsigned char command = CMD_SET_LOOP_COUNT;
	uint32_t low_buffer = command;
	uint32_t high_buffer = (loop_count & 0xFFFFFFFF);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
wire dig_in_jump_enabled;
wire [2:0] dig_in_jump_index;
wire selected_core_dig_in;
wire [13:0] jump_offset;

assign dig_in_jump_enabled = command_buffer[8];
assign dig_in_jump_index = command_buffer[7:5];
assign selected_core_dig_in = core_dig_in[dig_in_jump_index];
assign jump_offset = command_buffer[45:32];

CMD_CONDITIONAL_JUMP_FORWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
							if ((selected_core_dig_in && dig_in_jump_enabled) || (condition_0 && (command_buffer[9:9] == 1)) || (condition_1 && (command_buffer[10:10] == 1)) || (condition_PS && (command_buffer[11:11] == 1)) || (command_buffer[12:12] == 1))
								address <= address + jump_offset;
							else address <= address + 1;
							wait_time <= 1;
						end


*/
void CEthernetControllerFirefly::AddCommandJumpForward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr) {
	unsigned char command = CMD_CONDITIONAL_JUMP_FORWARD;
	unsigned __int32 low_buffer =
		((condition_0 ? (1 << 9) : 0) |
			(condition_1 ? (1 << 10) : 0) |
			(condition_PS ? (1 << 11) : 0) |
			(unconditional_jump ? (1 << 12) : 0) |
			((dig_in_bit_nr & 0x7) << 5) |
			(condition_dig_in ? (1 << 8) : 0) |
			command);
	unsigned __int32 high_buffer = jump_length & 0xFF;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
CMD_CONDITIONAL_JUMP_BACKWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
							if ((   (selected_core_dig_in && dig_in_jump_enabled) ||
							(condition_0_sync && (command_buffer[9:9] == 1)) ||
							(condition_1_sync && (command_buffer[10:10] == 1)) ||
							(condition_PS_sync && (command_buffer[11:11] == 1)) ||
								 (command_buffer[12:12] == 1) || ((command_buffer[13:13] == 1) && (loop_count > 0))))
								address <= address - jump_offset;
							else address <= address + 1;
							loop_count <= loop_count - 1;
							wait_time <= 1;
						end
*/

void CEthernetControllerFirefly::AddCommandJumpBackward(unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr, bool loop_count_greater_zero) {
	unsigned char command = CMD_CONDITIONAL_JUMP_BACKWARD;
	unsigned __int32 low_buffer =
		((condition_0 ? (1 << 9) : 0) |
			(condition_1 ? (1 << 10) : 0) |
			(condition_PS ? (1 << 11) : 0) |
			(unconditional_jump ? (1 << 12) : 0) |
			(loop_count_greater_zero ? (1 << 13) : 0) |
			((dig_in_bit_nr & 0x7) << 5) |
			(condition_dig_in ? (1 << 8) : 0) |
			command);

	unsigned __int32 high_buffer = jump_length & 0xFF;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


void CEthernetControllerFirefly::AddCommandWriteSystemTimeToInputMemory() {
	uint8_t command = CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM;
	uint32_t low_buffer =  (0x1F & command);
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
	AddCommandWait(4);//Writing FPGA time to input memory takes 4 clock cycles. We need to wait these 3 additional clock cycles, as the next command could also try to write and then we have a conflict.
}


/*
CMD_CALC_AD9854_FREQUENCY_TUNING_WORD: begin
							//we assume that the result of the ADC conversion is available in input_buf_mem_data_SPI
							AD9854FTWIntermediate[15:0] <= input_buf_mem_data_SPI[15:0];
							AD9854FTWIntermediate[47:16] <= 0;
							calc_ad9854_ftw_state <= CALC_AD9854_FTW_START;
							address <= address + 1;
							wait_time <= 3; //wait for ftw calculation to finish. a wait_time of 2 should be sufficient. 3 for safety.
						end


...


//The frequency tuning word is proportional to the period of the DDS output frequency, i.e. it's 1/frequency
			//The desired frequency is
			// f = f0 + c * voltage = f0 + deltaf
			// voltage is the 16-bit value provided by the DAC
			//We don't want to use a multiplication. Instead we approximate, using epsilon = deltaf/f0 << 1.
			// delta f = c* voltage
			// ftw = 1/f = 1/(f0 + deltaf) = 1/ (f0 * (1 + deltaf/f0)) = (1/f0)*(1/(1+epsilon)) ~ ftw0 * (1-epsilon)
			//     = ftw0 - ftw0*deltaf/f0 = ftw0 - ftw0 * ftw0 * c * voltage = ftw0 - scale * ADC_value
			//We replace the multiplication by a bitshift, i.e. we allow scale = 2^n with n=[0...32].
			case (calc_ad9854_ftw_state)
				CALC_AD9854_FTW_IDLE: begin
					send_AD9854_ftw <= 0;
					end
				CALC_AD9854_FTW_START: begin
					//shift 16-bit ADC value by up to 32 bit to align with 48-bit ftw
					//here we assume unsigend bit shift. For signed, use <<<
					AD9854FTWShifted <= AD9854FTWIntermediate << command_buffer[12:8];
					calc_ad9854_ftw_state <= CALC_AD9854_FTW_STOP;
				end
				CALC_AD9854_FTW_STOP: begin
				   AD9854FTW = command_buffer[63:16] - AD9854FTWShifted; //user provided FTW - shifted ADC value
				   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
				   send_AD9854_ftw <= 1;
				   AD9854_ftw_byte_shifts_to_do <= 6;
				end
				default: begin
				   calc_ad9854_ftw_state <= CALC_AD9854_FTW_IDLE;
				end
			endcase
*/

void CEthernetControllerFirefly::AddCommandCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift) {
	unsigned char command = CMD_CALC_AD9854_FREQUENCY_TUNING_WORD;
	uint16_t help = bit_shift;
	uint32_t help2 = (ftw0 << 16) & 0xFFFF0000;
	uint32_t low_buffer = (ftw0 << 16) & (help << 8) & (0x1F & command);
	uint32_t high_buffer = ftw0 >> 16;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer, /* duration_in_FPGA_clock_cycles*/ 4);
}




/*
EXTENDED_CMD_LOAD_SPI_TIMING: begin
                            extended_command[10:5] <= 0;
                            SPI_delay_CS_low_start_wait[9:0] <= extended_command[20:11];
                            SPI_delay_write[9:0] <= extended_command[30:21];
                            SPI_delay_pause_before_read[11:0] <= extended_command[42:31];
                            SPI_delay_read[9:0] <= extended_command[52:43];
                            SPI_delay_CS_low_end_wait[9:0] <= extended_command[62:53];
							*/
void CEthernetControllerFirefly::AddCommandSetSPITiming(uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait) {
	unsigned char ext_command = EXTENDED_CMD_LOAD_SPI_TIMING;
	//low buffer: bits 31:0
	if (SPI_delay_CS_low_start_wait < 1) SPI_delay_CS_low_start_wait = 1;
	uint32_t low_buffer =  ((SPI_delay_pause_before_read & 0x01) << (10+10+6+5)) |  ((SPI_delay_write & 0x3FF) << (10+6+5)) | ((SPI_delay_CS_low_start_wait & 0x3FF) << (6+5)) | ((0x3F & ext_command) << 5) | (0x1F & CMD_LOAD_EXTENDED_COMMAND);
	//high buffer: bits 63:32
	uint32_t high_buffer = ((SPI_delay_CS_low_end_wait & 0x3FF) << (11+10)) | ((SPI_delay_read & 0x3FF) << 11) | ((SPI_delay_pause_before_read & 0xFFF) >> 1);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


/*
EXTENDED_CMD_SET_SPI_MODE:
							SPI_CPOL  <= extended_command[12:12];
							SPI_CPHA <= extended_command[11:11];

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
void CEthernetControllerFirefly::AddCommandSetSPIMode(uint8_t SPI_mode) {
	unsigned char ext_command = EXTENDED_CMD_SET_SPI_MODE;
	if (SPI_mode > 3) SPI_mode = 0;
	//low buffer: bits 31:0
	uint32_t low_buffer = ((SPI_mode & 0x3) << (6 + 5)) | ((0x3F & ext_command) << 5) | (0x1F & CMD_LOAD_EXTENDED_COMMAND);
	//high buffer: bits 63:32
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

/*
I2C_0_Destination <= extended_command[16:16];
                                if (extended_command[56:17] != 0) begin
                                    I2C_delay_start_stop <= (extended_command[24:17] == 0) ? 1 : extended_command[24:17];
                                    I2C_delay_data_setup <= (extended_command[32:25] == 0) ? 1 : extended_command[32:25];
                                    I2C_delay_clock_high <= (extended_command[40:33] == 0) ? 1 : extended_command[40:33];
                                    I2C_delay_clock_low <= (extended_command[48:41] == 0) ? 1 : extended_command[48:41];
                                    I2C_delay_pause_before_read <= extended_command[56:49];
                                end
		I2C_0_Destination <= 0;
        I2C_delay_start_stop <= 60;
        I2C_delay_data_setup <= 40;
        I2C_delay_clock_high <= 60;
        I2C_delay_clock_low <= 150;
        I2C_delay_pause_before_read <= 0;
*/
void CEthernetControllerFirefly::AddCommandSetI2CParameters(uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read) {
	unsigned char ext_command = EXTENDED_CMD_SET_I2C_PARAMETERS;
	//low buffer: bits 31:0
	uint32_t low_buffer = ((I2C_delay_data_setup & 0x7F) << 25) | ((I2C_delay_start_stop & 0xFF) << 17) | ((I2C_0_Destination & 0x01) << 16) | ((0x3F & ext_command) << 5) | (0x1F & CMD_LOAD_EXTENDED_COMMAND);
	//high buffer: bits 63:32
	uint32_t high_buffer = ((I2C_delay_pause_before_read & 0xFF) << 17) | ((I2C_delay_clock_low & 0xFF) << 9) | ((I2C_delay_clock_high & 0xFF) << 1) | ((I2C_delay_data_setup & 0x80) >> 7);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


/*
//Vitis command:
void AddCommandAnalogInOut(u8 adc_register_address, u8 adc_write_enable, u16 adc_programming_out, u8 dont_execute_now, u8 only_read_write, u32 wait_time) {
	const u8 command_mask = 0x1F;  //5 bit
	u8 command = 24; //CMD_ANALOG_IN_OUT
	source[next_command * 2 + 0] = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (command_mask & command);
	source[next_command * 2 + 1] = ((wait_time & 0x3FFF) << (34 - 32)) | ((dont_execute_now & 0x01) << (32 - 32)) | ((only_read_write & 0x01) << (33 - 32));
	next_command++;
}*/

//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandAnalogInOut(uint8_t adc_register_address, uint8_t adc_write_enable, uint16_t adc_programming_out, uint8_t dont_execute_now, uint8_t only_read_write, uint32_t wait_time) {





	/*
		CMD_ANALOG_IN_OUT: begin
                            adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
                            adc_write_enable <= command[15:15];
                            adc_programming_out <= command[31:16];
                            wait_time[29:0] <= command[63:34];
                            wait_time[47:30] <= 0;
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
                                if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
                                else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
                            end
                        end*/
	uint8_t command = CMD_ANALOG_IN_OUT;
	uint32_t low_buffer = ((adc_programming_out & 0xFFFF) << 16) | ((adc_write_enable & 0x01) << 15) | ((adc_register_address & 0x7F) << 8) | (0x1F & command);
	uint32_t high_buffer = ((wait_time & 0x3FFF) << (34 - 32)) | ((dont_execute_now & 0x01) << (32 - 32)) | ((only_read_write & 0x01) << (33 - 32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}



void CEthernetControllerFirefly::AddCommandSetCoreOption_LED(bool a_core_option_LED) {
	core_option_LED = a_core_option_LED;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSwitchBuzzer(bool OnOff) {
	if (OnOff) core_option_dig_out |= 128; else core_option_dig_out &= ~128;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_dig_out(uint8_t a_core_option_dig_out) {
	core_option_dig_out = a_core_option_dig_out;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_PL_to_PS(uint8_t a_core_option_PL_to_PS) {
	core_option_PL_to_PS = a_core_option_PL_to_PS;
	AddCommandSetCoreOptions();
}

void CEthernetControllerFirefly::AddCommandSetCoreOption_SPI_CS(uint8_t a_core_option_SPI_CS) {
	core_option_SPI_CS = a_core_option_SPI_CS;
	AddCommandSetCoreOptions();
}

//Vitis command:
/*
	CMD_SET_OPTIONS:begin
			   options <= command[63:32];
						end

		options[0] = core_options_LED;
		options[1..4] = SPI_CS;
		options[8..15] = core_dig_out;
		options[24..31] = options_PL_to_PS;

	*/
	//Vitis command converted to Visual Studio command:
void CEthernetControllerFirefly::AddCommandSetCoreOptions() {
	uint8_t command = CMD_SET_OPTIONS;
	uint32_t low_buffer = command;
	uint32_t high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}
//Vitis command:
/*
CMD_PL_TO_PS_COMMAND: begin
PL_to_PS_command <= command_buffer[23:8];
address <= address + 1;
wait_time <= 1;
end
*/
//Vitis command converted to Visual Studio command:
constexpr unsigned int PLToPSCommandTest = 1 << 8;
constexpr unsigned int PLToPSCommandIgnoreTCPIP = (2<<8) | 1;
constexpr unsigned int PLToPSCommandAcceptTCPIP = (2 << 8);
void CEthernetControllerFirefly::AddCommandSetPLToPSCommand(unsigned int PLToPSCommand) {
	uint8_t command = CMD_PL_TO_PS_COMMAND;
	uint32_t low_buffer = ((PLToPSCommand & 0xFFFF) << 8) | command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}


void CEthernetControllerFirefly::AddCommandWait(unsigned long Wait_in_FPGA_clock_cycles) {
	/*
			 CMD_STEP:begin
							wait_time[30:0] <= command[35:5];
							wait_time[47:31] <= 0;
							bus_data <= command[63:36];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end
			*/
	uint8_t command = CMD_STEP;
	if (Wait_in_FPGA_clock_cycles <2) Wait_in_FPGA_clock_cycles = 2;
	uint32_t low_buffer = ((Wait_in_FPGA_clock_cycles - 1) <<5 ) | command; //The FPGA firmware always adds one wait cycle, which is used to decode the command
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddCommandStop() {
	uint8_t command = CMD_STOP;
	uint32_t low_buffer = command;
	uint32_t high_buffer = 0;
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddCommandTransmitI2C(uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t *data_out) {

	//total data length can be up to 128-(2*5) = 118 bits (because we need 2x 5 bits for the two load commands).
	uint8_t command = CMD_LOAD_REG_LOW;
	uint64_t data_low = static_cast<uint64_t>(data_out[0]);
	uint64_t data_high = static_cast<uint64_t>(data_out[1]);
	uint32_t low_buffer = ((data_low & 0x07FFFFFF) << 5)  | command; //lowest 27 bit
	uint32_t high_buffer = (data_low >> 27) & 0xFFFFFFFF; //bit 28 to 59 (5 bits missing)
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);

	command = CMD_LOAD_REG_HIGH;
	low_buffer = ((data_high & 0x003FFFFF) << 10) | ((data_low >> (27 + 32)) & 0x1F) | command; /*last 5 bits of data_low; if needed data can be extended*/
	high_buffer = (data_high >> 22) & 0xFFFFFFFF;  //data_high can have up 54 bits.
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);

	/*
	CMD_I2C_OUT : begin
							 I2C_out_length <= command_buffer[14:8];
                            I2C_in_length <= command_buffer[20:15];
                            I2C_SELECT_NEXT <= command_buffer[21:20];
                            I2C_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_I2C_OUT to copy 117-bit I2C data to register
                            I2C_state <= I2C_START;
                            address <= address + 1;
                            wait_time <= 1;
						end
	*/
	command = CMD_I2C_OUT;
	low_buffer = ((I2C_port & 0x03) << 20) | ((I2C_length_in & 0x3F) << 15) | ((I2C_length_out & 0x7F) << 8) | (0x1F & command);
	high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}


uint8_t reverse_byte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

void reverse_bits_array(const uint8_t* data_out,
	uint8_t* result,
	uint8_t number_of_bytes)
{
	for (uint8_t i = 0; i < number_of_bytes; ++i) {
		result[number_of_bytes - i - 1] = reverse_byte(data_out[i]);
	}
}

void CEthernetControllerFirefly::AddCommandTransmitSPI(const uint8_t chip_select, const uint16_t number_of_bits_out, const uint8_t data_out[], uint8_t number_of_bits_in, const bool start_now, const bool wait_for_SPI_ready_active, const bool wait_for_SPI_ready_edge_to_active, const bool SPI_ready_active_level, const bool LSB_first) {

	uint64_t data_low;
	uint64_t data_high;
	if (LSB_first) {
		uint8_t MSBfirst_data_out[16];
		uint16_t number_of_bytes = (number_of_bits_out + 7) / 8;
		reverse_bits_array(data_out,
			MSBfirst_data_out,
			number_of_bytes);

		data_low = static_cast<uint64_t>(((uint64_t*)MSBfirst_data_out)[0]);
		data_high = static_cast<uint64_t>(((uint64_t*)MSBfirst_data_out)[1]);
	}
	else {
		data_low = static_cast<uint64_t>(((uint64_t*)data_out)[0]);
		data_high = static_cast<uint64_t>(((uint64_t*)data_out)[1]);
	}

	//total data length can be up to 128-(2*5) = 118 bits (because we need 2x 5 bits for the two load commands).
	uint8_t command = CMD_LOAD_REG_LOW;

	uint32_t low_buffer = ((data_low & 0x07FFFFFF) << 5)  | command; //lowest 27 bit
	uint32_t high_buffer = (data_low >> 27) & 0xFFFFFFFF; //bit 28 to 59 (5 bits missing)
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);

	command = CMD_LOAD_REG_HIGH;
	low_buffer = ((data_high & 0x003FFFFF) << 10) | ((data_low >> (27 + 32)) & 0x1F) | command; /*last 5 bits of data_low; if needed data can be extended*/
	high_buffer = (data_high >> 22) & 0xFFFFFFFF;  //data_high can have up 54 bits.
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);

	/*
	CMD_SPI_OUT_IN : begin
                            SPI_OUT_length <= command[14:8];
                            SPI_IN_length <= command[21:16];
                            SPI_SEL_next <= command[33:32];
                            SPI_chip_select_next <= command[37:34];
                            SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[40:40] == 0) SPI_state <= SPI_START;
                        end
	*/

	command = CMD_SPI_OUT_IN;
	constexpr unsigned char SPI_port = 1; //only SPI port 1 is right now available to users (2026 04 04)
	unsigned char SPI_SEL_next = (SPI_port == 0) ? 0x01 : 0x02; //2 SPI ports, port 0 is controlled by bit zero here, port 1 is bit one here; 1 means active under PL control; if inactive they are under PS control
	unsigned char SPI_chip_select_next = chip_select & 0x03;// 2 + 4 + 8; //3 CS lines, which go to the input of a multiplexer on the serial port board, or can be used directly, if put
	unsigned int start_now_i = (start_now) ? 0 : 1;
	if (number_of_bits_in > 32) number_of_bits_in = 32;
	low_buffer =  ((number_of_bits_in & 0x3F) << 16) | ((number_of_bits_out & 0x7F) << 8) | (((wait_for_SPI_ready_active) ? 1 : 0) << 7) | (((wait_for_SPI_ready_edge_to_active) ? 1 : 0) << 6) | (((SPI_ready_active_level) ? 1 : 0) << 5) | command;
	high_buffer = (start_now_i << (40-32)) | (SPI_chip_select_next << 2) | (0x03 & SPI_SEL_next);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddCommandRepeatedOutIn(const uint16_t number_of_datapoints, const double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand, const bool SPI_restart_wait_on_ready_low) {
	uint8_t command = CMD_INPUT_REPEATED_OUT_IN;
	unsigned __int32 INPUT_REPEAT_repeats = number_of_datapoints;
	unsigned __int32 INPUT_REPEAT_wait = floor(delay_between_datapoints_in_ms * FPGAClockFrequencyInHz / 1000);
	unsigned __int32 INPUT_REPEAT_command = RepeatedOutInCommand;  //0: stop, 1: SPI input, 2: digital input, 3: dig_event_time_tagger, 4: analog in
	unsigned __int32 INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 1;//this is needed if input BRAM buffer should be copied to DDR when half buffer full

	uint32_t low_buffer = ((INPUT_REPEAT_repeats & 0xFFFFF) << 8) | (((SPI_restart_wait_on_ready_low) ? 1 : 0) << 7) | command;
	uint32_t high_buffer = (INPUT_REPEAT_wait & 0xFFFFFF)  | (INPUT_REPEAT_trigger_secondary_interrupt_when_finished << (56 - 32)) | ((INPUT_REPEAT_command & 0x7) << (57 - 32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}


void CEthernetControllerFirefly::StartSPIAnalogInAcquisition(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {
	//ToDo: As soon as Control is based on ControlLight, we should move the code here into CDeviceSequencer::StartSPIAnalogInAcquisition
	//and replace the AddSequencerCommandToSequenceList commands with
	//AddCommandSetSPITiming
	//AddCommandTransmitSPI
	//AddCommandRepeatedOutIn
	//commands

	// * @param analog_in_type Analog in board type. 0: AQuRA MCP3208 analog in board; 1: MCP3208 12-bit ADC on SerialPortBoard; 2: ADS1256  24-bit ADC

	if ((analog_in_type==0) || (analog_in_type==1)) StartSPIAnalogInAcquisition_MCP3208(analog_in_type, SPI_CS, channel_nr, number_of_datapoints, delay_between_datapoints_in_ms);
	else if (analog_in_type==2) StartSPIAnalogInAcquisition_ADS1256(analog_in_type, SPI_CS, channel_nr, number_of_datapoints, delay_between_datapoints_in_ms);
}

void CEthernetControllerFirefly::StartSPIAnalogInAcquisition_MCP3208(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {

	//This code is written in an outdated fashion. Look at StartSPIAnalogInAcquisition_ADS1256 to find a more modern way to write it.

	//@param analog_in_type Analog in board type. 0: AQuRA MCP3208 analog in board; 1: MCP3208 12-bit ADC on SerialPortBoard; 2: ADS1256  24-bit ADC

	//only for debugging
	//AddCommandSetCoreOption_LED(true);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 0, /*input_buf_mem_data*/0xDEADBEEF, /* wait_time_in_FPGA_cycles*/ 5);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 1, /*input_buf_mem_data*/0x1234ABCD, /* wait_time_in_FPGA_cycles*/ 5);

	//if (analog_in_type==1) {
	AddCommandSetSPITiming(/* SPI_delay_CS_low_start_wait*/ 4, /* SPI_delay_write*/ 4, /* SPI_delay_pause_before_read*/ 4, /* SPI_delay_read*/ 23, /* SPI_delay_CS_low_end_wait*/ 4);
	AddCommandSetSPIMode(/* SPI_mode */ 0);
	//}


	/*
	CMD_LOAD_REG_LOW:begin
                            register[58:0] <= command[63:5];
                        end
    CMD_LOAD_REG_HIGH:begin
                            register[117:59] <= command[63:5];
                        end
	*/

	if (channel_nr > 7) channel_nr = 7;

	unsigned __int32 SPI_SINGLE_ENDED_INPUT = 1;
	//old code with LSB first
	//unsigned __int32 SPI_ANALOG_IN_NR = channel_nr;
	//unsigned __int32 SPI_IN_NR_REVERSED = (((SPI_ANALOG_IN_NR & 1)>0) ? 4 : 0) + (((SPI_ANALOG_IN_NR & 2)>0) ? 2 : 0) + (((SPI_ANALOG_IN_NR & 4)>0) ? 1 : 0);
	//unsigned __int32 SPI_DATA_REVERSED = 1 + (SPI_SINGLE_ENDED_INPUT << 1) + (SPI_IN_NR_REVERSED << 2);
	//SPI_DATA = 1+2;

	//Data is written MSB first
	unsigned __int32 SPI_DATA = (1<<5) + (SPI_SINGLE_ENDED_INPUT << 4) + (channel_nr << 1);


	unsigned char command = CMD_LOAD_REG_LOW;
	unsigned __int32 low_buffer = (SPI_DATA << 5) | command;
	unsigned __int32 high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	command = CMD_LOAD_REG_HIGH;
	low_buffer = command;
	high_buffer = 0;
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);


	/*
	CMD_SPI_OUT_IN : begin
                            SPI_OUT_length <= command[14:8];
                            SPI_IN_length <= command[20:16];
                            SPI_SEL_next <= command[33:32];
                            SPI_chip_select_next <= command[37:34];
                            SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register
                            INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
                            if (command[40:40] == 0) SPI_state <= SPI_START;
                        end
	*/

	command = CMD_SPI_OUT_IN;
	unsigned __int32 SPI_out_length = 6;
	unsigned __int32 SPI_in_length = 13;
	//unsigned __int32 SPI_CS = 1;
	unsigned __int32 wait_time = 0;
	constexpr unsigned char SPI_port = 1;
	unsigned char SPI_SEL_next = (SPI_port == 0) ? 0x01 : 0x02; //2 SPI ports, port 0 is controlled by bit zero here, port 1 is bit one here; 1 means active under PL control; if inactive they are under PS control
	unsigned char SPI_chip_select_next;
	if (analog_in_type==0) {
		SPI_chip_select_next = (~(1 << SPI_CS)) & 0x0F;// 2 + 4 + 8; //4 CS lines, low means active; ToDo: in V2 we intend to install a multiplexer on the backplane, then every 16 bit value is valid and the code needs to be changed here.
	} else {
		SPI_chip_select_next = SPI_CS & 0x03;// 2 + 4 + 8; //3 CS lines, which go to the input of a multiplexer on the serial port board, or can be used directly, if put
	}
	unsigned int start_now = 0;
	low_buffer =  (SPI_in_length << 16) | (SPI_out_length << 8) | command;
	high_buffer = (start_now << (40-32)) | (SPI_chip_select_next << 2) | (0x03 & SPI_SEL_next);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	/*

	CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
							INPUT_REPEAT_repeats <= command[27:8];
							INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
							INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
							INPUT_REPEAT_state <= INPUT_REPEAT_START;
							INPUT_REPEAT_command <= command[59:57];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end

					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
					if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
					if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;

	*/
	command = CMD_INPUT_REPEATED_OUT_IN;
	unsigned __int32 INPUT_REPEAT_repeats = number_of_datapoints;
	unsigned __int32 INPUT_REPEAT_wait = floor(delay_between_datapoints_in_ms * FPGAClockFrequencyInHz / 1000);
	unsigned __int32 INPUT_REPEAT_command = 1;  //SPI input
	unsigned __int32 INPUT_REPEAT_trigger_secondary_interrupt_when_finished = 1;//this is needed if input BRAM buffer should be copied to DDR when half buffer full

	low_buffer = (INPUT_REPEAT_repeats << 8) | command;
	high_buffer = INPUT_REPEAT_wait | (INPUT_REPEAT_trigger_secondary_interrupt_when_finished << (56 - 32)) | (INPUT_REPEAT_command << (57 - 32));
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);

	//unsigned __int32 DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency);
	//if (DelayMultiplier < 2) DelayMultiplier = 2;
	//if (DelayMultiplier == 1) {
	//	AddCommandStep(0, DelayMultiplier * 3 - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
	//	AddCommandStep(0, DelayMultiplier * 3 - 2); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
	//} else {
		//AddCommandStep( 0, DelayMultiplier * 3 - 4); //CMD_STEP doing nothing, just in order to keep the time calculated by COutput aligned with the time used by the FPGA; needed because the four commands above only consume 4 FPGA cycles, but are accounted for with 2 bus cycles by COutput
//	}

	//debugging of BRAM operation only
	//for (unsigned long n=0;n<0xFF;n++)
	//	AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ n*2, /*input_buf_mem_data*/0xDEAD0000+n, /* wait_time_in_FPGA_cycles*/ 5);

}

void CEthernetControllerFirefly::AddDelay_in_ns(uint32_t delay_in_nanoseconds) {
	AddCommandStep( 0, delay_in_nanoseconds * FPGAClockFrequencyInHz / (1000.0*1000.0*1000.0));
}

void CEthernetControllerFirefly::StartSPIAnalogInAcquisition_ADS1256(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms) {
	#define clockspeedMhz 7.68  //ToDo: adjust to your board's clock speed
	// ADS1256 Register address
	#define ADS1256_RADD_STATUS 0x00
	#define ADS1256_RADD_MUX 0x01
	#define ADS1256_RADD_ADCON 0x02
	#define ADS1256_RADD_DRATE 0x03
	#define ADS1256_RADD_IO 0x04
	#define ADS1256_RADD_OFC0 0x05
	#define ADS1256_RADD_OFC1 0x06
	#define ADS1256_RADD_OFC2 0x07
	#define ADS1256_RADD_FSC0 0x08
	#define ADS1256_RADD_FSC1 0x09
	#define ADS1256_RADD_FSC2 0x0A
	// ADS1256 Command
	#define ADS1256_CMD_WAKEUP 0x00
	#define ADS1256_CMD_RDATA 0x01
	#define ADS1256_CMD_RDATAC 0x03
	#define ADS1256_CMD_SDATAC 0x0f
	#define ADS1256_CMD_RREG 0x10
	#define ADS1256_CMD_WREG 0x50
	#define ADS1256_CMD_SELFCAL 0xF0
	#define ADS1256_CMD_SELFOCAL 0xF1
	#define ADS1256_CMD_SELFGCAL 0xF2
	#define ADS1256_CMD_SYSOCAL 0xF3
	#define ADS1256_CMD_SYSGCAL 0xF4
	#define ADS1256_CMD_SYNC 0xFC
	#define ADS1256_CMD_STANDBY 0xFD
	#define ADS1256_CMD_RESET 0xFE

	//only for debugging
	//AddCommandSetCoreOption_LED(true);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 0, /*input_buf_mem_data*/0xDEADBEEF, /* wait_time_in_FPGA_cycles*/ 5);
	//AddCommandWriteInputBuffer(/*write_next_address*/ false, /* input_buf_mem_address */ 1, /*input_buf_mem_data*/0x1234ABCD, /* wait_time_in_FPGA_cycles*/ 5);
	if (number_of_datapoints == 0) return;
	uint16_t SPI_clock_period_10ns = floor(100 * 1.0/ (clockspeedMhz / 4)); //in 10ns; this is the period of the SPI clock, which is used for timing in the FPGA; ToDo: adjust to your board's clock speed
	uint16_t Wait_before_read_10ns = 700;              //  t6 delay (4*tCLKIN 50*0.13 = 6.5 us)
	AddCommandSetSPITiming(/* SPI_delay_CS_low_start_wait*/ SPI_clock_period_10ns, /* SPI_delay_write*/ SPI_clock_period_10ns, /* SPI_delay_pause_before_read*/ Wait_before_read_10ns, /* SPI_delay_read*/ SPI_clock_period_10ns, /* SPI_delay_CS_low_end_wait*/ SPI_clock_period_10ns);
	AddCommandSetSPIMode(/* SPI_mode */ 1);

	//_spi->beginTransaction(SPISettings(clockspeedMhz * 1000000 / 4, MSBFIRST, SPI_MODE1));
	uint8_t MUXP = (channel_nr & 0x07) <<4;
	constexpr uint8_t MUXN = 0x08;
	uint8_t MUX_CHANNEL = MUXP | MUXN;

	uint8_t data_out_channel_select[3] = { MUX_CHANNEL, 0, ADS1256_CMD_WREG | ADS1256_RADD_MUX }; // {LSByte, ..., MSByte}; opcode WREG (Write registers) starting from reg RADD_MUX; MSB written first
	AddCommandTransmitSPI(SPI_CS, /* number_of_bits_out*/ 24, data_out_channel_select, /* number_of_bits_in*/ 0, /* start_now*/ true);

	AddDelay_in_ns(26 * 20 * SPI_clock_period_10ns); //AddDelay_in_ns(10 * 1000);//AddDelay_in_ns(41 * 1000);// +26 * 20 * SPI_clock_period_10ns);

	uint8_t data_out_sync = ADS1256_CMD_SYNC;
	AddCommandTransmitSPI(SPI_CS, /* number_of_bits_out*/ 8, &data_out_sync, /* number_of_bits_in*/ 0, /* start_now*/ true);

	AddDelay_in_ns(10 * 20 * SPI_clock_period_10ns);//AddDelay_in_ns(41 * 1000);// +10 * 20 * SPI_clock_period_10ns);

	uint8_t data_out_wakeup = ADS1256_CMD_WAKEUP;
	AddCommandTransmitSPI(SPI_CS, /* number_of_bits_out*/ 8, &data_out_wakeup, /* number_of_bits_in*/ 0, /* start_now*/ true);

	AddDelay_in_ns(206 * 1000);// +10 * 20 * SPI_clock_period_10ns);

	uint8_t data_out_rdata = ADS1256_CMD_RDATA;

	AddCommandTransmitSPI(SPI_CS, /* number_of_bits_out*/ 8, &data_out_rdata, /* number_of_bits_in*/ 24, /* start_now*/ number_of_datapoints == 1, /* wait_for_SPI_ready_active */ false, /*wait_for_SPI_ready_edge_to_active*/ false);
	if (number_of_datapoints>1)	AddCommandRepeatedOutIn(/* number_of_datapoints*/ number_of_datapoints, /* delay_between_datapoints_in_ms*/ delay_between_datapoints_in_ms, /* RepeatedOutInCommand*/ 1, /*SPI_restart_wait_on_ready_low*/ false); //1 means repeated SPI input
}


void CEthernetControllerFirefly::SwitchDebugLED(bool OnOff) {
	AddCommandSetCoreOption_LED(OnOff);
}

void CEthernetControllerFirefly::IgnoreTCPIP(bool OnOff) {
	AddCommandSetPLToPSCommand((OnOff) ? PLToPSCommandIgnoreTCPIP : PLToPSCommandAcceptTCPIP);
}

void CEthernetControllerFirefly::AddMarker(uint8_t Marker) {
	AddCommandSetPLToPSCommand(PLToPSCommandTest | Marker);
}

void CEthernetControllerFirefly::AddCommandStep(uint32_t data, uint32_t delay) {
	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	uint8_t command = CMD_STEP;

	if (delay > 0x7FFFFFFF) { //31 bit
		AddErrorMessage("CEthernetControllerFirefly::AddCommandStep : delay too long.");
		return;
	}
	else if (delay < 1) {
		AddErrorMessage("CEthernetControllerFirefly::AddCommandStep : delay too short.");
		delay = 1;
	}

	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
	AddSequencerCommandToSequenceList( high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddCommandStepSPI(uint32_t data, uint32_t delay, bool bus_strobe_first_part, bool bus_strobe_second_part, bool bus_strobe_idle_part, bool bus_data15_second_part, bool bus_data15_idle_part) {
	const uint32_t delay_mask = 0x03FFFFFF; //26 bit
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F; //5 bit
	uint8_t command = CMD_STEP_SPI;

	if (delay > 0x03FFFFFF) {
		AddErrorMessage("CEthernetControllerFirefly::AddCommandStepSPI : delay too long.");
		return;
	}
	else if (delay < 1) {
		AddErrorMessage("CEthernetControllerFirefly::AddCommandStepSPI : delay too short.");
		delay = 1;
	}

	uint32_t low_buffer = ((delay & delay_mask) << 5) |
		((bus_data15_second_part ? 1u : 0u) << 31) |
		(command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) |
		((bus_data15_idle_part ? 1u : 0u) << 0) |
		((bus_strobe_first_part ? 1u : 0u) << 1) |
		((bus_strobe_second_part ? 1u : 0u) << 2) |
		((bus_strobe_idle_part ? 1u : 0u) << 3);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);
}

void CEthernetControllerFirefly::AddProgramLine( uint8_t command, uint32_t data, uint32_t delay ) {
	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
	const uint32_t bus_data_mask = 0x0FFFFFFF;
	const uint8_t command_mask = 0x1F;  //5 bit
	//uint8_t command = 1; //CMD_STEP

	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
	AddSequencerCommandToSequenceList(high_buffer, low_buffer);


	/*if (DebugBufferFile) {
		(*DebugBufferFile) << std::format("%8u CMD %x data = {:08X} wait = {:08X}", command, data, delay) << endl;
	}*/
}

//void CEthernetControllerFirefly::AddCommandStep( uint32_t data, uint32_t delay) {
//	const uint32_t delay_mask_low = 0x7FFFFFF; //27 bit
//	const uint32_t delay_mask_high = 0x0F; // 4 bit -> total of 31 bits
//	const uint32_t bus_data_mask = 0x0FFFFFFF;
//	const uint8_t command_mask = 0x1F;  //5 bit
//	uint8_t command = 1; //CMD_STEP
//
//	uint32_t low_buffer = ((delay & delay_mask_low) << 5) + (command_mask & command);
//	uint32_t high_buffer = ((bus_data_mask & data) << 4) | ((delay >> 27) & delay_mask_high);
//
//	if (DebugBufferFile) {
//		(*DebugBufferFile) << std::format("step data = {:08X} {:08X}", high_buffer, low_buffer) << endl;
//	}
//	MySequencer->AddCommandStepToSequence(high_word, low_word);
//
//}

void CEthernetControllerFirefly::SetStrobeOptions(uint8_t strobe_choice, uint8_t strobe_low_length, uint8_t strobe_high_length) {
	const uint8_t command_mask = 0x1F;  //5 bit
	uint8_t command = CMD_SET_STROBE_OPTIONS;

	if (strobe_low_length < 1) strobe_low_length = 1;
	if (strobe_high_length < 1) strobe_high_length = 1;

	uint32_t low_buffer = (((strobe_high_length - 1) & 0xFF) << 24) | (((strobe_low_length - 1) & 0xFF) << 16) | ((strobe_choice & 0x07) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);
	uint32_t high_buffer = 0;
	AddSequencerCommand(high_buffer, low_buffer);

	low_buffer = (((strobe_high_length - 1) & 0xFF) << 24) | (((strobe_low_length - 1) & 0xFF) << 16) | ((strobe_choice & 0x07) << 8) | (command_mask & command);
	high_buffer = 0;
	AddSequencerCommand(high_buffer, low_buffer);

}

void CEthernetControllerFirefly::AddExternalTrigger( bool ExternalTrigger0, bool ExternalTrigger1, bool FPGASoftwareTrigger) {
	if (ExternalTrigger0 || ExternalTrigger1 || FPGASoftwareTrigger) {
		//CMD_WAIT_FOR_TRIGGER: begin
		//	if ((trigger_0 && (command[8:8] == 1)) || (trigger_1 && (command[9:9] == 1)) || (trigger_PS && (command[10:10] == 1))) address <= address + 1;
		//end

		const uint8_t command_mask = 0x1F;  //5 bit
		uint8_t command = CMD_WAIT_FOR_TRIGGER;
		uint8_t trigger0 = (ExternalTrigger0) ? 1 : 0;
		uint8_t trigger1 = (ExternalTrigger1) ? 2 : 0;
		uint8_t softtrigger = (ExternalTrigger1) ? 4 : 0;

		uint32_t low_buffer = ((trigger0 | trigger1 | softtrigger) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);
		uint32_t high_buffer = 0;
		AddSequencerCommand(high_buffer, low_buffer);
		low_buffer = ((trigger0 | trigger1 | softtrigger) << 8) | (command_mask & command);
		high_buffer = 0;
		AddSequencerCommand(high_buffer, low_buffer);

	}
	else {
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
	}

}

void CEthernetControllerFirefly::SetTriggerOptions( bool ExternalTrigger0, bool ExternalTrigger1) {
	uint32_t low_buffer;
	uint32_t high_buffer;
	if (SetPeriodicTriggerAtBeginningOfNextSequence && (PeriodicTriggerPeriod_in_ms > 0)) {
		SetPeriodicTriggerAtBeginningOfNextSequence = false;
		//CMD_SET_PERIODIC_TRIGGER_PERIOD: begin
		//	periodic_trigger_period <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
		//end
		uint64_t PeriodicTriggerPeriod = floor(PeriodicTriggerPeriod_in_ms * FPGAClockFrequencyInHz / 1000);
		const uint8_t command_mask = 0x1F;  //5 bit
		uint8_t command = CMD_SET_PERIODIC_TRIGGER_PERIOD;
		low_buffer = ((PeriodicTriggerPeriod & 0xFFFFFF) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);  //low 24 bit << 8
		high_buffer = (PeriodicTriggerPeriod >> 24) & 0xFFFFFF; // high 24 bit
		AddSequencerCommand(high_buffer, low_buffer);
		low_buffer = ((PeriodicTriggerPeriod & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
		high_buffer = (PeriodicTriggerPeriod >> 24) & 0xFFFFFF; // high 24 bit
		AddSequencerCommand(high_buffer, low_buffer);

		//CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME: begin
		//	periodic_trigger_allowed_wait_cycles <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
		//end
		uint64_t PeriodicTriggerAllowedWaitCycles = floor(PeriodicTriggerAllowedWait_in_ms * FPGAClockFrequencyInHz / 1000);
		command = CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME;
		low_buffer = ((PeriodicTriggerAllowedWaitCycles & 0xFFFFFF) << 8) | (command_mask & CMD_LOAD_COMMAND_BUFFER);  //low 24 bit << 8
		high_buffer = (PeriodicTriggerAllowedWaitCycles >> 24) & 0xFFFFFF; // high 24 bit
		AddSequencerCommand(high_buffer, low_buffer);
		low_buffer  = ((PeriodicTriggerAllowedWaitCycles & 0xFFFFFF) << 8) | (command_mask & command);  //low 24 bit << 8
		high_buffer = (PeriodicTriggerAllowedWaitCycles >> 24) & 0xFFFFFF; // high 24 bit
		AddSequencerCommand(high_buffer, low_buffer);
		LastPeriodicTriggerPeriod_in_ms = PeriodicTriggerPeriod_in_ms;

		if (ChangePeriodicTriggerPeriodWhileCycling) {
			//switch LED on to indicate to user that we are waiting for the periodic trigger
			core_option_LED = true;
			uint8_t command = CMD_LOAD_COMMAND_BUFFER;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			command = CMD_SET_OPTIONS;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);

			low_buffer = CMD_WAIT_FOR_PERIODIC_TRIGGER;
			high_buffer = 0;
			AddSequencerCommand(high_buffer, low_buffer);

			//switch LED off again to indicate to user that we have detected the periodic trigger and are continuing with the sequence
			core_option_LED = false;
			command = CMD_LOAD_COMMAND_BUFFER;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
			command = CMD_SET_OPTIONS;
			low_buffer = command;
			high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
			AddSequencerCommand(high_buffer, low_buffer);
		}
		else {
			AddCommandStep( 0, 1);
			AddCommandStep( 0, 1);
			AddCommandStep( 0, 1);
			AddExternalTrigger(ExternalTrigger0, ExternalTrigger1, false);
		}
	}
	else if (WaitForPeriodicTriggerAtBeginningOfSequence && (LastPeriodicTriggerPeriod_in_ms > 0)) {
		//switch LED on to indicate to user that we are waiting for the periodic trigger
		core_option_LED = true;
		uint8_t command = CMD_LOAD_COMMAND_BUFFER;
		low_buffer =  command;
		high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
		AddSequencerCommand(high_buffer, low_buffer);
		command = CMD_SET_OPTIONS;
		low_buffer = command;
		high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
		AddSequencerCommand(high_buffer, low_buffer);

		AddCommandStep( 0, 1);
		//buffer[(n + 2) * 2 + 0] = CMD_WAIT_FOR_PERIODIC_TRIGGER;
		//buffer[(n + 2) * 2 + 1] = 0;
		low_buffer = CMD_WAIT_FOR_PERIODIC_TRIGGER;
		high_buffer = 0;
		AddSequencerCommand(high_buffer, low_buffer);

		//switch LED off again to indicate to user that we have detected the periodic trigger and are continuing with the sequence
		core_option_LED = false;
		command = CMD_LOAD_COMMAND_BUFFER;
		low_buffer = command;
		high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
		AddSequencerCommand(high_buffer, low_buffer);
		command = CMD_SET_OPTIONS;
		low_buffer = command;
		high_buffer = ((core_option_PL_to_PS & 0xFF) << 24) | ((core_option_dig_out & 0xFF) << 8) | ((core_option_SPI_CS & 0x0F) << 1) | (core_option_LED & 0x01);
		AddSequencerCommand(high_buffer, low_buffer);
	}
	else {
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddCommandStep( 0, 1);
		AddExternalTrigger(ExternalTrigger0, ExternalTrigger1, false);
	}
}


//CString CommandNames[NrCommands] = { "CMD_STOP", "CMD_STEP", "CMD_STEP_AND_ENTER_FAST_MODE", "CMD_SET_OPTIONS", "CMD_LOAD_REG_LOW", "CMD_LOAD_REG_HIGH", "CMD_LATCH_STATE", "CMD_RESET_WAIT_CYCLES", "CMD_LONG_WAIT", "CMD_SET_STROBE_OPTIONS", "CMD_SET_INPUT_BUF_MEM", "CMD_WAIT_FOR_TRIGGER", "CMD_SET_LOOP_COUNT", "CMD_CONDITIONAL_JUMP_FORWARD", "CMD_CONDITIONAL_JUMP_BACKWARD", "CMD_I2C_OUT", "CMD_SPI_OUT_IN", "CMD_INPUT_REPEATED_OUT_IN", "CMD_SET_PERIODIC_TRIGGER_PERIOD", "CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME", "CMD_WAIT_FOR_PERIODIC_TRIGGER", "CMD_WAIT_FOR_WAIT_CYCLE_NR", "CMD_DIG_IN", "CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT", "CMD_ANALOG_IN_OUT", "CMD_PL_TO_PS_COMMAND" };

static uint64_t CommandWord(uint32_t high_buffer, uint32_t low_buffer) {
	return (static_cast<uint64_t>(high_buffer) << 32) | low_buffer;
}

static uint64_t CommandBits(uint64_t command_word, unsigned int first_bit, unsigned int bit_count) {
	if (bit_count == 0) return 0;
	if (bit_count >= 64) return command_word >> first_bit;
	return (command_word >> first_bit) & ((uint64_t{ 1 } << bit_count) - 1);
}

static std::string CommandName(uint8_t command) {
	if (command < NrCommands) return CommandNames[command];
	return std::format("UNKNOWN_CMD_{:02X}", command);
}

static std::string ExtendedCommandName(uint8_t command) {
	if (command < NrExtendedCommands) return ExtendedCommandNames[command];
	return std::format("UNKNOWN_EXTENDED_CMD_{:02X}", command);
}

static std::string InputRepeatCommandName(uint32_t command) {
	switch (command) {
	case 0: return "IN_REP_CMD_IDLE";
	case 1: return "IN_REP_CMD_SPI";
	case 2: return "IN_REP_CMD_DIG_IN";
	case 3: return "IN_REP_CMD_DIG_EVENT";
	case 4: return "IN_REP_CMD_ANA_IN";
	default: return std::format("UNKNOWN_IN_REP_CMD_{:01X}", command);
	}
}

void CEthernetControllerFirefly::WriteBufferToFile(uint32_t* buffer, unsigned long length, const std::string& FileName) { //length in command lines
	ofstream out;
	out.open(FileName);// , CFile::modeCreate | CFile::modeWrite);
	std::string buf;
	double time = 0;
	uint32_t low_command_buffer = 0;
	uint32_t high_command_buffer = 0;

	for (unsigned long n = 0; n < length; n++) {
		//buf.Format("%05lu: ", n);
		buf = std::format("{:05X}: t= {:04.5f} ms ", n, time);
		out << buf;
		uint32_t low_buffer = buffer[2 * n];
		uint32_t high_buffer = buffer[2 * n + 1];
		uint8_t command = low_buffer & 0x1F;
		buf = std::format("{:08X} {:08X} {}", high_buffer, low_buffer, CommandName(command));
		out << buf;
		buf = "";
		if (command == CMD_LOAD_COMMAND_BUFFER) {
			low_command_buffer = low_buffer;
			high_command_buffer = high_buffer;
			buf = std::format(" command_buffer <= {:08X} {:08X}; data will be interpreted by the following command", high_command_buffer, low_command_buffer);
		}
		else if (command < NrCommands && CommandUsesBuffer[command]) {
			low_buffer = low_command_buffer;
			high_buffer = high_command_buffer;
			buf = " using previous CMD_LOAD_COMMAND_BUFFER data;";
		}
		uint64_t command_word = CommandWord(high_buffer, low_buffer);
		if (command == CMD_STEP) {
			/*
			 CMD_STEP:begin
							wait_time[30:0] <= command[35:5];
							wait_time[47:31] <= 0;
							bus_data <= command[63:36];
							if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
							strobe_generator_state <= DELAY_CYCLE;
						end
			*/
			uint32_t delay_low = static_cast<uint32_t>(CommandBits(command_word, 5, 31)) + 1;
			uint32_t bus_data = static_cast<uint32_t>(CommandBits(command_word, 36, 28));
			buf += std::format(" bus = {:07X} ; delay = {:08X}", bus_data, delay_low);
			time += delay_low * 10 * 0.000001;
		}
		else if (command == CMD_STEP_SPI) {
			uint32_t delay_low = static_cast<uint32_t>(CommandBits(command_word, 5, 26)) + 1;
			uint32_t bus_data15_second_part = static_cast<uint32_t>(CommandBits(command_word, 31, 1));
			uint32_t bus_data15_idle_part = static_cast<uint32_t>(CommandBits(command_word, 32, 1));
			uint32_t bus_strobe_first_part = static_cast<uint32_t>(CommandBits(command_word, 33, 1));
			uint32_t bus_strobe_second_part = static_cast<uint32_t>(CommandBits(command_word, 34, 1));
			uint32_t bus_strobe_idle_part = static_cast<uint32_t>(CommandBits(command_word, 35, 1));
			uint32_t bus_data = static_cast<uint32_t>(CommandBits(command_word, 36, 28));
			buf += std::format(" bus = {:07X} ; delay = {:08X} ; bus_data15_second_part = {} ; bus_data15_idle_part = {} ; bus_strobe_first_part = {} ; bus_strobe_second_part = {} ; bus_strobe_idle_part = {}", bus_data, delay_low, bus_data15_second_part, bus_data15_idle_part, bus_strobe_first_part, bus_strobe_second_part, bus_strobe_idle_part);
			time += delay_low * 10 * 0.000001;
		}
		else if (command == CMD_STEP_AND_ENTER_FAST_MODE) {
			uint32_t delay_low = static_cast<uint32_t>(CommandBits(command_word, 5, 31));
			uint32_t bus_data = static_cast<uint32_t>(CommandBits(command_word, 36, 28));
			buf += std::format(" bus = {:07X} ; delay = {:08X} ; enter_fast_mode = 1", bus_data, delay_low);
			time += delay_low * 10 * 0.000001;
		}
		else if (command == CMD_STOP) {
			buf += " stop sequencer; running = 0 ; trigger_out = 0";
		}
		else if (command == CMD_SET_OPTIONS) {
			uint32_t options = high_buffer;
			uint32_t LED = options & 0x01;
			uint32_t SPI_CS = (options >> 1) & 0x0F;
			uint32_t core_dig_out = (options >> 8) & 0xFF;
			uint32_t PL_to_PS = (options >> 24) & 0xFF;
			buf += std::format(" options = {:08X} ; LED = {:01X} ; SPI_CS = {:01X} ; core_dig_out = {:02X} ; PL_to_PS = {:02X}", options, LED, SPI_CS, core_dig_out, PL_to_PS);
		}
		else if (command == CMD_LATCH_STATE) {
			buf += " latch current state";
		}
		else if (command == CMD_RESET_WAIT_CYCLES) {
			buf += " reset wait_cycles counter";
		}
		else if (command == CMD_LONG_WAIT) {
			uint64_t wait_time = CommandBits(command_word, 5, 48);
			buf += std::format(" wait_time = {:012X} FPGA cycles", wait_time);
			time += wait_time * 10 * 0.000001;
		}
		else if (command == CMD_INPUT_REPEATED_OUT_IN) {
			/*
			CMD_INPUT_REPEATED_OUT_IN : begin   //This is a two cycle operation. The last state has to be LOAD_EXTENDED_DATA, in order to avoid writing the flg given here to the channels. The opcode I2C_OUT is encountered in that state, and argument stored. Here we use this stored argument.
								INPUT_REPEAT_repeats <= command[27:8];
								INPUT_REPEAT_wait <= command[55:32]; //Use LOAD_EXTENDED_DATA before INPUT_REPEATED_OUT to copy 64-bit channel content to extended_data
								INPUT_REPEAT_trigger_secondary_interrupt_when_finished <= command[56:56];
								INPUT_REPEAT_state <= INPUT_REPEAT_START;
								INPUT_REPEAT_command <= command[59:57];
								if (bus_clock) bus_clock <= 0; else bus_clock <= 1;
								strobe_generator_state <= DELAY_CYCLE;
							end
			*/
			uint32_t repeats = static_cast<uint32_t>(CommandBits(command_word, 8, 20));
			uint32_t wait = static_cast<uint32_t>(CommandBits(command_word, 32, 24));
			uint32_t trigger_secondary_interrupt_when_finished = static_cast<uint32_t>(CommandBits(command_word, 56, 1));
			uint32_t command = static_cast<uint32_t>(CommandBits(command_word, 57, 3));
			/*
					if (INPUT_REPEAT_command[0] == 1) SPI_state <= SPI_START;
                    if (INPUT_REPEAT_command[1] == 1) DIG_IN_state <= DIG_IN_START;
                    if (INPUT_REPEAT_command[2] == 1) ANA_IN_state <= ANA_IN_START;
			*/

			uint32_t SPI_restart_wait_on_ready_active = (low_buffer >> 7) & 0x01;
			buf += std::format(" repeats = {} ; wait = {} * 10ns ; SPI_restart_wait_on_ready_active = {:01X} ; trigger_secondary_interrupt_when_finished = {:01X} ; command = {:01X} ({})", repeats, wait, SPI_restart_wait_on_ready_active, trigger_secondary_interrupt_when_finished, command, InputRepeatCommandName(command));
		}
		else if (command == CMD_SET_PERIODIC_TRIGGER_PERIOD) {
			/*
			CMD_SET_PERIODIC_TRIGGER_PERIOD: begin
				periodic_trigger_period <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
			end
			*/
			uint64_t periodic_trigger_period = CommandBits(command_word, 8, 48);
			buf += std::format(" periodic_trigger_period = {:012X} FPGA cycles", periodic_trigger_period);
		}
		else if (command == CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME) {
			/*
			CMD_SET_PERIODIC_TRIGGER_ALLOWED_WAIT_TIME: begin
				periodic_trigger_allowed_wait_cycles <= command[55:8]; // >>8 =  [47:0] = 48 bit;  55:32 = 23:0 = 24 bit
			end
			*/
			uint64_t periodic_trigger_allowed_wait_cycles = CommandBits(command_word, 8, 48);
			buf += std::format(" periodic_trigger_allowed_wait_cycles = {:012X} FPGA cycles", periodic_trigger_allowed_wait_cycles);
		}
		else if (command == CMD_SPI_OUT_IN) {
			/*
			CMD_SPI_OUT_IN : begin
								SPI_OUT_length <= command[14:8];
								SPI_IN_length <= command[20:16];
								SPI_SEL_next <= command[34:32];
								SPI_data <= register;  //Use CMD_LOAD_REG_LOW and CMD_LOAD_REG_HIGH before CMD_SPI_OUT_IN to copy 117-bit I2C data to register
								INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
								if (command[40:40] == 0) SPI_state <= SPI_START;
							end
			*/
			uint32_t SPI_OUT_length = (low_buffer >> 8) & 0x3F;
			uint32_t SPI_IN_length = (low_buffer >> 16) & 0x3F;
			uint32_t SPI_ready_active_level = (low_buffer >> 5) & 0x01;
			uint32_t wait_for_SPI_ready_edge_to_active = (low_buffer >> 6) & 0x01;
			uint32_t wait_for_SPI_ready_active = (low_buffer >> 7) & 0x01;
			uint32_t SPI_SEL_next = high_buffer & 0x03;
			uint32_t SPI_chip_select = (high_buffer >> 2) & 0x07;
			uint32_t dont_start_now = (high_buffer >> 8) & 0x01;
			buf += std::format(" SPI_OUT_length = {:02X} ; SPI_IN_length = {:02X} ; SPI_ready_active_level = {:01X} ; wait_for_SPI_ready_edge_to_active = {:01X} ; wait_for_SPI_ready_active = {:01X} ; SPI_SEL_next = {:01X} ; SPI_chip_select = {:01X} ; start_now = {:01X}", SPI_OUT_length, SPI_IN_length, SPI_ready_active_level, wait_for_SPI_ready_edge_to_active, wait_for_SPI_ready_active, SPI_SEL_next, SPI_chip_select, dont_start_now ? 0 : 1);
		}
		else if (command == CMD_ANALOG_IN_OUT) {
			/*
			CMD_ANALOG_IN_OUT: begin
								adc_register_address <= command[14:8];  //to read standard analog in, this should be 3, see Xilinx user guide UG480
								adc_write_enable <= command[15:15];
								adc_programming_out <= command[31:16];
								wait_time[29:0] <= command[63:34];
								wait_time[47:30] <= 0;
								INPUT_REPEAT_state <= INPUT_REPEAT_IDLE;
								if (command[32:32] == 0) begin  //if command[40:40] is high, the actual reading will be started trhough CMD_REPEAT
									if (command[33:33] == 0) ANA_IN_state <= ANA_IN_START; //conversion and register read
									else ANA_IN_state <= ANA_IN_TRIGGER_READ_WRITE; //only register read or write
								end
							end
			*/
			uint32_t adc_register_address = (low_buffer >> 8) & 0x7F;
			uint32_t adc_write_enable = (low_buffer >> 15) & 0x01;
			uint32_t adc_programming_out = (low_buffer >> 16) & 0xFFFF;
			uint32_t dont_start_now = static_cast<uint32_t>(CommandBits(command_word, 32, 1));
			uint32_t only_read_write = static_cast<uint32_t>(CommandBits(command_word, 33, 1));
			uint32_t wait_time = static_cast<uint32_t>(CommandBits(command_word, 34, 30));
			buf += std::format(" adc_register_address = {:02X} ; adc_write_enable = {:01X} ; adc_programming_out = {:04X} ; wait_time = {:08X} ; start_now = {:01X} ; only_read_write = {:01X}", adc_register_address, adc_write_enable, adc_programming_out, wait_time, dont_start_now ? 0 : 1, only_read_write);
		}
		else if (command == CMD_SET_STROBE_OPTIONS) {
			/*
			CMD_SET_STROBE_OPTIONS: begin
							strobe_choice <= command[11:8]; // 3 bit
							strobe_low_length <= command[23:16]; // 8 bit
							strobe_high_length <= command[31:24]; // 8 bit
						end
			*/
			uint32_t strobe_choice = (low_buffer >> 8) & 0x07;
			uint32_t strobe_low_length = (low_buffer >> 16) & 0xFF;
			uint32_t strobe_high_length = (low_buffer >> 24) & 0xFF;
			buf += std::format(" strobe_choice = {:01X} ; strobe_low_length = {:02X} ; strobe_high_length = {:02X}", strobe_choice, strobe_low_length, strobe_high_length);
		}
		else if (command == CMD_LOAD_REG_LOW) {
			/*
			CMD_LOAD_REG_LOW:begin
							register[58:0] <= command[63:5];
						end
			*/
			uint64_t register_data = CommandBits(command_word, 5, 59);
			buf += std::format(" register[58:0] = {:015X}", register_data);
		}
		else if (command == CMD_LOAD_REG_HIGH) {
			/*
			CMD_LOAD_REG_HIGH:begin
							register[117:59] <= command[63:5];
						end
			*/
			uint64_t register_data = CommandBits(command_word, 5, 59);
			buf += std::format(" register[117:59] = {:015X}", register_data);
		}
		else if (command == CMD_WAIT_FOR_TRIGGER) {
			/*
			CMD_WAIT_FOR_TRIGGER: begin
							if ((trigger_0 && (command[8:8] == 1)) || (trigger_1 && (command[9:9] == 1)) || (trigger_PS && (command[10:10] == 1))) address <= address + 1;
						end
			*/
			uint32_t trigger0 = (low_buffer >> 8) & 0x01;
			uint32_t trigger1 = (low_buffer >> 9) & 0x01;
			uint32_t trigger_PS = (low_buffer >> 10) & 0x01;
			buf += std::format(" trigger0 = {:01X} ; trigger1 = {:01X} ; trigger_PS = {:01X}", trigger0, trigger1, trigger_PS);
		}
		else if (command == CMD_SET_LOOP_COUNT) {
			/*
			CMD_SET_LOOP_COUNT: begin
							loop_count <= command[63:32];
						end
			*/
			buf += std::format(" loop_count = {:08X}", high_buffer);
		}
		else if (command == CMD_CONDITIONAL_JUMP_FORWARD) {
			/*
			CMD_CONDITIONAL_JUMP_FORWARD: begin  //here we assume that the program assembling the sequence has made sure that the jump is within the current BRAM half
							if ((condition_0 && (command[8:8] == 1)) || (condition_1 && (command[9:9] == 1)) || (condition_PS && (command[10:10] == 1)) || (command[11:11] == 1))
								address <= address + command[15:8];
							else address <= address + 1;
						end
			*/
			uint32_t dig_in_bit_nr = (low_buffer >> 5) & 0x07;
			uint32_t condition_dig_in = (low_buffer >> 8) & 0x01;
			uint32_t condition0 = (low_buffer >> 9) & 0x01;
			uint32_t condition1 = (low_buffer >> 10) & 0x01;
			uint32_t condition_PS = (low_buffer >> 11) & 0x01;
			uint32_t unconditional_jump = (low_buffer >> 12) & 0x01;
			uint32_t jump = high_buffer & 0x3FFF;
			buf += std::format(" dig_in_bit_nr = {:01X} ; condition_dig_in = {:01X} ; condition0 = {:01X} ; condition1 = {:01X} ; condition_PS = {:01X} ; unconditional_jump = {:01X} ; jump = {:04X}", dig_in_bit_nr, condition_dig_in, condition0, condition1, condition_PS, unconditional_jump, jump);
		}
		else if (command == CMD_CONDITIONAL_JUMP_BACKWARD) {
			uint32_t dig_in_bit_nr = (low_buffer >> 5) & 0x07;
			uint32_t condition_dig_in = (low_buffer >> 8) & 0x01;
			uint32_t condition0 = (low_buffer >> 9) & 0x01;
			uint32_t condition1 = (low_buffer >> 10) & 0x01;
			uint32_t condition_PS = (low_buffer >> 11) & 0x01;
			uint32_t unconditional_jump = (low_buffer >> 12) & 0x01;
			uint32_t loop_count_greater_zero = (low_buffer >> 13) & 0x01;
			uint32_t jump = high_buffer & 0x3FFF;
			uint32_t legacy_jump = (low_buffer >> 8) & 0xFF;
			buf += std::format(" dig_in_bit_nr = {:01X} ; condition_dig_in = {:01X} ; condition0 = {:01X} ; condition1 = {:01X} ; condition_PS = {:01X} ; unconditional_jump = {:01X} ; loop_count_greater_zero = {:01X} ; jump = {:04X} ; legacy_low_byte_jump = {:02X}", dig_in_bit_nr, condition_dig_in, condition0, condition1, condition_PS, unconditional_jump, loop_count_greater_zero, jump, legacy_jump);
		}
		else if (command == CMD_I2C_OUT) {
			uint32_t I2C_out_length = (low_buffer >> 8) & 0x7F;
			uint32_t I2C_in_length = (low_buffer >> 15) & 0x3F;
			uint32_t I2C_SELECT_NEXT = (low_buffer >> 20) & 0x03;
			buf += std::format(" I2C_out_length = {:02X} ; I2C_in_length = {:02X} ; I2C_SELECT_NEXT = {:01X} ; I2C_data = register[117:0]", I2C_out_length, I2C_in_length, I2C_SELECT_NEXT);
		}
		else if (command == CMD_SET_INPUT_BUF_MEM) {
			uint32_t write_address = (low_buffer >> 7) & 0x01;
			uint32_t input_buf_mem_address = (low_buffer >> 8) & 0x1FFF;
			buf += std::format(" input_buf_mem_data = {:08X} ; write_address = {:01X} ; input_buf_mem_address = {:04X}", high_buffer, write_address, input_buf_mem_address);
		}
		else if (command == CMD_WAIT_FOR_PERIODIC_TRIGGER) {
			buf += " wait until periodic_trigger_signal; set warning_missed_periodic_trigger from periodic_trigger_wait_cycles";
		}
		else if (command == CMD_WAIT_FOR_WAIT_CYCLE_NR) {
			uint64_t wait_cycle_nr = CommandBits(command_word, 8, 48);
			buf += std::format(" wait until wait_cycles == {:012X}", wait_cycle_nr);
		}
		else if (command == CMD_DIG_IN) {
			uint32_t user_data = (low_buffer >> 8) & 0xFFFFFF;
			buf += std::format(" write input memory: data[7:0] = core_dig_in ; data[31:8] = {:06X}", user_data);
		}
		else if (command == CMD_TRIGGER_SECONDARY_PL_PS_INTERRUPT) {
			buf += " secondary_PS_PL_interrupt <= 1";
		}
		else if (command == CMD_PL_TO_PS_COMMAND) {
			uint32_t PL_to_PS_command = (low_buffer >> 8) & 0xFFFF;
			buf += std::format(" PL_to_PS_command = {:04X}", PL_to_PS_command);
		}
		else if (command == CMD_SAVE_CYCLE_COUNT_SINCE_STARTUP_IN_INPUT_BUF_MEM) {
			buf += " write cycle_count_since_startup[55:0] to input memory as 64-bit value";
		}
		else if (command == CMD_CALC_AD9854_FREQUENCY_TUNING_WORD) {
			uint32_t bit_shift = (low_buffer >> 8) & 0x1F;
			uint64_t ftw0 = CommandBits(command_word, 16, 48);
			buf += std::format(" ftw0 = {:012X} ; bit_shift = {:02X}", ftw0, bit_shift);
		}
		else if (command == CMD_LOAD_EXTENDED_COMMAND) {
			uint32_t ext_command = (low_buffer >> 5) & 0x3F;
			buf += std::format(" {} ; ", ExtendedCommandName(static_cast<uint8_t>(ext_command)));
			if (ext_command == EXTENDED_CMD_LOAD_SPI_TIMING) {
				uint32_t SPI_delay_CS_low_start_wait = static_cast<uint32_t>(CommandBits(command_word, 11, 10));
				uint32_t SPI_delay_write = static_cast<uint32_t>(CommandBits(command_word, 21, 10));
				uint32_t SPI_delay_pause_before_read = static_cast<uint32_t>(CommandBits(command_word, 31, 12));
				uint32_t SPI_delay_read = static_cast<uint32_t>(CommandBits(command_word, 43, 10));
				uint32_t SPI_delay_CS_low_end_wait = static_cast<uint32_t>(CommandBits(command_word, 53, 10));
				buf += std::format("SPI_delay_CS_low_start_wait = {:03X} ; SPI_delay_write = {:03X} ; SPI_delay_pause_before_read = {:03X} ; SPI_delay_read = {:03X} ; SPI_delay_CS_low_end_wait = {:03X}", SPI_delay_CS_low_start_wait, SPI_delay_write, SPI_delay_pause_before_read, SPI_delay_read, SPI_delay_CS_low_end_wait);
			}
			else if (ext_command == EXTENDED_CMD_SET_SPI_MODE) {
				uint32_t SPI_CPHA = static_cast<uint32_t>(CommandBits(command_word, 11, 1));
				uint32_t SPI_CPOL = static_cast<uint32_t>(CommandBits(command_word, 12, 1));
				uint32_t SPI_mode = (SPI_CPOL << 1) | SPI_CPHA;
				buf += std::format("SPI_mode = {:01X} ; SPI_CPHA = {:01X} ; SPI_CPOL = {:01X}", SPI_mode, SPI_CPHA, SPI_CPOL);
			}
			else if (ext_command == EXTENDED_CMD_SET_I2C_PARAMETERS) {
				uint32_t I2C_0_Destination = static_cast<uint32_t>(CommandBits(command_word, 16, 1));
				uint32_t I2C_delay_start_stop = static_cast<uint32_t>(CommandBits(command_word, 17, 8));
				uint32_t I2C_delay_data_setup = static_cast<uint32_t>(CommandBits(command_word, 25, 8));
				uint32_t I2C_delay_clock_high = static_cast<uint32_t>(CommandBits(command_word, 33, 8));
				uint32_t I2C_delay_clock_low = static_cast<uint32_t>(CommandBits(command_word, 41, 8));
				uint32_t I2C_delay_pause_before_read = static_cast<uint32_t>(CommandBits(command_word, 49, 8));
				buf += std::format("I2C_0_Destination = {:01X} ; I2C_delay_start_stop = {:02X} ; I2C_delay_data_setup = {:02X} ; I2C_delay_clock_high = {:02X} ; I2C_delay_clock_low = {:02X} ; I2C_delay_pause_before_read = {:02X}", I2C_0_Destination, I2C_delay_start_stop, I2C_delay_data_setup, I2C_delay_clock_high, I2C_delay_clock_low, I2C_delay_pause_before_read);
			}
		}
		out << buf << endl;
	}
	out.close();
}

void CEthernetControllerFirefly::SetPeriodicTrigger_ms(double aPeriodicTriggerPeriod_in_ms, double aPeriodicTriggerAllowedWaitTime_in_ms) {
	WaitForPeriodicTrigger(aPeriodicTriggerPeriod_in_ms > 0);
	PeriodicTriggerPeriod_in_ms = aPeriodicTriggerPeriod_in_ms;
	PeriodicTriggerAllowedWait_in_ms = aPeriodicTriggerAllowedWaitTime_in_ms;
	SetPeriodicTriggerAtBeginningOfNextSequence = true;
	ChangePeriodicTriggerPeriodWhileCycling = WaitForPeriodicTriggerAtBeginningOfSequence;
}

void CEthernetControllerFirefly::WaitForPeriodicTrigger(bool aWaitForPeriodicTriggerAtBeginningOfSequence) {
	WaitForPeriodicTriggerAtBeginningOfSequence = aWaitForPeriodicTriggerAtBeginningOfSequence;
}

void CEthernetControllerFirefly::TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool aDoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
	DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible = aDoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible;
	if (!DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
		previous_command_buffer = NULL;
		previous_command_buffer_length = 0;
	}
}

bool CEthernetControllerFirefly::ModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptModifySequence(differences, difference_index_table, difference_command_table))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::OptimizedCommand(CString CommandString) {
	if (MinimizeEthernetCommunicationDirectionChanges)
		return Command(CommandString + "0", /*DontWaitForReady*/ true);
	else
		return Command(CommandString);
}

bool CEthernetControllerFirefly::AttemptModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]) {
	if (!/*Optimized*/Command("modify_sequence")) return false;
	if (!WriteInteger(12 * differences)) return false;
	if (!SendData((uint8_t*)difference_index_table, 4 * differences)) return false;
	if (!SendData((uint8_t*)difference_command_table, 8 * differences,/*SendReady*/ false)) return false;
	return true;
}


bool CEthernetControllerFirefly::SendSequence(uint32_t DataSize, uint32_t* buffer) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptSendSequence(DataSize, buffer))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptSendSequence(uint32_t DataSize, uint32_t* buffer) {
	if (!/*Optimized*/Command("send_sequence")) return false;
	if (!WriteInteger(DataSize)) return false;
	if (!SendData((uint8_t*)buffer, DataSize)) return false;
	return true;
}



bool CEthernetControllerFirefly::TransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptTransmitI2CPort(I2C_port, I2C_destination, I2C_address, send_length, send_data, receive_length, receive_data, I2C_clock_frequency_in_Hz, I2C_success, fail_silently))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptTransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t &receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool &I2C_success, bool fail_silently) {
	I2C_success = false;
	if (!/*Optimized*/Command("transmit_I2C")) return false;
	if (!WriteInteger(I2C_port)) return false;
	if (!WriteInteger(I2C_destination)) return false;
	if (!WriteInteger(I2C_address >> 1)) return false;
	if (!WriteInteger(I2C_clock_frequency_in_Hz)) return false;
	if (!WriteInteger(send_length)) return false;
	if (!WriteInteger(receive_length)) return false;
	const uint16_t receive_capacity = receive_length;
	if (send_length>0) {
		if (!SendData(send_data, send_length)) return false;
	}
	int I2CTransmissionOk;
	if (!ReadInt(I2CTransmissionOk)) return false;
	if (I2CTransmissionOk != 1) {
		if (!fail_silently) AddErrorMessage("CEthernetControllerFirefly::AttemptTransmitI2CPort : I2C transmission failed\nIs I2C device connected?\n\nFor more debug info, put FPGA sequencer into debug mode using CLA_SwitchDebugMode(true,\"Filename\")\n(or comment out CLA_SwitchDebugMode(false,\"Filename\"), if that blocks I2C debugging),\nconnect USB cable to it's UART USB output, and look at data send back on COM terminal.\nSwitch debug mode off to work at highest possible speed.");
		return true;
	}
	I2C_success = true;
	unsigned long long InputBufferContentsLength;
	if (!ReadInt64(InputBufferContentsLength)) return false;
	if (InputBufferContentsLength > 0) {


		/*
		//pedestrian version
		if (InputBufferContentsLength > receive_length) {
			AddErrorMessage("CEthernetControllerFirefly::AttemptTransmitI2CPort : input data too large");
			return true;
		}
		receive_length = InputBufferContentsLength;
		if (receive_data) delete[] receive_data;
		receive_data = new uint8_t[receive_length];
		previous_receive_data_ptr = receive_data;
		receive_data_length = receive_length;
		if (Network) {
			return Network->ReceiveData(receive_data, receive_length, 5000);
		}
		else {
			delete[] receive_data;
			receive_data = NULL;
			previous_receive_data_ptr = NULL;
			receive_data_length = 0;
			return false;
		}
		*/


		if (!Network) return false;

		if (InputBufferContentsLength > (numeric_limits<unsigned long>::max)()) return true;

		const unsigned long bytes_to_receive = static_cast<unsigned long>(InputBufferContentsLength);
		receive_length = static_cast<uint16_t>((std::min<unsigned long long>)(InputBufferContentsLength, (numeric_limits<uint16_t>::max)()));

		if (receive_data && InputBufferContentsLength <= receive_capacity) {
			return Network->ReceiveData(receive_data, bytes_to_receive, 5000);
		}

		// Drain the socket without writing past a caller-owned receive buffer.
		vector<uint8_t> overflow_buffer(bytes_to_receive);
		if (!Network->ReceiveData(overflow_buffer.data(), bytes_to_receive,  5000)) return false;
		if (receive_data && receive_capacity > 0) {
			memcpy(receive_data, overflow_buffer.data(), receive_capacity);
		}
		return true;

	}
	return true;
}


bool CEthernetControllerFirefly::SetPSOptions(uint8_t options) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptSetPSOptions(options))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptSetPSOptions(uint8_t options) {
	if (!/*Optimized*/Command("set_PS_options")) return false;
	if (!WriteInteger(options)) return false;
	return true;
}


const unsigned int BusBitShift = 8;
const unsigned int BusSequencerSpecialCommand = 0x7 << BusBitShift;

//bool CEthernetControllerFirefly::AddData(uint32_t* BusData, uint32_t* Spacing,/* uint32_t* AbsoluteTime,*/ unsigned long Count) {
bool CEthernetControllerFirefly::AddSequencePreamble() {
	constexpr bool IgnoreSpecialFPGACommand = true;
	//delete AbsoluteTime;
	//AbsoluteTime = NULL;
	if (!Connected) return false;

	if (DebugBufferFile) {
		(*DebugBufferFile) << endl;
	}
	////Timestamp.Mark("AddData");
	uint32_t PreambleProgramLines = 11;  //make sure you put the number of program lines in the preamble here
	uint32_t PostProgramLines = 11;  //make sure you put the number of program lines in the preamble here
	//uint32_t DataSize = (PreambleProgramLines + PostProgramLines + Count) * 8; //data size in byte

	//ControlMessageBox(std::format("CNetwork::ReceiveMsg :: sending  {} bytes", DataSize));

	//uint32_t* buffer = new uint32_t[(PreambleProgramLines + PostProgramLines + Count + MaxSequencerCommandListSize) * 4];

	//Add preamble for external clock and trigger setup

	uint32_t DelayMultiplier = FPGAClockToBusClockRatio;// floor(FPGAClockFrequencyInHz / BusFrequency - 1);
	if (DelayMultiplier < 2) DelayMultiplier = 2;

	unsigned int StrobeDuration = ((DelayMultiplier) / 3);
	AddCommandWriteSystemTimeToInputMemory(); //write time stamp to first command line
	//strobe/clock output pin content: 0: clock 1: strobe, 2: low, 3: high, 4: flags_hi[31]
	SetStrobeOptions( (FPGAUseStrobeGenerator) ? 1 : 0, StrobeDuration, StrobeDuration); // this command fills 2 command lines
	SetTriggerOptions(  ExternalTrigger0, ExternalTrigger1); // this command fills 6 command lines

	//end of preamble
	return true;

}

bool CEthernetControllerFirefly::SendSequenceToFPGA(uint32_t* buffer, const std::string& DebugFileName) {
	//uint32_t FPGA_Special_Command = BusSequencerSpecialCommand << (2 + 16);
	//uint32_t FPGA_Special_Command_Mask = 0xF << (BusBitShift + 2 + 16);
	//uint32_t AdditionalSteps = 0;

	//for (uint32_t n = 0; n < Count; n++) {
		/*if (((BusData[n] & FPGA_Special_Command_Mask) == FPGA_Special_Command) && (!IgnoreSpecialFPGACommand)) {
			//ToDo: add devices that can add special commands, e.g. analog input, I2C etc.
			unsigned __int16 bus_data_without_address = BusData[n] & 0xFFFF;
			if (bus_data_without_address < SequencerCommandListSize) {
				AddSequencerCommandToBuffer(buffer, n + AdditionalSteps + PreambleProgramLines, SequencerCommandList[bus_data_without_address * 2 + 1], SequencerCommandList[bus_data_without_address * 2]);
				if (Spacing[n] > 0) {
					//This code should be necessary for timing to be correct. However when it's added input doesn't work anymore. Also NI status bar doesn't work and WaitForSequenceEnd sent too early.
					AdditionalSteps++;
					AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, 0, Spacing[n] * DelayMultiplier - 1);
				}
			}
			else {
				ControlMessageBox("EthernetMultiIOControllerFirefly.cpp: AddData(): special command not found in special command list.");
				AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, 0, DelayMultiplier);
			}
		}
		else {*/
		//	if (Spacing[n] < 1) Spacing[n] = 1; //2024 08 18: current minimum spacing is 1 clock cycle, to give BRAM time to deliver data; would need pipeline in Vivado core.sv to reduce this
		//	AddProgramLine(buffer, n + AdditionalSteps + PreambleProgramLines, CMD_STEP, BusData[n], Spacing[n] * DelayMultiplier);
		//}
	//}
	//weird workaround to make program stop correctly; perhaps the last wait time in the buffer is not calculated correctly
	//AddProgramLine(buffer, PreambleProgramLines + AdditionalSteps + Count - 1, CMD_STEP, 0, DelayMultiplier);
	//for (uint32_t n = (PreambleProgramLines + AdditionalSteps + Count); n < (PreambleProgramLines + AdditionalSteps + PostProgramLines - 1 + Count); n++) { //add a few code lines to make sure program is not too short for DMA transfer to start and stop correctly; might be better solved by improving MicroZed code

	for (uint32_t n=0;n< 10;n++) {
		AddProgramLine(CMD_STOP, 0, 0);
		//AddProgramLine(buffer, n, CMD_STEP, 0, DelayMultiplier);
	}
	//AddProgramLine(buffer, Count + PreambleProgramLines + PostProgramLines - 2, CMD_STOP, 0, 0);
	//AddProgramLine(buffer, Count + AdditionalSteps + PreambleProgramLines + PostProgramLines - 1, CMD_STOP, 0, 0);

	uint32_t FilledBufferLength;
	uint32_t MaxBufferLength;

	MySequencer->GetBufferLength(FilledBufferLength, MaxBufferLength);


	uint32_t Count = FilledBufferLength;
	uint32_t DataSize = 8 * Count;
	if (!DebugFileName.empty()) WriteBufferToFile(buffer, DataSize / 8, DebugFileName);

	if (!(DataSize < MaxFPGAProgramLength * 8)) {
		ControlMessageBox("CEthernetControllerFirefly::AddData : Program longer than FPGA SOM memory allows");
		//delete buffer;
		return false;
	}

	if (DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible) {
		bool possible = (previous_command_buffer != NULL) && (previous_command_buffer_length == DataSize);
		if (possible) {
			constexpr unsigned long max_differences = 1000; //maximum number of differences to check; if more differences are found, the program is sent as is
			unsigned long differences = 0;
			uint32_t difference_index_table[max_differences];
			uint32_t difference_command_table[2 * max_differences];
			for (uint32_t n = 0; n < DataSize / 8; n++) {
				if ((buffer[2*n] != previous_command_buffer[2*n]) || (buffer[2*n+1] != previous_command_buffer[2*n+1])) {
					if (differences < max_differences) {
						difference_index_table[differences] = n;
						difference_command_table[2 * differences] = buffer[2 * n ];
						difference_command_table[2 * differences + 1] = buffer[2 * n + 1];
						//previous_command_buffer[2 * n] = buffer[2 * n];
						//previous_command_buffer[2 * n + 1] = buffer[2 * n + 1];
						differences++;
					}
					else {
						possible = false;
						//delete previous_command_buffer; //don't delete previous_command_buffer, as it is just a copy of buffer.
						previous_command_buffer = NULL;
						previous_command_buffer_length = 0;
						break;
					}
				}
			}
			if (possible) {
				//delete buffer;
				//arrange data as two consecutive tables
				//the first table is a list of 32-bit values, containing the list index of the 64 bit command that needs to be modified
				//the second table contains a list of the new 64-bit commands.
				/*for (uint32_t n = 0; n < differences; n++) {
					difference_index_table[differences + 2 * n] = difference_command_table[2 * n];
					difference_index_table[differences + 2 * n + 1] = difference_command_table[2 * n + 1];
				}*/
				if (differences > 0) {
					ModifySequence(differences, difference_index_table, difference_command_table);
				}
				//Command("print_sequence");//only for debug
				previous_command_buffer = buffer;
				previous_command_buffer_length = DataSize;
				ClearSequencerCommandList();
				return true;
			}
		}
	}
	////Timestamp.Mark("send_sequence");
	bool ok = SendSequence(DataSize, buffer);
	if (ok) {
		std::string buf = std::format("send data, {} bytes sent", DataSize);
		////Timestamp.Mark(buf);
		//if (previous_command_buffer) delete previous_command_buffer; //don't delete previous_command_buffer, as it is just a copy of buffer.
		previous_command_buffer = buffer;
		previous_command_buffer_length = DataSize;
		ClearSequencerCommandList();
		//Command("print_sequence");//only for debug
		////Timestamp.Mark("end AddData");
		return true;
	}
	ClearSequencerCommandList();
	return false;
}

double CEthernetControllerFirefly::MeasureEthernetBandwidth(uint32_t DataSize, double MinimumExpected) {
	SwitchDebugMode(false, "");
	bool ok = Command("send_sequence");
	if (ok) {

		WriteInteger(DataSize);
		uint32_t* buffer = new uint32_t[DataSize];
		for (uint32_t n = 0; n < DataSize; n++) {
			buffer[n] = 0;
		}
		Time StartTickCount = Clock::now();
		SendData((uint8_t*)buffer, DataSize);
		CheckReady(10);
		Time EndTickCount = Clock::now();
		delete[] buffer;
		double Bandwidth = 0.000001*DataSize * 8 / (milliSeconds(EndTickCount - StartTickCount) / 1000.0); //in Mbit/s
		if (Bandwidth < MinimumExpected) {
			AddErrorMessage(std::format("CEthernetControllerFirefly::MeasureEthernetBandwidth : Ethernet Bandwidth = {:.1f} MBit/s is lower than expected. Check ethernet connection.", Bandwidth));
		}
		return Bandwidth;
	}
	if (MinimumExpected > 0) AddErrorMessage("CEthernetControllerFirefly::MeasureEthernetBandwidth : No ethernet connection to FPGA.");
	return -1;
}

bool CEthernetControllerFirefly::GetAktWaveformPoint(unsigned long long& DataPointsWritten, bool& running) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetAktWaveformPoint(DataPointsWritten, running))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetAktWaveformPoint(unsigned long long& DataPointsWritten, bool &running) {
	if (!Connected) return false;
	//progress bar without communication
	//unsigned  long TickCounts = GetTickCount();
	//DataPointsWritten = (TickCounts - StartTickCounts)*(BusFrequency/1000);
	//return true;
	//////Timestamp.Mark("CEthernetControllerFirefly::GetAktWaveformPoint");
	//progress bar with communication
	if (!OptimizedCommand("get_current_waveform_point")) return false;
	unsigned long long AktWaveformEntry;
	if (!ReadInt64(AktWaveformEntry)) return false; //100MHz clock cycles
	if (!ReadBool(running)) return false;
	DataPointsWritten = AktWaveformEntry / FPGAClockToBusClockRatio;// (FPGAClockFrequencyInHz / BusFrequency); //conversion to 2MHz clock cycles
	return true;
}

bool CEthernetControllerFirefly::AttemptNetworkCommand(tBoolFunction fCommand) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!fCommand())) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::GetNextCycleNumber(long& NextCycleNumber) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetNextCycleNumber(NextCycleNumber))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetNextCycleNumber(long& NextCycleNumber) {
	if (!Connected) return false;
	if (!/*Optimized*/Command("get_sequence_number")) return false;
	return ReadLong(NextCycleNumber);
}

bool CEthernetControllerFirefly::SwitchDebugMode(bool OnOff, const std::string& aFilename) {
	(void)aFilename;
	if (OnOff) return AttemptNetworkCommand([this]() {return Command("switch_debug_mode_on"); });
	else return AttemptNetworkCommand([this]() {return Command("switch_debug_mode_off"); });
}

bool CEthernetControllerFirefly::ResetCycleNumber() {
	return AttemptNetworkCommand([this]() {return Command("reset_sequence_number"); });
}

bool CEthernetControllerFirefly::CloseConnection() {
	Connected = false;
	return AttemptNetworkCommand([this]() {return Command("close"); });
}

bool CEthernetControllerFirefly::Reset() {
	//this commands resets the whole FPGA core. It should only be used if an error occured
	return AttemptNetworkCommand([this]() {return Command("reset"); });
}

bool CEthernetControllerFirefly::Start() {
	if (!Connected) return false;
	StartTickCounts = Clock::now();
	////Timestamp.Mark("CEthernetControllerFirefly::Start");
	return AttemptNetworkCommand([this]() {return Command("start"); });
}

bool CEthernetControllerFirefly::Stop() {
	return true;
	//if (!Connected) return false;
	//return AttemptNetworkCommand([this]() {return Command("stop"); });
}


bool CEthernetControllerFirefly::SetFrequency(double Frequency) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptSetFrequency(Frequency))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptSetFrequency(double Frequency) {
	if (!Command("set_frequency")) return false;
	return WriteDouble(Frequency);
}



bool CEthernetControllerFirefly::StartAssemblingCPUCommandSequence() {
	if (!Command("cs_assemble_sequence")) return false;
	return true;
}

bool CEthernetControllerFirefly::AddCPUCommand(const char* command) {
	if (!Command("cs_add_command")) return false;
	return WriteString(command);
}

bool CEthernetControllerFirefly::ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms) {
	if (!Command("cs_execute_sequence")) return false;
	WriteInteger(ethernet_check_period_in_ms);
	return true;
}

bool CEthernetControllerFirefly::StopCPUCommandSequence() {
	if (!Command("cs_stop_sequence")) return false;
	return true;
}

bool CEthernetControllerFirefly::InterruptCPUCommandSequence() {
	if (!Command("cs_interrupt_sequence")) return false;


	return true;
}

bool CEthernetControllerFirefly::GetCPUCommandErrorMessages() {
	if (!Command("cs_get_error_message")) return false;
	long NrErrorMessages;
	bool ok = ReadLong(NrErrorMessages);
	if (!ok) {
		AddErrorMessage("GetCPUCommandErrorMessages : couldn't get number of error messages");
		return false;
	}
	if (NrErrorMessages > 0) {
		std::string allErrorMessages = "";
		for (long n = 0; n < NrErrorMessages; n++) {
			CString ErrorMessage;
			if (!GetCommand(ErrorMessage)) return false;
			allErrorMessages += CStringToStdString(ErrorMessage) + "\n";
		}
		AddErrorMessage(allErrorMessages);
	}
	return true;
}

bool CEthernetControllerFirefly::PrintCPUCommandErrorMessages() {
	if (!Command("cs_print_error_messages")) return false;
	return true;

}

bool CEthernetControllerFirefly::PrintCPUCommandSequence() {
	if (!Command("cs_print_command_table")) return false;
	return true;
}






void CEthernetControllerFirefly::SetExternalTrigger(bool aExternalTrigger0, bool aExternalTrigger1) {
	ExternalTrigger0 = aExternalTrigger0;
	ExternalTrigger1 = aExternalTrigger1;
}

bool CEthernetControllerFirefly::SetExternalClock(bool aExternalClock0, bool aExternalClock1) {
	bool changed = (ExternalClock0 != aExternalClock0) || (ExternalClock1 != aExternalClock1);
	ExternalClock0 = aExternalClock0;
	ExternalClock1 = aExternalClock1;
	if (!Connected) return false;
	if (changed) {
		if (ExternalClock0) {
			return AttemptNetworkCommand([this]() {return Command("select_external_clock_0"); });
		} else if (ExternalClock1) {
			return AttemptNetworkCommand([this]() {return Command("select_external_clock_1"); });
		} else {
			return AttemptNetworkCommand([this]() {return Command("select_internal_clock"); });
		}
	}
	return true;
}



bool CEthernetControllerFirefly::GetFrequency(double &Frequency) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetFrequency(Frequency))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetFrequency(double& Frequency) {
	if (!Command("get_frequency")) return false;
	return ReadDouble(Frequency);
}



bool CEthernetControllerFirefly::GetPeriodicTriggerError(bool& Error) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptGetPeriodicTriggerError(Error))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptGetPeriodicTriggerError(bool& Error) {
	if (!/*Optimized*/Command("get_periodic_trigger_error")) return false;
	int Errori;
	bool err = ReadInt(Errori);
	Error = Errori != 0;
	return err;
}


bool CEthernetControllerFirefly::CheckReady(double timeout_in_seconds) {
	return AttemptNetworkCommand([this]() {return Command("check_ready"); }); //Do not use OptimizedCommand here. The whole point is to get a "Ready" back.
}


bool CEthernetControllerFirefly::WaitTillEndOfSequence(double timeout_in_s) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptWaitTillEndOfSequence(timeout_in_s))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}



bool CEthernetControllerFirefly::AttemptWaitTillEndOfSequence(double timeout_in_s) {
	if (!Command("wait_till_finished")) return false;
	WriteDouble(timeout_in_s);//sequence timeout, <0.001 means no timeout
	int Success;
	if (!ReadInt(Success, timeout_in_s + 5)) return false;
	if (Success == 0) AddErrorMessage("CEthernetControllerFirefly::AttemptWaitTillEndOfSequence : timeout : sequence execution took too long.");
	return true;
}



bool CEthernetControllerFirefly::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
	if (!Connected) return false;
	unsigned int attempts = 0;
	while ((attempts < MaxReconnectAttempts) && (!AttemptWaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s))) {
		Network->ResetConnection();
		this_thread::sleep_for(100ms);
		attempts++;
	}
	return (attempts < MaxReconnectAttempts);
}

bool CEthernetControllerFirefly::AttemptWaitTillEndOfSequenceThenGetInputData(uint8_t * &buffer, unsigned long & buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
	if (previous_input_buffer_ptr) {
		delete[] previous_input_buffer_ptr;
		previous_input_buffer_ptr = NULL;
	}
	buffer_length = 0;
	//progress bar without communication
	//unsigned  long TickCounts = GetTickCount();
	//DataPointsWritten = (TickCounts - StartTickCounts)*(BusFrequency/1000);
	//return true;
	//progress bar with communication
	if (!Command("wait_till_end_of_sequence_then_get_input_data")) return false;

	WriteDouble(timeout_in_s);//sequence timeout, <0.001 means no timeout
	WriteDouble(timeout_in_s);//read input data timeout, <0.001 means no timeout

	int Success;
	if (!ReadInt(Success, timeout_in_s + 5)) return false;
	if (Success == 0) AddErrorMessage("CEthernetControllerFirefly::AttemptWaitTillEndOfSequenceThenGetInputData : timeout : sequence execution took too long.");

	int PeriodicTriggerError;
	if (!ReadInt(PeriodicTriggerError, 1)) return false;
#ifdef WIN32
	EndTimeOfCycle = GetTickCount();
#else
	/// @todo portable equivalent of GetTickCount() ??
#endif

	if (!ReadInt(Success, timeout_in_s + 5)) return false;
	if (Success == 0) AddErrorMessage("CEthernetControllerFirefly::AttemptWaitTillEndOfSequenceThenGetInputData : timeout : reading input data took too long.");

	//int InputBufferContentOriginSequence;
	//ReadInt(InputBufferContentOriginSequence);
	//int InputBufferContentsLength;
	//ReadInt(InputBufferContentsLength);
	unsigned long long InputBufferContentOriginSequence;
	if (!ReadInt64(InputBufferContentOriginSequence)) return false;
	unsigned long long InputBufferContentsLength;
	if (!ReadInt64(InputBufferContentsLength)) return false;
	if (InputBufferContentsLength > 0) {
		if (InputBufferContentsLength > 1024*1024*12024) {
			AddErrorMessage("CEthernetControllerFirefly::AttemptWaitTillEndOfSequenceThenGetInputData : input data too large");
			return true;
		}

		buffer = new uint8_t[InputBufferContentsLength];
		previous_input_buffer_ptr = buffer;
		buffer_length = InputBufferContentsLength;
		if (Network) return Network->ReceiveData(buffer, buffer_length, /*timeout_in_ms = */ 5000);
		else {
			delete[] buffer;
			buffer = NULL;
			previous_input_buffer_ptr = NULL;
			buffer_length = 0;
			return false;
		}
	}
	return true;
}
