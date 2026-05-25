#ifndef CONTROLAPI_H
#define CONTROLAPI_H


#include <QLibrary>
#include <cstddef>
#include <cstdint>

typedef void* HControlLightAPI;

/*
 ChatGPT, please provide function pointer types for the following functions, similar to the examples further down
    API_EXPORT HControlLightAPI CLA_GetInstance();
    API_EXPORT int CLA_Create(bool InitializeAfx, bool InitializeAfxSocket);
    API_EXPORT void CLA_Configure(bool _DisplayErrors);
    API_EXPORT void CLA_Cleanup();
    API_EXPORT const char* CLA_GetLastError();
    API_EXPORT bool CLA_LoadFromJSONFile(const char* filename);
    API_EXPORT void CLA_Initialize();
    API_EXPORT void CLA_SwitchDebugMode(bool OnOff, const char* FileName);
    API_EXPORT bool CLA_IsReady();
    API_EXPORT void CLA_StartAssemblingSequence();
    API_EXPORT bool CLA_SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    API_EXPORT bool CLA_Wait_ms(double time_in_ms);
    API_EXPORT double CLA_GetTime_ms();
    API_EXPORT bool CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
    API_EXPORT void CLA_ExecuteSequence();
    API_EXPORT bool CLA_GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten);
    API_EXPORT bool CLA_WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
    API_EXPORT void CLA_SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms);
    API_EXPORT bool CLA_SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
    API_EXPORT bool CLA_SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
    API_EXPORT bool CLA_SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer);
    API_EXPORT bool CLA_SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff);
    API_EXPORT bool CLA_SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff);
    API_EXPORT bool CLA_SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker);
    API_EXPORT bool CLA_AddDeviceSequencer(
        unsigned int id,
        const char* type,
        const char* ip,
        unsigned int port,
        bool master,
        unsigned int startDelay,
        double clockFrequency,
        unsigned long FPGAClockToBusClockRatio,
        bool useExternalClock,
        bool useStrobeGenerator,
        bool connect);
    API_EXPORT bool CLA_AddDeviceAnalogOut16bit(
        unsigned int sequencer,
        unsigned int startAddress,
        unsigned int numberChannels,
        bool signedValue,
        double minVoltage,
        double maxVoltage);
    API_EXPORT bool CLA_AddDeviceDigitalOut(
        unsigned int sequencer,
        unsigned int address,
        unsigned int numberChannels);
    API_EXPORT bool CLA_AddDeviceAD9854(
        unsigned int sequencer,
        unsigned int address,
        unsigned int version,
        double externalClockFrequency,
        uint8_t PLLReferenceMultiplier,
        unsigned int frequencyMultiplier);
    API_EXPORT bool CLA_AddDeviceAD9858(
        unsigned int sequencer,
        unsigned int address,
        double externalClockFrequency,
        unsigned int frequencyMultiplier);
    API_EXPORT bool CLA_AddDeviceAnalogIn12bit(
        unsigned int sequencer,
        unsigned int chipSelect,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

API_EXPORT ERROR_CODE_TYPE CLA_SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);
//AD9854
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetPower(const unsigned int& Sequencer, const unsigned int& Address, double Power);
API_EXPORT ERROR_CODE_TYPE CLA_SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);
API_EXPORT ERROR_CODE_TYPE CLA_SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);
API_EXPORT ERROR_CODE_TYPE CLA_SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);
API_EXPORT ERROR_CODE_TYPE CLA_SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
API_EXPORT ERROR_CODE_TYPE CLA_SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

//AD9858
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

//AD9959
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9959Frequency(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9959FrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9959Phase(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);
API_EXPORT ERROR_CODE_TYPE CLA_SetAD9959Power(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);


*/

