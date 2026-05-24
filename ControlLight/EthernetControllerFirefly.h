#pragma once

#include "NetworkClient.h"	// Added by ClassView
#include "EthernetControllerFirefly.h"
#include <functional>
#include "std.h"


#pragma once

typedef std::function<bool(void)> tBoolFunction;

class CDeviceSequencer;

class CEthernetControllerFirefly : public CNetworkClient
{
public:
	unsigned long SequencerCommandListSize;
	uint32_t* SequencerCommandList;
	unsigned int MyMultiIO;
public:
	bool Connected;
	bool DebugBufferOn;
	std::ofstream* DebugBufferFile;
	bool ExternalTrigger0;
	bool ExternalTrigger1;
	double PeriodicTriggerPeriod_in_ms;
	double PeriodicTriggerAllowedWait_in_ms;
	bool ExternalClock0;
	bool ExternalClock1;
	//double BusFrequency;
	unsigned int FPGAClockToBusClockRatio;
	double FPGAClockFrequencyInHz;
	bool FPGAUseExternalClock;
	bool FPGAUseStrobeGenerator;
	Time StartTickCounts;
	//uint32_t* FPGABuffer;
	//uint32_t* FPGAAbsoluteTime;
	//uint32_t FPGABufferUsed;
	//COutput* myOutput;
	double LastPeriodicTriggerPeriod_in_ms;
private:
	bool core_option_LED; 
	uint8_t core_option_SPI_CS;
	uint8_t core_option_dig_out;  
	uint8_t core_option_PL_to_PS;
	bool SetPeriodicTriggerAtBeginningOfNextSequence;
	bool WaitForPeriodicTriggerAtBeginningOfSequence;
	bool ChangePeriodicTriggerPeriodWhileCycling;
	uint32_t* previous_command_buffer;
	unsigned long previous_command_buffer_length;
	CDeviceSequencer* MySequencer;
	uint8_t* previous_command_buffer_ptr;

	uint8_t* previous_receive_data_ptr;
	unsigned long receive_data_length;