typedef HControlLightAPI (*GetInstanceFunc)();
typedef bool (*CreateFunc)(bool, bool);
typedef void (*ConfigureFunc)(bool);
typedef void (*CleanupFunc)();
typedef bool (*DidErrorOccurFunc)();
typedef const char* (*GetLastErrorFunc)();
typedef bool (*LoadFromJSONFileFunc)(const char*);
typedef void (*InitializeFunc)();
typedef void (*SwitchDebugModeFunc)(bool, const char*);
typedef bool (*IsReadyFunc)();
typedef void (*StartAssemblingSequenceFunc)();
typedef bool (*SetValueFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetRegisterFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetValueSerialDeviceFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetRegisterSerialDeviceFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*Wait_msFunc)(double);
typedef bool (*GetTime_msFunc)(double&);
typedef bool (*GetTimeOfSequencer_msFunc)(const unsigned int&, double&);
typedef bool (*GetTimeDebtOfSequencer_msFunc)(const unsigned int&, double&);
typedef bool (*TransmitI2CPortFunc)(uint8_t, uint8_t, uint8_t, uint16_t, uint8_t*, uint16_t, uint8_t*, uint32_t, bool&, bool);
typedef bool (*SetPSOptionsFunc)(uint8_t);
typedef bool (*SetVoltageFunc)(const unsigned int&, const unsigned int&, double);
typedef void (*ExecuteSequenceFunc)(const char*);
typedef bool (*GetSequenceExecutionStatusFunc)(bool&, unsigned long long&);
typedef bool (*WaitTillEndOfSequenceThenGetInputDataFunc)(uint8_t*&, unsigned long&, unsigned long&, double);
typedef void (*SetTimeDebtGuardFunc)(const double&);
typedef bool (*SequencerStartAnalogInAcquisitionFunc)(const unsigned int&, const uint8_t&, const uint8_t&, const uint8_t&, const uint32_t&, const double&);
typedef bool (*SequencerWriteInputMemoryFunc)(const unsigned int&, unsigned long, bool, unsigned long);
typedef bool (*SequencerWriteSystemTimeToInputMemoryFunc)(const unsigned int&);
typedef bool (*SequencerSwitchDebugLEDFunc)(const unsigned int&, unsigned int);
typedef bool (*SequencerIgnoreTCPIPFunc)(const unsigned int&, bool);
typedef bool (*SequencerAddMarkerFunc)(const unsigned int&, unsigned char);
typedef bool (*AddDeviceSequencerFunc)(
    unsigned int, const char*, const char*, unsigned int, bool, unsigned int,
    double, unsigned long, bool, bool, bool, bool);
typedef bool (*AddDeviceAnalogOut16bitFunc)(
    unsigned int, unsigned int, unsigned int, bool, double, double);
typedef bool (*AddDeviceDigitalOutFunc)(
    unsigned int, unsigned int, unsigned int);
typedef bool (*AddDeviceAD9854Func)(
    unsigned int, unsigned int, unsigned int, double, uint8_t, unsigned int);
typedef bool (*AddDeviceAD9858Func)(
    unsigned int, unsigned int, double, unsigned int);
typedef bool (*AddDeviceAnalogIn12bitFunc)(
    unsigned int, unsigned int, bool, double, double);
typedef bool (*SetDigitalOutputFunc)(const unsigned int&, const unsigned int&, uint8_t, bool);
typedef bool (*SetStartFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetStopFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetModulationFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetPowerFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetAttenuationFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetStartFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetStopFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetFSKModeFunc)(const unsigned int&, const unsigned int&, uint8_t);
typedef bool (*SetRampRateClockFunc)(const unsigned int&, const unsigned int&, uint8_t);
typedef bool (*SetClearACC1Func)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetTriangleBitFunc)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetFSKBitFunc)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetFrequencyOfChannelFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
typedef bool (*SetFrequencyTuningWordOfChannelFunc)(const unsigned int&, const unsigned int&, uint8_t, uint64_t);
typedef bool (*SetPhaseOfChannelFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
typedef bool (*SetPowerOfChannelFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
typedef bool (*SetIOUpdateEnabledFunc)(const unsigned int&, const unsigned int&, bool);
typedef bool (*AutoConfigureFunc)(const char* filename);
typedef void (*TransmitOnlyDifferenceBetweenCommandSequenceIfPossibleFunc)(bool OnOff);
typedef void (*StartAssemblingNextSequenceFunc)();
typedef unsigned int (*GetNumberOfSequencersFunc)();
typedef bool (*GetNextBufferPositionOfMasterSequencerFunc)(unsigned long& next_buffer_position);
typedef bool (*SetPeriodicTrigger_msFunc)(double PeriodicTriggerPeriod_in_ms, double PeriodicTriggerAllowedWaitTime_in_ms);
typedef bool (*GetNextCycleNumberFunc)(long& NextCycleNumber);
typedef bool (*ResetCycleNumberFunc)();
typedef bool (*ResetSequencerFunc)(uint8_t SequencerID);
typedef bool (*ResetAllSequencersFunc)();
typedef bool (*WriteConfigEEPROMFunc)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length);
typedef bool (*ReadConfigEEPROMFunc)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool &I2C_success);
typedef bool (*WriteConfigAddressFunc)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address);
typedef bool (*ReadConfigAddressFunc)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool& I2C_success);
typedef const char* (*ReadConfigurationFunc)(const char* filename);
typedef const char* (*GetAutoConfigJSONFunc)(const char* filename);
typedef bool (*StartAssemblingCPUCommandSequenceFunc)();
typedef bool (*AddCPUCommandFunc)(const char* command);
typedef bool (*ExecuteCPUCommandSequenceFunc)(unsigned long ethernet_check_period_in_ms);
typedef bool (*StopCPUCommandSequenceFunc)();
typedef bool (*InterruptCPUCommandSequenceFunc)();
typedef bool (*GetCPUCommandErrorMessagesFunc)();
typedef bool (*PrintCPUCommandErrorMessagesFunc)();
typedef bool (*PrintCPUCommandSequenceFunc)();
typedef bool (*ResetFunc)(const unsigned int& Sequencer, const unsigned int& Address);
typedef void (*SendSequenceFunc)(const char* FileName);
typedef void (*RepeatSequenceFunc)();
typedef bool (*WaitTillEndOfSequenceFunc)(double timeout_in_s);
typedef bool (*SequencerCalcAD9854FrequencyTuningWordFunc)(const unsigned int& Sequencer, uint64_t ftw0, uint8_t bit_shift);
typedef bool (*SetSequencerDigitalOutFunc)(const unsigned int& Sequencer, uint8_t dig_out_pattern);
typedef bool (*SetSequencer_PL_to_PS_commandFunc)(const unsigned int& Sequencer, uint8_t PL_to_PS_command);
typedef bool (*SwitchSequencerBuzzerFunc)(const unsigned int& Sequencer, bool OnOff);
typedef bool (*UseEdgeTriggeredLatchesFunc)(const unsigned int& Sequencer, bool UseEdgeTriggeredLatches);
typedef bool (*SequencerSetTimeDebtGuard_in_msFunc)(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms);
typedef bool (*SequencerSetLoopCountFunc)(const unsigned int& Sequencer, unsigned int loop_count);
typedef bool (*SequencerJumpBackwardFunc)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr, bool loop_count_greater_zero);
typedef bool (*SequencerJumpForwardFunc)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr);
typedef bool (*SequencerTransmitI2CFunc)(const unsigned int& Sequencer, uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t* data_out);
typedef bool (*SequencerTransmitSPIFunc)(const unsigned int& Sequencer, uint8_t chip_select, uint16_t number_of_bits_out, const uint8_t* data_out, uint8_t number_of_bits_in, bool start_now);
typedef bool (*SequencerRepeatedOutInFunc)(const unsigned int& Sequencer, uint16_t number_of_datapoints, double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand);
typedef bool (*SequencerSetSPITimingFunc)(const unsigned int& Sequencer, uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait);
typedef bool (*SequencerSetSPIModeFunc)(const unsigned int& Sequencer, uint8_t SPI_mode);
typedef bool (*SequencerSetI2CParametersFunc)(const unsigned int& Sequencer, uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read);
typedef bool (*SelectRackSlotFunc)(const unsigned int& Sequencer, uint8_t rack_nr, uint8_t slot_nr);
typedef bool (*ResetI2CMultiplexerFunc)(const unsigned int& Sequencer);
typedef bool (*AddDeviceAD9959Func)(unsigned int sequencer, unsigned int address, double externalClockFrequency, unsigned int frequencyMultiplier, bool AD9958, double version);

#endif