	uint8_t* previous_input_buffer_ptr;

private:
	void StartSPIAnalogInAcquisition(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);	
	void StartXADCAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	void StartSPIAnalogInAcquisition(unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	void StartSPIAnalogInAcquisition_MCP3208(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	void StartSPIAnalogInAcquisition_ADS1256(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	void AddCommandAnalogInOut(uint8_t adc_register_address, uint8_t adc_write_enable, uint16_t adc_programming_out, uint8_t dont_execute_now, uint8_t only_read_write, uint32_t wait_time);
	void AddCommandSetCoreOptions();
	void AddCommandWait(unsigned long Wait_in_FPGA_clock_cycles);
	void AddCommandSetPLToPSCommand(unsigned int PLToPSCommand);
	bool DoTransmitOnlyDifferenceBetweenCommandSequenceIfPossible;
	bool AttemptNetworkCommand(tBoolFunction fCommand);
public:
	void AddCommandStop();
	void AddCommandSetCoreOption_LED(bool a_core_option_LED);
	void AddCommandSwitchBuzzer(bool OnOff);
	void AddCommandSetCoreOption_SPI_CS(uint8_t a_core_option_SPI_CS);
	void AddCommandSetCoreOption_dig_out(uint8_t a_core_option_dig_out);
	void AddCommandSetCoreOption_PL_to_PS(uint8_t a_core_option_PL_to_PS);
public:	
	CEthernetControllerFirefly(CDeviceSequencer* _MySequencer);
	virtual ~CEthernetControllerFirefly();
	bool SendSequenceToFPGA(uint32_t* buffer, const std::string& DebugFileName = "");
	void AddSequencerCommandToSequenceList(uint32_t high_buffer, uint32_t low_buffer, const uint8_t duration_in_FPGA_clock_cycles = 2);
	void StartAnalogInAcquisition(unsigned char analog_in_type, unsigned char SPI_CS, unsigned int channel_nr, unsigned int number_of_datapoints, double delay_between_datapoints_in_ms);
	bool AddSequencePreamble();
	//bool AddData(uint32_t* BusData, uint32_t* Spacing, /*uint32_t* AbsoluteTime,*/ unsigned long Count);
	bool GetAktWaveformPoint(unsigned long long& DataPointsWritten, bool &running);
	bool GetNextCycleNumber(long& NextCycleNumber);
	bool ResetCycleNumber();
	bool CheckReady(double timeout_in_s = 1);
	bool Reset();
	bool WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s = 10);
	void AddCommandStep(uint32_t data, uint32_t delay);
	void AddCommandStepSPI(uint32_t data, uint32_t delay, bool bus_strobe_first_part, bool bus_strobe_second_part, bool bus_strobe_idle_part, bool bus_data15_second_part, bool bus_data15_idle_part);
	void AddProgramLine( uint8_t command, uint32_t data, uint32_t delay);
	void AddDelay_in_ns(uint32_t delay_in_nanoseconds);
	void SetStrobeOptions( uint8_t strobe_choice, uint8_t strobe_low_length, uint8_t strobe_high_length);
	void SetTriggerOptions( bool ExternalTrigger0, bool ExternalTrigger1);
	void AddExternalTrigger( bool ExternalTrigger0, bool ExternalTrigger1, bool FPGASoftwareTrigger );
	void WriteBufferToFile(uint32_t* buffer, unsigned long length, const std::string& FileName);
	bool ConnectSocket(const std::string& host, unsigned int port, unsigned int aFPGAClockToBusClockRatio, double aFPGAClockFrequencyInHz, bool aFPGAUseExternalClock, bool aFPGAUseStrobeGenerator, bool ExternalTrigger);
	double GetFPGAClockFrequency_in_Hz();
	bool WaitTillEndOfSequence(double timeout_in_s = 0);//timeout_in_s <0.001 means no timeout
	bool Start();
	bool Stop();
	bool CloseConnection();
	//void ResetProgramBuffer() { FPGABufferUsed = 0; }
	bool SetFrequency(double Frequency);
	bool GetFrequency(double& Frequency);
	bool GetPeriodicTriggerError(bool& Error);
	void SetExternalTrigger(bool aExternalTrigger0, bool aExternalTrigger1);
	void SetPeriodicTrigger_ms(double aPeriodicTriggerPeriod_in_ms, double aPeriodicTriggerAllowedWaitTime_in_ms);
	void WaitForPeriodicTrigger(bool aWaitForPeriodicTriggerAtBeginningOfSequence);
	bool SetExternalClock(bool ExternalClock0, bool ExternalClock1);
	
	bool TransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently);
	bool SetPSOptions(uint8_t options);

	void DebugBuffer(const std::string& filename);
	void AddSequencerCommandToBuffer(uint32_t* buffer, uint32_t n, uint32_t high_buffer, uint32_t low_buffer);
	void ClearSequencerCommandList();
	void AddSequencerCommand(uint32_t high_word, uint32_t low_word, const uint8_t duration_in_FPGA_clock_cycles = 2);
	bool SwitchDebugMode(bool OnOff, const std::string& aFilename);
	void SwitchDebugLED(bool OnOff);
	void IgnoreTCPIP(bool OnOff);
	void AddMarker(uint8_t Marker);

	void AddCommandTransmitI2C(uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t *data_out);
	void AddCommandTransmitSPI(const uint8_t chip_select, const uint16_t number_of_bits_out, const uint8_t data_out[], uint8_t number_of_bits_in, const bool start_now, const bool wait_for_SPI_ready_active = false, const bool wait_for_SPI_ready_edge_to_active = false, const bool SPI_ready_active_level = false, const bool LSB_first = false);
	void AddCommandRepeatedOutIn(const uint16_t number_of_datapoints, const double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand, const bool SPI_restart_wait_on_ready_low = false);
	void AddCommandWriteSystemTimeToInputMemory();
	void AddCommandCalcAD9854FrequencyTuningWord(uint64_t ftw0, uint8_t bit_shift);
	void AddCommandSetSPITiming(uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait);
	void AddCommandSetSPIMode(uint8_t SPI_mode);
	void AddCommandSetI2CParameters(uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop = 60, uint8_t I2C_delay_data_setup = 40, uint8_t I2C_delay_clock_high = 60, uint8_t I2C_delay_clock_low = 150, uint8_t I2C_delay_pause_before_read = 0);
	void AddCommandWriteInputBuffer(unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
	void AddCommandSetLoopCount(unsigned int loop_count);
	void AddCommandJumpBackward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0, bool loop_count_greater_zero = false);
	void AddCommandJumpForward(unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0);
	
	void TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool OnOff);
	double MeasureEthernetBandwidth(uint32_t DataSize = 1024 * 1024, double MinimumExpected = -1);
	bool OptimizedCommand(CString CommandString);
	bool AttemptGetAktWaveformPoint(unsigned long long& DataPointsWritten, bool& running);

	
	bool StartAssemblingCPUCommandSequence();
	bool AddCPUCommand(const char* command);
	bool ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms);
	bool StopCPUCommandSequence();
	bool InterruptCPUCommandSequence();
	bool GetCPUCommandErrorMessages();
	bool PrintCPUCommandErrorMessages();
	bool PrintCPUCommandSequence();


private:
	bool ModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]);
	bool AttemptModifySequence(unsigned long differences, uint32_t difference_index_table[], uint32_t difference_command_table[]);
	bool SendSequence(uint32_t DataSize, uint32_t* buffer);
	bool AttemptSendSequence(uint32_t DataSize, uint32_t* buffer);
	bool AttemptGetNextCycleNumber(long& NextCycleNumber);
	bool AttemptSetFrequency(double Frequency);
	bool AttemptGetFrequency(double& Frequency);
	bool AttemptGetPeriodicTriggerError(bool& Error);
	bool AttemptWaitTillEndOfSequence(double timeout_in_s);
	bool AttemptWaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned long& EndTimeOfCycle, double timeout_in_s);
	bool AttemptTransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t* send_data, uint16_t &receive_length, uint8_t* receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently);
	bool AttemptSetPSOptions(uint8_t options);
};