class CControlLightAPI
{

private:


/* ChatGPT, please provide variables for these typedefs, similar to the examples further down
typedef HControlLightAPI (*GetInstanceFunc)();
typedef void (*CreateFunc)(bool bool);
typedef void (*ConfigureFunc)(bool);
typedef void (*CleanupFunc)();
typedef const char* (*GetLastErrorFunc)();
typedef bool (*LoadFromJSONFileFunc)(const char*);
typedef void (*InitializeFunc)();
typedef void (*SwitchDebugModeFunc)(bool, const char*);
typedef bool (*IsReadyFunc)();
typedef void (*StartAssemblingSequenceFunc)();
typedef bool (*SetValueFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetRegisterFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetValueSerialDeviceFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*SetRegisterSerialDeviceFunc)(const unsigned int&, const unsigned int&, const unsigned int&, const uint8_t*, const unsigned long&, const uint8_t&);
typedef bool (*Wait_msFunc)(double);
typedef double (*GetTime_msFunc)();
typedef bool (*SetVoltageFunc)(const unsigned int&, const unsigned int&, double);
typedef void (*ExecuteSequenceFunc)();
typedef bool (*GetSequenceExecutionStatusFunc)(bool&, unsigned long long&);
typedef bool (*WaitTillEndOfSequenceThenGetInputDataFunc)(uint8_t*&, unsigned long&, unsigned long&, double);
typedef void (*SetTimeDebtGuardFunc)(const double&);
typedef bool (*SequencerStartAnalogInAcquisitionFunc)(const unsigned int&, const uint8_t&, const uint32_t&, const double&);
typedef bool (*SequencerWriteInputMemoryFunc)(const unsigned int&, unsigned long, bool, unsigned long);
typedef bool (*SequencerWriteSystemTimeToInputMemoryFunc)(const unsigned int&);
typedef bool (*SequencerSwitchDebugLEDFunc)(const unsigned int&, unsigned int);
typedef bool (*SequencerIgnoreTCPIPFunc)(const unsigned int&, bool);
typedef bool (*SequencerAddMarkerFunc)(const unsigned int&, unsigned char);
typedef bool (*AddDeviceSequencerFunc)(
    unsigned int, const char*, const char*, unsigned int, bool, unsigned int,
    double, unsigned long, bool, bool, bool);
typedef bool (*AddDeviceAnalogOut16bitFunc)(
    unsigned int, unsigned int, unsigned int, bool, double, double);
typedef bool (*AddDeviceDigitalOutFunc)(
    unsigned int, unsigned int, unsigned int);
typedef bool (*AddDeviceAD9854Func)(
    unsigned int, unsigned int, unsigned int, double, uint8_t, unsigned int);
typedef bool (*AddDeviceAD9858Func)(
    unsigned int, unsigned int, double, unsigned int);
typedef bool (*AddDeviceAnalogIn12bitFunc)(
    unsigned int, unsigned int, bool, double, double);
typedef bool (*SetDigitalOutputFunc)(const unsigned int&, const unsigned int&, uint8_t, bool);
typedef bool (*SetStartFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetStopFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetModulationFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetPowerFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetAttenuationFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetStartFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetStopFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetFSKModeFunc)(const unsigned int&, const unsigned int&, uint8_t);
typedef bool (*SetRampRateClockFunc)(const unsigned int&, const unsigned int&, uint8_t);
typedef bool (*SetClearACC1Func)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetTriangleBitFunc)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetFSKBitFunc)(const unsigned int&, const unsigned int&, bool);
typedef bool (*SetFrequencyFunc)(const unsigned int&, const unsigned int&, double);
typedef bool (*SetFrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint64_t);
typedef bool (*SetAD9959FrequencyFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
typedef bool (*SetAD9959FrequencyTuningWordFunc)(const unsigned int&, const unsigned int&, uint8_t, uint64_t);
typedef bool (*SetAD9959PhaseFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
typedef bool (*SetAD9959PowerFunc)(const unsigned int&, const unsigned int&, uint8_t, double);
*/



    GetInstanceFunc CLA_GetInstance;
    CreateFunc CLA_Create;
    ConfigureFunc CLA_Configure;
    CleanupFunc CLA_Cleanup;
    DidErrorOccurFunc CLA_DidErrorOccur;
    GetLastErrorFunc CLA_GetLastError;
    LoadFromJSONFileFunc CLA_LoadFromJSONFile;
    InitializeFunc CLA_Initialize;
    SwitchDebugModeFunc CLA_SwitchDebugMode;
    IsReadyFunc CLA_IsReady;
    StartAssemblingSequenceFunc CLA_StartAssemblingSequence;
    SetValueFunc CLA_SetValue;
    SetRegisterFunc CLA_SetRegister;
    SetValueSerialDeviceFunc CLA_SetValueSerialDevice;
    SetRegisterSerialDeviceFunc CLA_SetRegisterSerialDevice;
    Wait_msFunc CLA_Wait_ms;
    GetTime_msFunc CLA_GetTime_ms;
    GetTimeOfSequencer_msFunc CLA_GetTimeOfSequencer_ms;
    GetTimeDebtOfSequencer_msFunc CLA_GetTimeDebtOfSequencer_ms;
    TransmitI2CPortFunc CLA_TransmitI2CPort;
    SetPSOptionsFunc CLA_SetPSOptions;
    SetVoltageFunc CLA_SetVoltage;
    ExecuteSequenceFunc CLA_ExecuteSequence;
    GetSequenceExecutionStatusFunc CLA_GetSequenceExecutionStatus;
    WaitTillEndOfSequenceThenGetInputDataFunc CLA_WaitTillEndOfSequenceThenGetInputData;
    SetTimeDebtGuardFunc CLA_SetTimeDebtGuard_in_ms;
    SequencerStartAnalogInAcquisitionFunc CLA_SequencerStartAnalogInAcquisition;
    SequencerWriteInputMemoryFunc CLA_SequencerWriteInputMemory;
    SequencerWriteSystemTimeToInputMemoryFunc CLA_SequencerWriteSystemTimeToInputMemory;
    SequencerSwitchDebugLEDFunc CLA_SequencerSwitchDebugLED;
    SequencerIgnoreTCPIPFunc CLA_SequencerIgnoreTCPIP;
    SequencerAddMarkerFunc CLA_SequencerAddMarker;
    AddDeviceSequencerFunc CLA_AddDeviceSequencer;
    AddDeviceAnalogOut16bitFunc CLA_AddDeviceAnalogOut16bit;
    AddDeviceDigitalOutFunc CLA_AddDeviceDigitalOut;
    AddDeviceAD9854Func CLA_AddDeviceAD9854;
    AddDeviceAD9858Func CLA_AddDeviceAD9858;
    AddDeviceAnalogIn12bitFunc CLA_AddDeviceAnalogIn12bit;
    SetDigitalOutputFunc CLA_SetDigitalOutput;
    SetStartFrequencyFunc CLA_SetStartFrequency;
    SetStopFrequencyFunc CLA_SetStopFrequency;
    SetModulationFrequencyFunc CLA_SetModulationFrequency;
    SetPowerFunc CLA_SetPower;
    SetAttenuationFunc CLA_SetAttenuation;
    SetIOUpdateEnabledFunc CLA_SetIOUpdateEnabled;
    SetStartFrequencyTuningWordFunc CLA_SetStartFrequencyTuningWord;
    SetStopFrequencyTuningWordFunc CLA_SetStopFrequencyTuningWord;
    SetFSKModeFunc CLA_SetFSKMode;
    SetRampRateClockFunc CLA_SetRampRateClock;
    SetClearACC1Func CLA_SetClearACC1;
    SetTriangleBitFunc CLA_SetTriangleBit;
    SetFSKBitFunc CLA_SetFSKBit;
    SetFrequencyFunc CLA_SetFrequency;
    SetFrequencyTuningWordFunc CLA_SetFrequencyTuningWord;
    SetFrequencyOfChannelFunc CLA_SetFrequencyOfChannel;
    SetFrequencyTuningWordOfChannelFunc CLA_SetFrequencyTuningWordOfChannel;
    SetPhaseOfChannelFunc CLA_SetPhaseOfChannel;
    SetPowerOfChannelFunc CLA_SetPowerOfChannel;
    AutoConfigureFunc CLA_AutoConfigure;
    TransmitOnlyDifferenceBetweenCommandSequenceIfPossibleFunc CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible;
    StartAssemblingNextSequenceFunc CLA_StartAssemblingNextSequence;
    GetNumberOfSequencersFunc CLA_GetNumberOfSequencers;
    GetNextBufferPositionOfMasterSequencerFunc CLA_GetNextBufferPositionOfMasterSequencer;
    SetPeriodicTrigger_msFunc CLA_SetPeriodicTrigger_ms;
    GetNextCycleNumberFunc CLA_GetNextCycleNumber;
    ResetCycleNumberFunc CLA_ResetCycleNumber;
    ResetSequencerFunc CLA_ResetSequencer;
    ResetAllSequencersFunc CLA_ResetAllSequencers;
    WriteConfigEEPROMFunc CLA_WriteConfigEEPROM;
    ReadConfigEEPROMFunc CLA_ReadConfigEEPROM;
    WriteConfigAddressFunc CLA_WriteConfigAddress;
    ReadConfigAddressFunc CLA_ReadConfigAddress;
    ReadConfigurationFunc CLA_ReadConfiguration;
    GetAutoConfigJSONFunc CLA_GetAutoConfigJSON;
    StartAssemblingCPUCommandSequenceFunc CLA_StartAssemblingCPUCommandSequence;
    AddCPUCommandFunc CLA_AddCPUCommand;
    ExecuteCPUCommandSequenceFunc CLA_ExecuteCPUCommandSequence;
    StopCPUCommandSequenceFunc CLA_StopCPUCommandSequence;
    InterruptCPUCommandSequenceFunc CLA_InterruptCPUCommandSequence;
    GetCPUCommandErrorMessagesFunc CLA_GetCPUCommandErrorMessages;
    PrintCPUCommandErrorMessagesFunc CLA_PrintCPUCommandErrorMessages;
    PrintCPUCommandSequenceFunc CLA_PrintCPUCommandSequence;
    ResetFunc CLA_Reset;
    SendSequenceFunc CLA_SendSequence;
    RepeatSequenceFunc CLA_RepeatSequence;
    WaitTillEndOfSequenceFunc CLA_WaitTillEndOfSequence;
    SequencerCalcAD9854FrequencyTuningWordFunc CLA_SequencerCalcAD9854FrequencyTuningWord;
    SetSequencerDigitalOutFunc CLA_SetSequencerDigitalOut;
    SetSequencer_PL_to_PS_commandFunc CLA_SetSequencer_PL_to_PS_command;
    SwitchSequencerBuzzerFunc CLA_SwitchSequencerBuzzer;
    UseEdgeTriggeredLatchesFunc CLA_UseEdgeTriggeredLatches;
    SequencerSetTimeDebtGuard_in_msFunc CLA_SequencerSetTimeDebtGuard_in_ms;
    SequencerSetLoopCountFunc CLA_SequencerSetLoopCount;
    SequencerJumpBackwardFunc CLA_SequencerJumpBackward;
    SequencerJumpForwardFunc CLA_SequencerJumpForward;
    SequencerTransmitI2CFunc CLA_SequencerTransmitI2C;
    SequencerTransmitSPIFunc CLA_SequencerTransmitSPI;
    SequencerRepeatedOutInFunc CLA_SequencerRepeatedOutIn;
    SequencerSetSPITimingFunc CLA_SequencerSetSPITiming;
    SequencerSetSPIModeFunc CLA_SequencerSetSPIMode;
    SequencerSetI2CParametersFunc CLA_SequencerSetI2CParameters;
    SelectRackSlotFunc CLA_SelectRackSlot;
    ResetI2CMultiplexerFunc CLA_ResetI2CMultiplexer;
    AddDeviceAD9959Func CLA_AddDeviceAD9959;


    HControlLightAPI CLA_Handle;
    QLibrary* CLA_Lib;
    

    void Set_CLA_CallsToNull();



    private:
        bool DLL_Loaded;
    public:
        bool isLoaded() {return this->DLL_Loaded;}
    private:
        void Call_CLA_Cleanup();
    public:
        CControlLightAPI();
        virtual ~CControlLightAPI();
        void MessageBox(QString aMessage);
        bool LoadDLL(); //ok
    
        HControlLightAPI GetInstance();
        //Call these funcitons in roughly this order
        bool Create(bool InitializeAfx = true, bool InitializeAfxSocket = true); //you must call this first, otherwise the API will not work
        void Cleanup(); //You must call this before leaving your program, otherwise the API can provoke errors because memory is not freed
        bool DidErrorOccur();
        const char* GetLastError();
        void Configure(bool _DisplayErrors); //optional
    
        //you can load the configuration from a JSON file, or define the IO devices with the functions further below (mixing is also possible)
        bool LoadFromJSONFile(const char* filename);
    
        //Once all devices have been declared, you must initialize the system, otherwise the API will not work
        void Initialize();
        //Some optional commands
        void SwitchDebugMode(bool OnOff, const char* Filename);
        bool IsReady();
    
        //To start a sequence, first call StartAssemblingSequence, then add all the commands you want to execute in the sequence
    
        void StartAssemblingSequence();
    
        //here are possible commands
        bool SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
        bool SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
        bool SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
        bool SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
        bool Wait_ms(double time_in_ms);
        bool GetTime_ms(double &time_in_ms);
        bool GetTimeOfSequencer_ms(const unsigned int& Sequencer,double &time_in_ms);
        bool GetTimeDebtOfSequencer_ms(const unsigned int& Sequencer,double &time_in_ms);
        bool TransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t* send_data, uint16_t receive_length, uint8_t* receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently);
        bool SetPSOptions(uint8_t options);
        bool AutoConfigure(const char* filename = "");
        void TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool OnOff);
        void StartAssemblingNextSequence();
        unsigned int GetNumberOfSequencers();
        bool GetNextBufferPositionOfMasterSequencer(unsigned long& next_buffer_position);
        bool SetPeriodicTrigger_ms(double PeriodicTriggerPeriod_in_ms, double PeriodicTriggerAllowedWaitTime_in_ms);
        bool GetNextCycleNumber(long& NextCycleNumber);
        bool ResetCycleNumber();
        bool ResetSequencer(uint8_t SequencerID);
        bool ResetAllSequencers();
        bool WriteConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length);
        bool ReadConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool &I2C_success);
        bool WriteConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address);
        bool ReadConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool& I2C_success);
        const char* ReadConfiguration(const char* filename = "");
        const char* GetAutoConfigJSON(const char* filename = "");
        bool StartAssemblingCPUCommandSequence();
        bool AddCPUCommand(const char* command);
        bool ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms);
        bool StopCPUCommandSequence();
        bool InterruptCPUCommandSequence();
        bool GetCPUCommandErrorMessages();
        bool PrintCPUCommandErrorMessages();
        bool PrintCPUCommandSequence();
        bool Reset(const unsigned int& Sequencer, const unsigned int& Address);
        void SendSequence(const char* FileName = "");
        void RepeatSequence();
        bool WaitTillEndOfSequence(double timeout_in_s = 0);
        bool SequencerCalcAD9854FrequencyTuningWord(const unsigned int& Sequencer, uint64_t ftw0, uint8_t bit_shift = 22);
        bool SetSequencerDigitalOut(const unsigned int& Sequencer, uint8_t dig_out_pattern);
        bool SetSequencer_PL_to_PS_command(const unsigned int& Sequencer, uint8_t PL_to_PS_command);
        bool SwitchSequencerBuzzer(const unsigned int& Sequencer, bool OnOff);
        bool UseEdgeTriggeredLatches(const unsigned int& Sequencer, bool UseEdgeTriggeredLatches);
        bool SequencerSetTimeDebtGuard_in_ms(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms);
        bool SequencerSetLoopCount(const unsigned int& Sequencer, unsigned int loop_count);
        bool SequencerJumpBackward(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0, bool loop_count_greater_zero = false);
        bool SequencerJumpForward(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0);
        bool SequencerTransmitI2C(const unsigned int& Sequencer, uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t* data_out);
        bool SequencerTransmitSPI(const unsigned int& Sequencer, uint8_t chip_select, uint16_t number_of_bits_out, const uint8_t* data_out, uint8_t number_of_bits_in, bool start_now);
        bool SequencerRepeatedOutIn(const unsigned int& Sequencer, uint16_t number_of_datapoints, double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand);
        bool SequencerSetSPITiming(const unsigned int& Sequencer, uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait);
        bool SequencerSetSPIMode(const unsigned int& Sequencer, uint8_t SPI_mode);
        bool SequencerSetI2CParameters(const unsigned int& Sequencer, uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read);
        bool SelectRackSlot(const unsigned int& Sequencer, uint8_t rack_nr, uint8_t slot_nr);
        bool ResetI2CMultiplexer(const unsigned int& Sequencer);
        bool AddDeviceAD9959(unsigned int sequencer, unsigned int address, double externalClockFrequency, unsigned int frequencyMultiplier, bool AD9958, double version);
    
        //the following are convenience functions, which allow us to define nice names to the few most important functions
        //You can add as many convenience functions as you like. Make sure to copy them also into the list of convenience functions in CDevice, CDevice.h, to assure they can always be called in any device.
        //Then define them in the device that provides the function. In this way we use the inheritance mechanism to automatically check if the function is available in the device and (optionally) produce an error if not.
        
        //set the voltage of a device
        bool SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
        //set the digital output of a device
        bool SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);
        //set the start frequency of the AD9854
        bool SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
        //set the stop frequency of the AD9854
        bool SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
        //set the modulation frequency of the AD9854
        bool SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
        //set the power of the AD9854 or AD9858
        bool SetPower(const unsigned int& Sequencer, const unsigned int& Address, double Power);
        //set the attenuation of the AD9854 or AD9858
        bool SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);
        //set the start frequency tuning word of the AD9854
        bool SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
        //set the stop frequency tuning word of the AD9854
        bool SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
        //set the FSK mode of the AD9854
        bool SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);
        //set the ramp rate clock of the AD9854
        bool SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);
        //set the clear ACC1 bit of the AD9854
        bool SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
        //set the triangle bit of the AD9854
        bool SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
        //set the FSK bit of the AD9854
        bool SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);
        //set the frequency of the AD9858
        bool SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);
        //set the frequency tuning word of the AD9858
        bool SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);
        //set the frequency of the AD9959
        bool SetFrequencyOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
        //set the frequency tuning word of the AD9959
        bool SetFrequencyTuningWordOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
        //set the phase of the AD9959
        bool SetPhaseOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);
        //set the power of the AD9959
        bool SetPowerOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);
        //set auto IOUpdate for the AD9959
        bool SetIOUpdateEnabled(const unsigned int& Sequencer, const unsigned int& Address, bool EnableIOUpdate);

    


    
        // once the sequence is assembled, then execute it
        void ExecuteSequence(const char* Filename = "");
        //check how far the sequence has been executed
        bool GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten);
        //Wait till the sequence is finished, and get the data from the input devices
        bool WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
        void SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms);
        bool SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& analog_in_type, const uint8_t& SPI_CS, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
        bool SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
        bool SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer);
        bool SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff);
        bool SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff);
        bool SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker);
    
    
    
    
        //the following functions are used to add devices to the sequencer. I placed them here to avoid clutter above. They have to be called before Initialize().
        bool AddDeviceSequencer(
            unsigned int id,
            const char* type,
            const char* ip,
            unsigned int port,
            bool master,
            unsigned int startDelay,
            double clockFrequency,
            unsigned long StrobeDurationInFPGAClockPeriods,
            bool useExternalClock,
            bool useStrobeGenerator,
            bool useEdgeTriggeredLatches,
            bool connect);
    
        bool AddDeviceAnalogOut16bit(
            unsigned int sequencer,
            unsigned int startAddress,
            unsigned int numberChannels,
            bool signedValue,
            double minVoltage,
            double maxVoltage);
    
        bool AddDeviceDigitalOut(
            unsigned int sequencer,
            unsigned int address,
            unsigned int numberChannels);
    
        bool AddDeviceAD9854(
            unsigned int sequencer,
            unsigned int address,
            unsigned int version,
            double externalClockFrequency,
            uint8_t PLLReferenceMultiplier,
            unsigned int frequencyMultiplier);
    
        bool AddDeviceAD9858(
            unsigned int sequencer,
            unsigned int address,
            double externalClockFrequency,
            unsigned int frequencyMultiplier);
    
        bool AddDeviceAnalogIn12bit(
            unsigned int sequencer,
            unsigned int chipSelect,
            bool signedValue,
            double minVoltage,
            double maxVoltage);
        
};

