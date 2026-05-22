// ControlAPI.cpp: implementation of the API.
//
//////////////////////////////////////////////////////////////////////

#include  "ControlLightAPI.h"


#include <QDebug>



CControlLightAPI::CControlLightAPI()
{
    DLL_Loaded = false;
    Set_CLA_CallsToNull();
}



void CControlLightAPI::Set_CLA_CallsToNull() {
    
/*ChatGPT, please provide code to set these function pointers to nullptr, see examples further down
    CreateFunc CLA_Create;
    ConfigureFunc CLA_Configure;
    CleanupFunc CLA_Cleanup;
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
    SetStartFrequencyTuningWordFunc CLA_SetStartFrequencyTuningWord;
    SetStopFrequencyTuningWordFunc CLA_SetStopFrequencyTuningWord;
    SetFSKModeFunc CLA_SetFSKMode;
    SetRampRateClockFunc CLA_SetRampRateClock;
    SetClearACC1Func CLA_SetClearACC1;
    SetTriangleBitFunc CLA_SetTriangleBit;
    SetFSKBitFunc CLA_SetFSKBit;
    SetFrequencyFunc CLA_SetFrequency;
    SetFrequencyTuningWordFunc CLA_SetFrequencyTuningWord;
    SetAD9959FrequencyFunc CLA_SetAD9959Frequency;
    SetAD9959FrequencyTuningWordFunc CLA_SetAD9959FrequencyTuningWord;
    SetAD9959PhaseFunc CLA_SetAD9959Phase;
    SetAD9959PowerFunc CLA_SetAD9959Power;

    */

    //CLA_GetInstance = nullptr;
    CLA_Create = nullptr;
    CLA_Configure = nullptr;
    CLA_DidErrorOccur = nullptr;
    CLA_GetLastError = nullptr;
    CLA_LoadFromJSONFile = nullptr;
    CLA_Initialize = nullptr;
    CLA_SwitchDebugMode = nullptr;
    CLA_IsReady = nullptr;
    CLA_StartAssemblingSequence = nullptr;
    CLA_SetValue = nullptr;
    CLA_SetRegister = nullptr;
    CLA_SetValueSerialDevice = nullptr;
    CLA_SetRegisterSerialDevice = nullptr;
    CLA_Wait_ms = nullptr;
    CLA_GetTime_ms = nullptr;
    CLA_GetTimeOfSequencer_ms = nullptr;
    CLA_GetTimeDebtOfSequencer_ms = nullptr;
    CLA_TransmitI2CPort = nullptr;
    CLA_SetPSOptions = nullptr;
    CLA_SetVoltage = nullptr;
    CLA_ExecuteSequence = nullptr;
    CLA_GetSequenceExecutionStatus = nullptr;
    CLA_WaitTillEndOfSequenceThenGetInputData = nullptr;
    CLA_SetTimeDebtGuard_in_ms = nullptr;
    CLA_SequencerStartAnalogInAcquisition = nullptr;
    CLA_SequencerWriteInputMemory = nullptr;
    CLA_SequencerWriteSystemTimeToInputMemory = nullptr;
    CLA_SequencerSwitchDebugLED = nullptr;
    CLA_SequencerIgnoreTCPIP = nullptr;
    CLA_SequencerAddMarker = nullptr;
    CLA_AddDeviceSequencer = nullptr;
    CLA_AddDeviceAnalogOut16bit = nullptr;
    CLA_AddDeviceDigitalOut = nullptr;
    CLA_AddDeviceAD9854 = nullptr;
    CLA_AddDeviceAD9858 = nullptr;
    CLA_AddDeviceAnalogIn12bit = nullptr;
    CLA_SetDigitalOutput = nullptr;
    CLA_SetStartFrequency = nullptr;
    CLA_SetStopFrequency = nullptr;
    CLA_SetModulationFrequency = nullptr;
    CLA_SetPower = nullptr;
    CLA_SetAttenuation = nullptr;
    CLA_SetStartFrequencyTuningWord = nullptr;
    CLA_SetStopFrequencyTuningWord = nullptr;
    CLA_SetFSKMode = nullptr;
    CLA_SetRampRateClock = nullptr;
    CLA_SetClearACC1 = nullptr;
    CLA_SetTriangleBit = nullptr;
    CLA_SetFSKBit = nullptr;
    CLA_SetFrequency = nullptr;
    CLA_SetFrequencyTuningWord = nullptr;
    CLA_SetFrequencyOfChannel = nullptr;
    CLA_SetFrequencyTuningWordOfChannel = nullptr;
    CLA_SetPhaseOfChannel = nullptr;
    CLA_SetPowerOfChannel = nullptr;
    CLA_SetIOUpdateEnabled = nullptr;
    CLA_AutoConfigure = nullptr;
    CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible = nullptr;
    CLA_StartAssemblingNextSequence = nullptr;
    CLA_GetNumberOfSequencers = nullptr;
    CLA_GetNextBufferPositionOfMasterSequencer = nullptr;
    CLA_SetPeriodicTrigger_ms = nullptr;
    CLA_GetNextCycleNumber = nullptr;
    CLA_ResetCycleNumber = nullptr;
    CLA_WriteConfigEEPROM = nullptr;
    CLA_ReadConfigEEPROM = nullptr;
    CLA_WriteConfigAddress = nullptr;
    CLA_ReadConfigAddress = nullptr;
    CLA_ReadConfiguration = nullptr;
    CLA_GetAutoConfigJSON = nullptr;
    CLA_StartAssemblingCPUCommandSequence = nullptr;
    CLA_AddCPUCommand = nullptr;
    CLA_ExecuteCPUCommandSequence = nullptr;
    CLA_StopCPUCommandSequence = nullptr;
    CLA_InterruptCPUCommandSequence = nullptr;
    CLA_GetCPUCommandErrorMessages = nullptr;
    CLA_PrintCPUCommandErrorMessages = nullptr;
    CLA_PrintCPUCommandSequence = nullptr;
    CLA_Reset = nullptr;
    CLA_SendSequence = nullptr;
    CLA_RepeatSequence = nullptr;
    CLA_WaitTillEndOfSequence = nullptr;
    CLA_SequencerCalcAD9854FrequencyTuningWord = nullptr;
    CLA_SetSequencerDigitalOut = nullptr;
    CLA_SetSequencer_PL_to_PS_command = nullptr;
    CLA_SwitchSequencerBuzzer = nullptr;
    CLA_UseEdgeTriggeredLatches = nullptr;
    CLA_SequencerSetTimeDebtGuard_in_ms = nullptr;
    CLA_SequencerSetLoopCount = nullptr;
    CLA_SequencerJumpBackward = nullptr;
    CLA_SequencerJumpForward = nullptr;
    CLA_SequencerTransmitI2C = nullptr;
    CLA_SequencerTransmitSPI = nullptr;
    CLA_SequencerRepeatedOutIn = nullptr;
    CLA_SequencerSetSPITiming = nullptr;
    CLA_SequencerSetSPIMode = nullptr;
    CLA_SequencerSetI2CParameters = nullptr;
    CLA_SelectRackSlot = nullptr;
    CLA_ResetI2CMultiplexer = nullptr;
    CLA_AddDeviceAD9959 = nullptr;

}


CControlLightAPI::~CControlLightAPI()
{
    Cleanup();
}

void CControlLightAPI::MessageBox(QString aMessage) {
    qDebug() << aMessage;
}

#define TRY_RESOLVE(name) \
name = (decltype(name))CLA_Lib->resolve(#name); \
    if (!name) qDebug() << "Failed to resolve:" << #name;


/*

void listAllExports(const QString& dllPath) {
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModule = LoadLibraryW((LPCWSTR)dllPath.utf16());
    if (!hModule) {
        qDebug() << "LoadLibraryW failed.";
        return;
    }

    ULONG size;
    PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData(
        hModule, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size);

    if (!exports) {
        qDebug() << "No exports found.";
        return;
    }

    DWORD* names = (DWORD*)((BYTE*)hModule + exports->AddressOfNames);
    for (DWORD i = 0; i < exports->NumberOfNames; i++) {
        const char* funcName = (const char*)hModule + names[i];
        qDebug() << "Exported:" << funcName;
    }
}
*/

bool CControlLightAPI::LoadDLL() {


   // listAllExports("ControlLight.dll");//for debug

    CLA_Lib = new QLibrary("ControlLight.dll");

    //for debug

    TRY_RESOLVE(CLA_Configure);
    TRY_RESOLVE(CLA_Cleanup);
    TRY_RESOLVE(CLA_Create);
    TRY_RESOLVE(CLA_GetLastError);
    TRY_RESOLVE(CLA_DidErrorOccur);
    TRY_RESOLVE(CLA_LoadFromJSONFile);
    TRY_RESOLVE(CLA_Initialize);
    TRY_RESOLVE(CLA_SwitchDebugMode);
    TRY_RESOLVE(CLA_IsReady);
    TRY_RESOLVE(CLA_StartAssemblingSequence);
    TRY_RESOLVE(CLA_SetValue);
    TRY_RESOLVE(CLA_SetRegister);
    TRY_RESOLVE(CLA_SetValueSerialDevice);
    TRY_RESOLVE(CLA_SetRegisterSerialDevice);
    TRY_RESOLVE(CLA_Wait_ms);
    TRY_RESOLVE(CLA_GetTime_ms);
    TRY_RESOLVE(CLA_GetTimeOfSequencer_ms);
    TRY_RESOLVE(CLA_GetTimeDebtOfSequencer_ms);
    TRY_RESOLVE(CLA_TransmitI2CPort);
    TRY_RESOLVE(CLA_SetPSOptions);
    TRY_RESOLVE(CLA_SetVoltage);
    TRY_RESOLVE(CLA_ExecuteSequence);
    TRY_RESOLVE(CLA_GetSequenceExecutionStatus);
    TRY_RESOLVE(CLA_WaitTillEndOfSequenceThenGetInputData);
    TRY_RESOLVE(CLA_SetTimeDebtGuard_in_ms);
    TRY_RESOLVE(CLA_SequencerStartAnalogInAcquisition);
    TRY_RESOLVE(CLA_SequencerWriteInputMemory);
    TRY_RESOLVE(CLA_SequencerWriteSystemTimeToInputMemory);
    TRY_RESOLVE(CLA_SequencerSwitchDebugLED);
    TRY_RESOLVE(CLA_SequencerIgnoreTCPIP);
    TRY_RESOLVE(CLA_SequencerAddMarker);
    TRY_RESOLVE(CLA_AddDeviceSequencer);
    TRY_RESOLVE(CLA_AddDeviceAnalogOut16bit);
    TRY_RESOLVE(CLA_AddDeviceDigitalOut);
    TRY_RESOLVE(CLA_AddDeviceAD9854);
    TRY_RESOLVE(CLA_AddDeviceAD9858);
    TRY_RESOLVE(CLA_AddDeviceAnalogIn12bit);
    TRY_RESOLVE(CLA_SetDigitalOutput);
    TRY_RESOLVE(CLA_SetStartFrequency);
    TRY_RESOLVE(CLA_SetStopFrequency);
    TRY_RESOLVE(CLA_SetModulationFrequency);
    TRY_RESOLVE(CLA_SetPower);
    TRY_RESOLVE(CLA_SetAttenuation);
    TRY_RESOLVE(CLA_SetStartFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetStopFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetFSKMode);
    TRY_RESOLVE(CLA_SetRampRateClock);
    TRY_RESOLVE(CLA_SetClearACC1);
    TRY_RESOLVE(CLA_SetTriangleBit);
    TRY_RESOLVE(CLA_SetFSKBit);
    TRY_RESOLVE(CLA_SetFrequency);
    TRY_RESOLVE(CLA_SetFrequencyTuningWord);
    TRY_RESOLVE(CLA_SetFrequencyOfChannel);
    TRY_RESOLVE(CLA_SetFrequencyTuningWordOfChannel);
    TRY_RESOLVE(CLA_SetPhaseOfChannel);
    TRY_RESOLVE(CLA_SetPowerOfChannel);
    TRY_RESOLVE(CLA_SetIOUpdateEnabled);
    TRY_RESOLVE(CLA_AutoConfigure);
    TRY_RESOLVE(CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible);
    TRY_RESOLVE(CLA_StartAssemblingNextSequence);
    TRY_RESOLVE(CLA_GetNumberOfSequencers);
    TRY_RESOLVE(CLA_GetNextBufferPositionOfMasterSequencer);
    TRY_RESOLVE(CLA_SetPeriodicTrigger_ms);
    TRY_RESOLVE(CLA_GetNextCycleNumber);
    TRY_RESOLVE(CLA_ResetCycleNumber);
    TRY_RESOLVE(CLA_WriteConfigEEPROM);
    TRY_RESOLVE(CLA_ReadConfigEEPROM);
    TRY_RESOLVE(CLA_WriteConfigAddress);
    TRY_RESOLVE(CLA_ReadConfigAddress);
    TRY_RESOLVE(CLA_ReadConfiguration);
    TRY_RESOLVE(CLA_GetAutoConfigJSON);
    TRY_RESOLVE(CLA_StartAssemblingCPUCommandSequence);
    TRY_RESOLVE(CLA_AddCPUCommand);
    TRY_RESOLVE(CLA_ExecuteCPUCommandSequence);
    TRY_RESOLVE(CLA_StopCPUCommandSequence);
    TRY_RESOLVE(CLA_InterruptCPUCommandSequence);
    TRY_RESOLVE(CLA_GetCPUCommandErrorMessages);
    TRY_RESOLVE(CLA_PrintCPUCommandErrorMessages);
    TRY_RESOLVE(CLA_PrintCPUCommandSequence);
    TRY_RESOLVE(CLA_Reset);
    TRY_RESOLVE(CLA_SendSequence);
    TRY_RESOLVE(CLA_RepeatSequence);
    TRY_RESOLVE(CLA_WaitTillEndOfSequence);
    TRY_RESOLVE(CLA_SequencerCalcAD9854FrequencyTuningWord);
    TRY_RESOLVE(CLA_SetSequencerDigitalOut);
    TRY_RESOLVE(CLA_SetSequencer_PL_to_PS_command);
    TRY_RESOLVE(CLA_SwitchSequencerBuzzer);
    TRY_RESOLVE(CLA_UseEdgeTriggeredLatches);
    TRY_RESOLVE(CLA_SequencerSetTimeDebtGuard_in_ms);
    TRY_RESOLVE(CLA_SequencerSetLoopCount);
    TRY_RESOLVE(CLA_SequencerJumpBackward);
    TRY_RESOLVE(CLA_SequencerJumpForward);
    TRY_RESOLVE(CLA_SequencerTransmitI2C);
    TRY_RESOLVE(CLA_SequencerTransmitSPI);
    TRY_RESOLVE(CLA_SequencerRepeatedOutIn);
    TRY_RESOLVE(CLA_SequencerSetSPITiming);
    TRY_RESOLVE(CLA_SequencerSetSPIMode);
    TRY_RESOLVE(CLA_SequencerSetI2CParameters);
    TRY_RESOLVE(CLA_SelectRackSlot);
    TRY_RESOLVE(CLA_ResetI2CMultiplexer);
    TRY_RESOLVE(CLA_AddDeviceAD9959);

    //TRY_RESOLVE(CLA_GetInstance);

























    if (!CLA_Lib->load()) {
        qDebug() << "Failed to load DLL:" << CLA_Lib->errorString();
        return false;
    }

    /*
 ChatGPT, please provide function pointer types for the following functions, similar to the examples further down
    API_EXPORT HControlLightAPI CLA_GetInstance();
    API_EXPORT int CLA_Create(bool InitializeAfx, bool InitializeAfxSocket);
    API_EXPORT void CLA_Configure(bool _DisplayErrors);
    API_EXPORT void CLA_Cleanup();
    API_EXPORT const char* CLA_GetLastError();
    API_EXPORT bool CLA_LoadFromJSONFile(const char* filename);
    API_EXPORT void CLA_Initialize();
    API_EXPORT void CLA_SwitchDebugMode(bool OnOff);
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





    CLA_Configure = (ConfigureFunc)CLA_Lib->resolve("CLA_Configure");
    CLA_Create = (CreateFunc)CLA_Lib->resolve("CLA_Create");
    CLA_Cleanup = (CleanupFunc)CLA_Lib->resolve("CLA_Cleanup");
    CLA_GetLastError = (GetLastErrorFunc)CLA_Lib->resolve("CLA_GetLastError");
    CLA_DidErrorOccur = (DidErrorOccurFunc)CLA_Lib->resolve("CLA_DidErrorOccur");
    CLA_LoadFromJSONFile = (LoadFromJSONFileFunc)CLA_Lib->resolve("CLA_LoadFromJSONFile");
    CLA_Initialize = (InitializeFunc)CLA_Lib->resolve("CLA_Initialize");
    CLA_SwitchDebugMode = (SwitchDebugModeFunc)CLA_Lib->resolve("CLA_SwitchDebugMode");
    CLA_IsReady = (IsReadyFunc)CLA_Lib->resolve("CLA_IsReady");
    CLA_StartAssemblingSequence = (StartAssemblingSequenceFunc)CLA_Lib->resolve("CLA_StartAssemblingSequence");
    CLA_SetValue = (SetValueFunc)CLA_Lib->resolve("CLA_SetValue");
    CLA_SetRegister = (SetRegisterFunc)CLA_Lib->resolve("CLA_SetRegister");
    CLA_SetValueSerialDevice = (SetValueSerialDeviceFunc)CLA_Lib->resolve("CLA_SetValueSerialDevice");
    CLA_SetRegisterSerialDevice = (SetRegisterSerialDeviceFunc)CLA_Lib->resolve("CLA_SetRegisterSerialDevice");
    CLA_Wait_ms = (Wait_msFunc)CLA_Lib->resolve("CLA_Wait_ms");
    CLA_GetTime_ms = (GetTime_msFunc)CLA_Lib->resolve("CLA_GetTime_ms");
    CLA_GetTimeOfSequencer_ms = (GetTimeOfSequencer_msFunc)CLA_Lib->resolve("CLA_GetTimeOfSequencer_ms");
    CLA_GetTimeDebtOfSequencer_ms = (GetTimeDebtOfSequencer_msFunc)CLA_Lib->resolve("CLA_GetTimeDebtOfSequencer_ms");
    CLA_TransmitI2CPort = (TransmitI2CPortFunc)CLA_Lib->resolve("CLA_TransmitI2CPort");
    CLA_SetPSOptions = (SetPSOptionsFunc)CLA_Lib->resolve("CLA_SetPSOptions");
    CLA_SetVoltage = (SetVoltageFunc)CLA_Lib->resolve("CLA_SetVoltage");
    CLA_ExecuteSequence = (ExecuteSequenceFunc)CLA_Lib->resolve("CLA_ExecuteSequence");
    CLA_GetSequenceExecutionStatus = (GetSequenceExecutionStatusFunc)CLA_Lib->resolve("CLA_GetSequenceExecutionStatus");
    CLA_WaitTillEndOfSequenceThenGetInputData = (WaitTillEndOfSequenceThenGetInputDataFunc)CLA_Lib->resolve("CLA_WaitTillEndOfSequenceThenGetInputData");
    CLA_SetTimeDebtGuard_in_ms = (SetTimeDebtGuardFunc)CLA_Lib->resolve("CLA_SetTimeDebtGuard_in_ms");
    CLA_SequencerStartAnalogInAcquisition = (SequencerStartAnalogInAcquisitionFunc)CLA_Lib->resolve("CLA_SequencerStartAnalogInAcquisition");
    CLA_SequencerWriteInputMemory = (SequencerWriteInputMemoryFunc)CLA_Lib->resolve("CLA_SequencerWriteInputMemory");
    CLA_SequencerWriteSystemTimeToInputMemory = (SequencerWriteSystemTimeToInputMemoryFunc)CLA_Lib->resolve("CLA_SequencerWriteSystemTimeToInputMemory");
    CLA_SequencerSwitchDebugLED = (SequencerSwitchDebugLEDFunc)CLA_Lib->resolve("CLA_SequencerSwitchDebugLED");
    CLA_SequencerIgnoreTCPIP = (SequencerIgnoreTCPIPFunc)CLA_Lib->resolve("CLA_SequencerIgnoreTCPIP");
    CLA_SequencerAddMarker = (SequencerAddMarkerFunc)CLA_Lib->resolve("CLA_SequencerAddMarker");
    CLA_AddDeviceSequencer = (AddDeviceSequencerFunc)CLA_Lib->resolve("CLA_AddDeviceSequencer");
    CLA_AddDeviceAnalogOut16bit = (AddDeviceAnalogOut16bitFunc)CLA_Lib->resolve("CLA_AddDeviceAnalogOut16bit");
    CLA_AddDeviceDigitalOut = (AddDeviceDigitalOutFunc)CLA_Lib->resolve("CLA_AddDeviceDigitalOut");
    CLA_AddDeviceAD9854 = (AddDeviceAD9854Func)CLA_Lib->resolve("CLA_AddDeviceAD9854");
    CLA_AddDeviceAD9858 = (AddDeviceAD9858Func)CLA_Lib->resolve("CLA_AddDeviceAD9858");
    CLA_AddDeviceAnalogIn12bit = (AddDeviceAnalogIn12bitFunc)CLA_Lib->resolve("CLA_AddDeviceAnalogIn12bit");
    CLA_SetDigitalOutput = (SetDigitalOutputFunc)CLA_Lib->resolve("CLA_SetDigitalOutput");
    CLA_SetStartFrequency = (SetStartFrequencyFunc)CLA_Lib->resolve("CLA_SetStartFrequency");
    CLA_SetStopFrequency = (SetStopFrequencyFunc)CLA_Lib->resolve("CLA_SetStopFrequency");
    CLA_SetModulationFrequency = (SetModulationFrequencyFunc)CLA_Lib->resolve("CLA_SetModulationFrequency");
    CLA_SetPower = (SetPowerFunc)CLA_Lib->resolve("CLA_SetPower");
    CLA_SetAttenuation = (SetAttenuationFunc)CLA_Lib->resolve("CLA_SetAttenuation");
    CLA_SetStartFrequencyTuningWord = (SetStartFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetStartFrequencyTuningWord");
    CLA_SetStopFrequencyTuningWord = (SetStopFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetStopFrequencyTuningWord");
    CLA_SetFSKMode = (SetFSKModeFunc)CLA_Lib->resolve("CLA_SetFSKMode");
    CLA_SetRampRateClock = (SetRampRateClockFunc)CLA_Lib->resolve("CLA_SetRampRateClock");
    CLA_SetClearACC1 = (SetClearACC1Func)CLA_Lib->resolve("CLA_SetClearACC1");
    CLA_SetTriangleBit = (SetTriangleBitFunc)CLA_Lib->resolve("CLA_SetTriangleBit");
    CLA_SetFSKBit = (SetFSKBitFunc)CLA_Lib->resolve("CLA_SetFSKBit");
    CLA_SetFrequency = (SetFrequencyFunc)CLA_Lib->resolve("CLA_SetFrequency");
    CLA_SetFrequencyTuningWord = (SetFrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SetFrequencyTuningWord");
    CLA_SetFrequencyOfChannel = (SetFrequencyOfChannelFunc)CLA_Lib->resolve("CLA_SetFrequencyOfChannel");
    CLA_SetFrequencyTuningWordOfChannel = (SetFrequencyTuningWordOfChannelFunc)CLA_Lib->resolve("CLA_SetFrequencyTuningWordOfChannel");
    CLA_SetPhaseOfChannel = (SetPhaseOfChannelFunc)CLA_Lib->resolve("CLA_SetPhaseOfChannel");
    CLA_SetPowerOfChannel = (SetPowerOfChannelFunc)CLA_Lib->resolve("CLA_SetPowerOfChannel");
    CLA_SetIOUpdateEnabled = (SetIOUpdateEnabledFunc)CLA_Lib->resolve("CLA_SetIOUpdateEnabled");
    CLA_AutoConfigure = (AutoConfigureFunc)CLA_Lib->resolve("CLA_AutoConfigure");
    CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible = (TransmitOnlyDifferenceBetweenCommandSequenceIfPossibleFunc)CLA_Lib->resolve("CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible");
    CLA_StartAssemblingNextSequence = (StartAssemblingNextSequenceFunc)CLA_Lib->resolve("CLA_StartAssemblingNextSequence");
    CLA_GetNumberOfSequencers = (GetNumberOfSequencersFunc)CLA_Lib->resolve("CLA_GetNumberOfSequencers");
    CLA_GetNextBufferPositionOfMasterSequencer = (GetNextBufferPositionOfMasterSequencerFunc)CLA_Lib->resolve("CLA_GetNextBufferPositionOfMasterSequencer");
    CLA_SetPeriodicTrigger_ms = (SetPeriodicTrigger_msFunc)CLA_Lib->resolve("CLA_SetPeriodicTrigger_ms");
    CLA_GetNextCycleNumber = (GetNextCycleNumberFunc)CLA_Lib->resolve("CLA_GetNextCycleNumber");
    CLA_ResetCycleNumber = (ResetCycleNumberFunc)CLA_Lib->resolve("CLA_ResetCycleNumber");
    CLA_WriteConfigEEPROM = (WriteConfigEEPROMFunc)CLA_Lib->resolve("CLA_WriteConfigEEPROM");
    CLA_ReadConfigEEPROM = (ReadConfigEEPROMFunc)CLA_Lib->resolve("CLA_ReadConfigEEPROM");
    CLA_WriteConfigAddress = (WriteConfigAddressFunc)CLA_Lib->resolve("CLA_WriteConfigAddress");
    CLA_ReadConfigAddress = (ReadConfigAddressFunc)CLA_Lib->resolve("CLA_ReadConfigAddress");
    CLA_ReadConfiguration = (ReadConfigurationFunc)CLA_Lib->resolve("CLA_ReadConfiguration");
    CLA_GetAutoConfigJSON = (GetAutoConfigJSONFunc)CLA_Lib->resolve("CLA_GetAutoConfigJSON");
    CLA_StartAssemblingCPUCommandSequence = (StartAssemblingCPUCommandSequenceFunc)CLA_Lib->resolve("CLA_StartAssemblingCPUCommandSequence");
    CLA_AddCPUCommand = (AddCPUCommandFunc)CLA_Lib->resolve("CLA_AddCPUCommand");
    CLA_ExecuteCPUCommandSequence = (ExecuteCPUCommandSequenceFunc)CLA_Lib->resolve("CLA_ExecuteCPUCommandSequence");
    CLA_StopCPUCommandSequence = (StopCPUCommandSequenceFunc)CLA_Lib->resolve("CLA_StopCPUCommandSequence");
    CLA_InterruptCPUCommandSequence = (InterruptCPUCommandSequenceFunc)CLA_Lib->resolve("CLA_InterruptCPUCommandSequence");
    CLA_GetCPUCommandErrorMessages = (GetCPUCommandErrorMessagesFunc)CLA_Lib->resolve("CLA_GetCPUCommandErrorMessages");
    CLA_PrintCPUCommandErrorMessages = (PrintCPUCommandErrorMessagesFunc)CLA_Lib->resolve("CLA_PrintCPUCommandErrorMessages");
    CLA_PrintCPUCommandSequence = (PrintCPUCommandSequenceFunc)CLA_Lib->resolve("CLA_PrintCPUCommandSequence");
    CLA_Reset = (ResetFunc)CLA_Lib->resolve("CLA_Reset");
    CLA_SendSequence = (SendSequenceFunc)CLA_Lib->resolve("CLA_SendSequence");
    CLA_RepeatSequence = (RepeatSequenceFunc)CLA_Lib->resolve("CLA_RepeatSequence");
    CLA_WaitTillEndOfSequence = (WaitTillEndOfSequenceFunc)CLA_Lib->resolve("CLA_WaitTillEndOfSequence");
    CLA_SequencerCalcAD9854FrequencyTuningWord = (SequencerCalcAD9854FrequencyTuningWordFunc)CLA_Lib->resolve("CLA_SequencerCalcAD9854FrequencyTuningWord");
    CLA_SetSequencerDigitalOut = (SetSequencerDigitalOutFunc)CLA_Lib->resolve("CLA_SetSequencerDigitalOut");
    CLA_SetSequencer_PL_to_PS_command = (SetSequencer_PL_to_PS_commandFunc)CLA_Lib->resolve("CLA_SetSequencer_PL_to_PS_command");
    CLA_SwitchSequencerBuzzer = (SwitchSequencerBuzzerFunc)CLA_Lib->resolve("CLA_SwitchSequencerBuzzer");
    CLA_UseEdgeTriggeredLatches = (UseEdgeTriggeredLatchesFunc)CLA_Lib->resolve("CLA_UseEdgeTriggeredLatches");
    CLA_SequencerSetTimeDebtGuard_in_ms = (SequencerSetTimeDebtGuard_in_msFunc)CLA_Lib->resolve("CLA_SequencerSetTimeDebtGuard_in_ms");
    CLA_SequencerSetLoopCount = (SequencerSetLoopCountFunc)CLA_Lib->resolve("CLA_SequencerSetLoopCount");
    CLA_SequencerJumpBackward = (SequencerJumpBackwardFunc)CLA_Lib->resolve("CLA_SequencerJumpBackward");
    CLA_SequencerJumpForward = (SequencerJumpForwardFunc)CLA_Lib->resolve("CLA_SequencerJumpForward");
    CLA_SequencerTransmitI2C = (SequencerTransmitI2CFunc)CLA_Lib->resolve("CLA_SequencerTransmitI2C");
    CLA_SequencerTransmitSPI = (SequencerTransmitSPIFunc)CLA_Lib->resolve("CLA_SequencerTransmitSPI");
    CLA_SequencerRepeatedOutIn = (SequencerRepeatedOutInFunc)CLA_Lib->resolve("CLA_SequencerRepeatedOutIn");
    CLA_SequencerSetSPITiming = (SequencerSetSPITimingFunc)CLA_Lib->resolve("CLA_SequencerSetSPITiming");
    CLA_SequencerSetSPIMode = (SequencerSetSPIModeFunc)CLA_Lib->resolve("CLA_SequencerSetSPIMode");
    CLA_SequencerSetI2CParameters = (SequencerSetI2CParametersFunc)CLA_Lib->resolve("CLA_SequencerSetI2CParameters");
    CLA_SelectRackSlot = (SelectRackSlotFunc)CLA_Lib->resolve("CLA_SelectRackSlot");
    CLA_ResetI2CMultiplexer = (ResetI2CMultiplexerFunc)CLA_Lib->resolve("CLA_ResetI2CMultiplexer");
    CLA_AddDeviceAD9959 = (AddDeviceAD9959Func)CLA_Lib->resolve("CLA_AddDeviceAD9959");

    
    //now check if all functions were loaded correctly

    if (
        !CLA_Configure ||
        !CLA_Create ||
        !CLA_Cleanup ||
        !CLA_GetLastError ||
        !CLA_DidErrorOccur ||
        !CLA_LoadFromJSONFile ||
        !CLA_Initialize ||
        !CLA_SwitchDebugMode ||
        !CLA_IsReady ||
        !CLA_StartAssemblingSequence ||
        !CLA_SetValue ||
        !CLA_SetRegister ||
        !CLA_SetValueSerialDevice ||
        !CLA_SetRegisterSerialDevice ||
        !CLA_Wait_ms ||
        !CLA_GetTime_ms ||
        !CLA_GetTimeOfSequencer_ms ||
        !CLA_GetTimeDebtOfSequencer_ms ||
        !CLA_TransmitI2CPort ||
        !CLA_SetPSOptions ||
        !CLA_SetVoltage ||
        !CLA_ExecuteSequence ||
        !CLA_GetSequenceExecutionStatus ||
        !CLA_WaitTillEndOfSequenceThenGetInputData ||
        !CLA_SetTimeDebtGuard_in_ms ||
        !CLA_SequencerStartAnalogInAcquisition ||
        !CLA_SequencerWriteInputMemory ||
        !CLA_SequencerWriteSystemTimeToInputMemory ||
        !CLA_SequencerSwitchDebugLED ||
        !CLA_SequencerIgnoreTCPIP ||
        !CLA_SequencerAddMarker ||
        !CLA_AddDeviceSequencer ||
        !CLA_AddDeviceAnalogOut16bit ||
        !CLA_AddDeviceDigitalOut ||
        !CLA_AddDeviceAD9854 ||
        !CLA_AddDeviceAD9858 ||
        !CLA_AddDeviceAnalogIn12bit ||
        !CLA_SetDigitalOutput ||
        !CLA_SetStartFrequency ||
        !CLA_SetStopFrequency ||
        !CLA_SetModulationFrequency ||
        !CLA_SetPower ||
        !CLA_SetAttenuation ||
        !CLA_SetStartFrequencyTuningWord ||
        !CLA_SetStopFrequencyTuningWord ||
        !CLA_SetFSKMode ||
        !CLA_SetRampRateClock ||
        !CLA_SetClearACC1 ||
        !CLA_SetTriangleBit ||
        !CLA_SetFSKBit ||
        !CLA_SetFrequency ||
        !CLA_SetFrequencyTuningWord ||
        !CLA_SetFrequencyOfChannel ||
        !CLA_SetFrequencyTuningWordOfChannel ||
        !CLA_SetPhaseOfChannel ||
        !CLA_SetPowerOfChannel ||
        !CLA_AutoConfigure ||
        !CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible ||
        !CLA_StartAssemblingNextSequence ||
        !CLA_GetNumberOfSequencers ||
        !CLA_GetNextBufferPositionOfMasterSequencer ||
        !CLA_SetPeriodicTrigger_ms ||
        !CLA_GetNextCycleNumber ||
        !CLA_ResetCycleNumber ||
        !CLA_WriteConfigEEPROM ||
        !CLA_ReadConfigEEPROM ||
        !CLA_WriteConfigAddress ||
        !CLA_ReadConfigAddress ||
        !CLA_ReadConfiguration ||
        !CLA_GetAutoConfigJSON ||
        !CLA_StartAssemblingCPUCommandSequence ||
        !CLA_AddCPUCommand ||
        !CLA_ExecuteCPUCommandSequence ||
        !CLA_StopCPUCommandSequence ||
        !CLA_InterruptCPUCommandSequence ||
        !CLA_GetCPUCommandErrorMessages ||
        !CLA_PrintCPUCommandErrorMessages ||
        !CLA_PrintCPUCommandSequence ||
        !CLA_Reset ||
        !CLA_SendSequence ||
        !CLA_RepeatSequence ||
        !CLA_WaitTillEndOfSequence ||
        !CLA_SequencerCalcAD9854FrequencyTuningWord ||
        !CLA_SetSequencerDigitalOut ||
        !CLA_SetSequencer_PL_to_PS_command ||
        !CLA_SwitchSequencerBuzzer ||
        !CLA_UseEdgeTriggeredLatches ||
        !CLA_SequencerSetTimeDebtGuard_in_ms ||
        !CLA_SequencerSetLoopCount ||
        !CLA_SequencerJumpBackward ||
        !CLA_SequencerJumpForward ||
        !CLA_SequencerTransmitI2C ||
        !CLA_SequencerTransmitSPI ||
        !CLA_SequencerRepeatedOutIn ||
        !CLA_SequencerSetSPITiming ||
        !CLA_SequencerSetSPIMode ||
        !CLA_SequencerSetI2CParameters ||
        !CLA_SelectRackSlot ||
        !CLA_ResetI2CMultiplexer ||
        !CLA_AddDeviceAD9959 ||
        !CLA_SetIOUpdateEnabled
      
        ) {
        qDebug() << "Failed to resolve one or more ControlAPI symbols.";
        Cleanup();
        return false;
    }
    Configure(true);
    CLA_Handle = GetInstance();
    DLL_Loaded = true;
    return true;
}

void CControlLightAPI::Cleanup() {
    if (CLA_Lib) {
        Set_CLA_CallsToNull();
        Call_CLA_Cleanup();
        CLA_Cleanup = NULL;
        CLA_Lib->unload();
        CLA_Handle=nullptr;
        delete CLA_Lib;
        CLA_Lib = NULL;
    }
    DLL_Loaded = false;
}



/*ChatGPT, load the following functions, see example further down
    HControlLightAPI CLA_GetInstance();
    //Call these funcitons in roughly this order
    int CLA_Create(bool InitializeAfx, bool InitializeAfxSocket); //you must call this first, otherwise the API will not work
    void CLA_Configure(bool _DisplayErrors); //optional
    void CLA_Cleanup(); //You must call this before leaving your program, otherwise the API can provoke errors because memory is not freed
    const char* CLA_GetLastError();
    

    //you can load the configuration from a JSON file, or define the IO devices with the functions further below (mixing is also possible)
    bool CLA_LoadFromJSONFile(const char* filename);

    //Once all devices have been declared, you must initialize the system, otherwise the API will not work
    void CLA_Initialize();
    //Some optional commands
    void CLA_SwitchDebugMode(bool OnOff);
    bool CLA_IsReady();

    //To start a sequence, first call StartAssemblingSequence, then add all the commands you want to execute in the sequence

    void CLA_StartAssemblingSequence();

    //here are possible commands
    bool CLA_SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
    bool CLA_Wait_ms(double time_in_ms);
    double CLA_GetTime_ms();

    //the following are convenience functions, which allow us to define nice names to the few most important functions
    //You can add as many convenience functions as you like. Make sure to copy them also into the list of convenience functions in CDevice, CDevice.h, to assure they can always be called in any device.
    //Then define them in the device that provides the function. In this way we use the inheritance mechanism to automatically check if the function is available in the device and (optionally) produce an error if not.
    bool CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);


    // once the sequence is assembled, then execute it
    void CLA_ExecuteSequence();
    //check how far the sequence has been executed
    bool CLA_GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten);
    //Wait till the sequence is finished, and get the data from the input devices
    bool CLA_WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
    void CLA_SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms);
    bool CLA_SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
    bool CLA_SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
    bool CLA_SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer);
    bool CLA_SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff);
    bool CLA_SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff);
    bool CLA_SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker);




    //the following functions are used to add devices to the sequencer. I placed them here to avoid clutter above. They have to be called before Initialize().
    bool CLA_AddDeviceSequencer(
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

    bool CLA_AddDeviceAnalogOut16bit(
        unsigned int sequencer,
        unsigned int startAddress,
        unsigned int numberChannels,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

    bool CLA_AddDeviceDigitalOut(
        unsigned int sequencer,
        unsigned int address,
        unsigned int numberChannels);

    bool CLA_AddDeviceAD9854(
        unsigned int sequencer,
        unsigned int address,
        unsigned int version,
        double externalClockFrequency,
        uint8_t PLLReferenceMultiplier,
        unsigned int frequencyMultiplier);

    bool CLA_AddDeviceAD9858(
        unsigned int sequencer,
        unsigned int address,
        double externalClockFrequency,
        unsigned int frequencyMultiplier);

    bool CLA_AddDeviceAnalogIn12bit(
        unsigned int sequencer,
        unsigned int chipSelect,
        bool signedValue,
        double minVoltage,
        double maxVoltage);

 */


HControlLightAPI CControlLightAPI::GetInstance() {
    if (CLA_GetInstance)
        return CLA_GetInstance();
    else
        return NULL;
}


bool CControlLightAPI::Create(bool InitializeAfx, bool InitializeAfxSocket) {
    if (CLA_Create)
        return CLA_Create(InitializeAfx, InitializeAfxSocket);
}

void CControlLightAPI::Configure(bool DisplayCommandErrors) {
    if (CLA_Configure)
        CLA_Configure(DisplayCommandErrors);
}

void CControlLightAPI::Call_CLA_Cleanup() {
    if (CLA_Cleanup)
        CLA_Cleanup();
}

const char* CControlLightAPI::GetLastError() {
    if (CLA_GetLastError)
        return CLA_GetLastError();
    else
        return "Error: GetLastError function not loaded.";
}

bool CControlLightAPI::DidErrorOccur() {
    if (CLA_DidErrorOccur)
        return CLA_DidErrorOccur();
    else
        return false;
}

bool CControlLightAPI::LoadFromJSONFile(const char* filename) {
    if (CLA_LoadFromJSONFile)
        return CLA_LoadFromJSONFile(filename);
    else
        return false;
}

void CControlLightAPI::Initialize() {
    if (CLA_Initialize)
        CLA_Initialize();
}

void CControlLightAPI::SwitchDebugMode(bool OnOff, const char* Filename) {
    if (CLA_SwitchDebugMode)
        CLA_SwitchDebugMode(OnOff, Filename);
}

bool CControlLightAPI::IsReady() {
    if (CLA_IsReady)
        return CLA_IsReady();
    else
        return false;
}

void CControlLightAPI::StartAssemblingSequence() {
    if (CLA_StartAssemblingSequence)
        CLA_StartAssemblingSequence();
}

bool CControlLightAPI::SetValue(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetValue)
        return CLA_SetValue(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetRegister(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetRegister)
        return CLA_SetRegister(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetValueSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetValueSerialDevice)
        return CLA_SetValueSerialDevice(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::SetRegisterSerialDevice(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
    if (CLA_SetRegisterSerialDevice)
        return CLA_SetRegisterSerialDevice(Sequencer, Address, SubAddress, Data, DataLength_in_bit, StartBit);
    else
        return false;
}

bool CControlLightAPI::Wait_ms(double time_in_ms) {
    if (CLA_Wait_ms)
        return CLA_Wait_ms(time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTime_ms(double &time_in_ms) {
    if (CLA_GetTime_ms)
        return CLA_GetTime_ms(time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTimeOfSequencer_ms(const unsigned int& Sequencer, double &time_in_ms) {
    if (CLA_GetTimeOfSequencer_ms)
        return CLA_GetTimeOfSequencer_ms(Sequencer, time_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetTimeDebtOfSequencer_ms(const unsigned int& Sequencer, double &time_in_ms) {
    if (CLA_GetTimeDebtOfSequencer_ms)
        return CLA_GetTimeDebtOfSequencer_ms(Sequencer, time_in_ms);
    else
        return false;
}

bool CControlLightAPI::TransmitI2CPort(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t* send_data, uint16_t receive_length, uint8_t* receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently) {
    if (CLA_TransmitI2CPort)
        return CLA_TransmitI2CPort(I2C_port, I2C_destination, I2C_address, send_length, send_data, receive_length, receive_data, I2C_clock_frequency_in_Hz, I2C_success, fail_silently);
    else
        return false;
}

bool CControlLightAPI::SetPSOptions(uint8_t options) {
    if (CLA_SetPSOptions)
        return CLA_SetPSOptions(options);
    else
        return false;
}


/*
API_EXPORT ERROR_CODE_TYPE CLA_SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
		
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

bool CControlLightAPI::SetVoltage(const unsigned int& Sequencer, const unsigned int& Address, double Voltage) {
    if (CLA_SetVoltage)
        return CLA_SetVoltage(Sequencer, Address, Voltage);
    else
        return false;
}

bool CControlLightAPI::SetDigitalOutput(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff) {
    if (CLA_SetDigitalOutput)
        return CLA_SetDigitalOutput(Sequencer, Address, BitNr, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetStartFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetStartFrequency)
        return CLA_SetStartFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetStopFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetStopFrequency)
        return CLA_SetStopFrequency(Sequencer, Address,  Frequency);
    else
        return false;
}

bool CControlLightAPI::SetModulationFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetModulationFrequency)
        return CLA_SetModulationFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetPower(const unsigned int& Sequencer, const unsigned int& Address,  double Power) {
    if (CLA_SetPower)
        return CLA_SetPower(Sequencer, Address,  Power);
    else
        return false;
}

bool CControlLightAPI::SetAttenuation(const unsigned int& Sequencer, const unsigned int& Address,  double Attenuation) {
    if (CLA_SetAttenuation)
        return CLA_SetAttenuation(Sequencer, Address, Attenuation);
    else
        return false;
}

bool CControlLightAPI::SetIOUpdateEnabled(const unsigned int& Sequencer, const unsigned int& Address, bool EnableIOUpdate) {
    if (CLA_SetIOUpdateEnabled)
        return CLA_SetIOUpdateEnabled(Sequencer, Address, EnableIOUpdate);
    else
        return false;
}

bool CControlLightAPI::SetStartFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetStartFrequencyTuningWord)
        return CLA_SetStartFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetStopFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetStopFrequencyTuningWord)
        return CLA_SetStopFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetFSKMode(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode) {
    if (CLA_SetFSKMode)
        return CLA_SetFSKMode(Sequencer, Address, mode);
    else
        return false;
}

bool CControlLightAPI::SetRampRateClock(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate) {
    if (CLA_SetRampRateClock)
        return CLA_SetRampRateClock(Sequencer, Address, rate);
    else
        return false;
}

bool CControlLightAPI::SetClearACC1(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetClearACC1)
        return CLA_SetClearACC1(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetTriangleBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetTriangleBit)
        return CLA_SetTriangleBit(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetFSKBit(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
    if (CLA_SetFSKBit)
        return CLA_SetFSKBit(Sequencer, Address, OnOff);
    else
        return false;
}

bool CControlLightAPI::SetFrequency(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
    if (CLA_SetFrequency)
        return CLA_SetFrequency(Sequencer, Address, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyTuningWord(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
    if (CLA_SetFrequencyTuningWord)
        return CLA_SetFrequencyTuningWord(Sequencer, Address, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency) {
    if (CLA_SetFrequencyOfChannel)
        return CLA_SetFrequencyOfChannel(Sequencer, Address, channel, Frequency);
    else
        return false;
}

bool CControlLightAPI::SetFrequencyTuningWordOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord) {
    if (CLA_SetFrequencyTuningWordOfChannel)
        return CLA_SetFrequencyTuningWordOfChannel(Sequencer, Address, channel, FrequencyTuningWord);
    else
        return false;
}

bool CControlLightAPI::SetPhaseOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase) {
    if (CLA_SetPhaseOfChannel)
        return CLA_SetPhaseOfChannel(Sequencer, Address, channel, Phase);
    else
        return false;
}

bool CControlLightAPI::SetPowerOfChannel(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power) {
    if (CLA_SetPowerOfChannel)
        return CLA_SetPowerOfChannel(Sequencer, Address, channel, Power);
    else
        return false;
}

bool CControlLightAPI::AutoConfigure(const char* filename) {
    if (CLA_AutoConfigure)
        return CLA_AutoConfigure(char*filename);
    else
        return false;
}

void CControlLightAPI::TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(bool OnOff) {
    if (CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible)
        CLA_TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(OnOff);
}

void CControlLightAPI::StartAssemblingNextSequence() {
    if (CLA_StartAssemblingNextSequence)
        CLA_StartAssemblingNextSequence();
}

unsigned int CControlLightAPI::GetNumberOfSequencers() {
    if (CLA_GetNumberOfSequencers)
        return CLA_GetNumberOfSequencers();
    else
        return 0;
}

bool CControlLightAPI::GetNextBufferPositionOfMasterSequencer(unsigned long& next_buffer_position) {
    if (CLA_GetNextBufferPositionOfMasterSequencer)
        return CLA_GetNextBufferPositionOfMasterSequencer(long&next_buffer_position);
    else
        return false;
}

bool CControlLightAPI::SetPeriodicTrigger_ms(double PeriodicTriggerPeriod_in_ms, double PeriodicTriggerAllowedWaitTime_in_ms) {
    if (CLA_SetPeriodicTrigger_ms)
        return CLA_SetPeriodicTrigger_ms(PeriodicTriggerPeriod_in_ms, PeriodicTriggerAllowedWaitTime_in_ms);
    else
        return false;
}

bool CControlLightAPI::GetNextCycleNumber(long& NextCycleNumber) {
    if (CLA_GetNextCycleNumber)
        return CLA_GetNextCycleNumber(long&NextCycleNumber);
    else
        return false;
}

bool CControlLightAPI::ResetCycleNumber() {
    if (CLA_ResetCycleNumber)
        return CLA_ResetCycleNumber();
    else
        return false;
}

bool CControlLightAPI::WriteConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length) {
    if (CLA_WriteConfigEEPROM)
        return CLA_WriteConfigEEPROM(SequencerID, RackNr, SlotNr, char*data, length);
    else
        return false;
}

bool CControlLightAPI::ReadConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool &I2C_success) {
    if (CLA_ReadConfigEEPROM)
        return CLA_ReadConfigEEPROM(SequencerID, RackNr, SlotNr, char*data, size_t&length, I2C_success);
    else
        return false;
}

bool CControlLightAPI::WriteConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address) {
    if (CLA_WriteConfigAddress)
        return CLA_WriteConfigAddress(SequencerID, RackNr, SlotNr, address);
    else
        return false;
}

bool CControlLightAPI::ReadConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool& I2C_success) {
    if (CLA_ReadConfigAddress)
        return CLA_ReadConfigAddress(SequencerID, RackNr, SlotNr, uint8_t&address, bool&I2C_success);
    else
        return false;
}

const char* CControlLightAPI::ReadConfiguration(const char* filename) {
    if (CLA_ReadConfiguration)
        return CLA_ReadConfiguration(char*filename);
    else
        return "";
}

const char* CControlLightAPI::GetAutoConfigJSON(const char* filename) {
    if (CLA_GetAutoConfigJSON)
        return CLA_GetAutoConfigJSON(char*filename);
    else
        return "";
}

bool CControlLightAPI::StartAssemblingCPUCommandSequence() {
    if (CLA_StartAssemblingCPUCommandSequence)
        return CLA_StartAssemblingCPUCommandSequence();
    else
        return false;
}

bool CControlLightAPI::AddCPUCommand(const char* command) {
    if (CLA_AddCPUCommand)
        return CLA_AddCPUCommand(char*command);
    else
        return false;
}

bool CControlLightAPI::ExecuteCPUCommandSequence(unsigned long ethernet_check_period_in_ms) {
    if (CLA_ExecuteCPUCommandSequence)
        return CLA_ExecuteCPUCommandSequence(ethernet_check_period_in_ms);
    else
        return false;
}

bool CControlLightAPI::StopCPUCommandSequence() {
    if (CLA_StopCPUCommandSequence)
        return CLA_StopCPUCommandSequence();
    else
        return false;
}

bool CControlLightAPI::InterruptCPUCommandSequence() {
    if (CLA_InterruptCPUCommandSequence)
        return CLA_InterruptCPUCommandSequence();
    else
        return false;
}

bool CControlLightAPI::GetCPUCommandErrorMessages() {
    if (CLA_GetCPUCommandErrorMessages)
        return CLA_GetCPUCommandErrorMessages();
    else
        return false;
}

bool CControlLightAPI::PrintCPUCommandErrorMessages() {
    if (CLA_PrintCPUCommandErrorMessages)
        return CLA_PrintCPUCommandErrorMessages();
    else
        return false;
}

bool CControlLightAPI::PrintCPUCommandSequence() {
    if (CLA_PrintCPUCommandSequence)
        return CLA_PrintCPUCommandSequence();
    else
        return false;
}

bool CControlLightAPI::Reset(const unsigned int& Sequencer, const unsigned int& Address) {
    if (CLA_Reset)
        return CLA_Reset(int&Sequencer, int&Address);
    else
        return false;
}

void CControlLightAPI::SendSequence(const char* FileName) {
    if (CLA_SendSequence)
        CLA_SendSequence(char*FileName);
}

void CControlLightAPI::RepeatSequence() {
    if (CLA_RepeatSequence)
        CLA_RepeatSequence();
}

bool CControlLightAPI::WaitTillEndOfSequence(double timeout_in_s) {
    if (CLA_WaitTillEndOfSequence)
        return CLA_WaitTillEndOfSequence(timeout_in_s);
    else
        return false;
}

bool CControlLightAPI::SequencerCalcAD9854FrequencyTuningWord(const unsigned int& Sequencer, uint64_t ftw0, uint8_t bit_shift) {
    if (CLA_SequencerCalcAD9854FrequencyTuningWord)
        return CLA_SequencerCalcAD9854FrequencyTuningWord(int&Sequencer, ftw0, bit_shift);
    else
        return false;
}

bool CControlLightAPI::SetSequencerDigitalOut(const unsigned int& Sequencer, uint8_t dig_out_pattern) {
    if (CLA_SetSequencerDigitalOut)
        return CLA_SetSequencerDigitalOut(int&Sequencer, dig_out_pattern);
    else
        return false;
}

bool CControlLightAPI::SetSequencer_PL_to_PS_command(const unsigned int& Sequencer, uint8_t PL_to_PS_command) {
    if (CLA_SetSequencer_PL_to_PS_command)
        return CLA_SetSequencer_PL_to_PS_command(int&Sequencer, PL_to_PS_command);
    else
        return false;
}

bool CControlLightAPI::SwitchSequencerBuzzer(const unsigned int& Sequencer, bool OnOff) {
    if (CLA_SwitchSequencerBuzzer)
        return CLA_SwitchSequencerBuzzer(int&Sequencer, OnOff);
    else
        return false;
}

bool CControlLightAPI::UseEdgeTriggeredLatches(const unsigned int& Sequencer, bool UseEdgeTriggeredLatches) {
    if (CLA_UseEdgeTriggeredLatches)
        return CLA_UseEdgeTriggeredLatches(int&Sequencer, UseEdgeTriggeredLatches);
    else
        return false;
}

bool CControlLightAPI::SequencerSetTimeDebtGuard_in_ms(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms) {
    if (CLA_SequencerSetTimeDebtGuard_in_ms)
        return CLA_SequencerSetTimeDebtGuard_in_ms(int&Sequencer, double&MaxTimeDebt_in_ms);
    else
        return false;
}

bool CControlLightAPI::SequencerSetLoopCount(const unsigned int& Sequencer, unsigned int loop_count) {
    if (CLA_SequencerSetLoopCount)
        return CLA_SequencerSetLoopCount(int&Sequencer, loop_count);
    else
        return false;
}

bool CControlLightAPI::SequencerJumpBackward(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr, bool loop_count_greater_zero) {
    if (CLA_SequencerJumpBackward)
        return CLA_SequencerJumpBackward(int&Sequencer, jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr, loop_count_greater_zero);
    else
        return false;
}

bool CControlLightAPI::SequencerJumpForward(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr) {
    if (CLA_SequencerJumpForward)
        return CLA_SequencerJumpForward(int&Sequencer, jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr);
    else
        return false;
}

bool CControlLightAPI::SequencerTransmitI2C(const unsigned int& Sequencer, uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t* data_out) {
    if (CLA_SequencerTransmitI2C)
        return CLA_SequencerTransmitI2C(int&Sequencer, I2C_port, I2C_length_out, I2C_length_in, uint8_t*data_out);
    else
        return false;
}

bool CControlLightAPI::SequencerTransmitSPI(const unsigned int& Sequencer, uint8_t chip_select, uint16_t number_of_bits_out, const uint8_t* data_out, uint8_t number_of_bits_in, bool start_now) {
    if (CLA_SequencerTransmitSPI)
        return CLA_SequencerTransmitSPI(int&Sequencer, chip_select, number_of_bits_out, uint8_t*data_out, number_of_bits_in, start_now);
    else
        return false;
}

bool CControlLightAPI::SequencerRepeatedOutIn(const unsigned int& Sequencer, uint16_t number_of_datapoints, double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand) {
    if (CLA_SequencerRepeatedOutIn)
        return CLA_SequencerRepeatedOutIn(int&Sequencer, number_of_datapoints, delay_between_datapoints_in_ms, RepeatedOutInCommand);
    else
        return false;
}

bool CControlLightAPI::SequencerSetSPITiming(const unsigned int& Sequencer, uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait) {
    if (CLA_SequencerSetSPITiming)
        return CLA_SequencerSetSPITiming(int&Sequencer, SPI_delay_CS_low_start_wait, SPI_delay_write, SPI_delay_pause_before_read, SPI_delay_read, SPI_delay_CS_low_end_wait);
    else
        return false;
}

bool CControlLightAPI::SequencerSetSPIMode(const unsigned int& Sequencer, uint8_t SPI_mode) {
    if (CLA_SequencerSetSPIMode)
        return CLA_SequencerSetSPIMode(int&Sequencer, SPI_mode);
    else
        return false;
}

bool CControlLightAPI::SequencerSetI2CParameters(const unsigned int& Sequencer, uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read) {
    if (CLA_SequencerSetI2CParameters)
        return CLA_SequencerSetI2CParameters(int&Sequencer, I2C_0_Destination, I2C_delay_start_stop, I2C_delay_data_setup, I2C_delay_clock_high, I2C_delay_clock_low, I2C_delay_pause_before_read);
    else
        return false;
}

bool CControlLightAPI::SelectRackSlot(const unsigned int& Sequencer, uint8_t rack_nr, uint8_t slot_nr) {
    if (CLA_SelectRackSlot)
        return CLA_SelectRackSlot(int&Sequencer, rack_nr, slot_nr);
    else
        return false;
}

bool CControlLightAPI::ResetI2CMultiplexer(const unsigned int& Sequencer) {
    if (CLA_ResetI2CMultiplexer)
        return CLA_ResetI2CMultiplexer(int&Sequencer);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAD9959(unsigned int sequencer, unsigned int address, double externalClockFrequency, unsigned int frequencyMultiplier, bool AD9958) {
    if (CLA_AddDeviceAD9959)
        return CLA_AddDeviceAD9959(sequencer, address, externalClockFrequency, frequencyMultiplier, AD9958);
    else
        return false;
}

//end of convenience functions


void CControlLightAPI::ExecuteSequence(const char* Filename) {
    if (CLA_ExecuteSequence)
        CLA_ExecuteSequence(Filename);
}

bool CControlLightAPI::GetSequenceExecutionStatus(bool& running, unsigned long long& DataPointsWritten) {
    if (CLA_GetSequenceExecutionStatus)
        return CLA_GetSequenceExecutionStatus(running, DataPointsWritten);
    else
        return false;
}

bool CControlLightAPI::WaitTillEndOfSequenceThenGetInputData(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s) {
    if (CLA_WaitTillEndOfSequenceThenGetInputData)
        return CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s);
    else
        return false;
}

void CControlLightAPI::SetTimeDebtGuard_in_ms(const double& MaxTimeDebt_in_ms) {
    if (CLA_SetTimeDebtGuard_in_ms)
        CLA_SetTimeDebtGuard_in_ms(MaxTimeDebt_in_ms);
}

bool CControlLightAPI::SequencerStartAnalogInAcquisition(const unsigned int& Sequencer, const uint8_t& analog_in_type, const uint8_t& SPI_CS, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms) {
    if (CLA_SequencerStartAnalogInAcquisition)
        return CLA_SequencerStartAnalogInAcquisition(Sequencer, analog_in_type, SPI_CS, ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms);
    else
        return false;
}

bool CControlLightAPI::SequencerWriteInputMemory(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
    if (CLA_SequencerWriteInputMemory)
        return CLA_SequencerWriteInputMemory(Sequencer, input_buf_mem_data, write_next_address, input_buf_mem_address);
    else
        return false;
}

bool CControlLightAPI::SequencerWriteSystemTimeToInputMemory(const unsigned int& Sequencer) {
    if (CLA_SequencerWriteSystemTimeToInputMemory)
        return CLA_SequencerWriteSystemTimeToInputMemory(Sequencer);
    else
        return false;
}

bool CControlLightAPI::SequencerSwitchDebugLED(const unsigned int& Sequencer, unsigned int OnOff) {
    if (CLA_SequencerSwitchDebugLED)
        return CLA_SequencerSwitchDebugLED(Sequencer, OnOff);
    else
        return false;
}

bool CControlLightAPI::SequencerIgnoreTCPIP(const unsigned int& Sequencer, bool OnOff) {
    if (CLA_SequencerIgnoreTCPIP)
        return CLA_SequencerIgnoreTCPIP(Sequencer, OnOff);
    else
        return false;
}

bool CControlLightAPI::SequencerAddMarker(const unsigned int& Sequencer, unsigned char marker) {
    if (CLA_SequencerAddMarker)
        return CLA_SequencerAddMarker(Sequencer, marker);
    else
        return false;
}

bool CControlLightAPI::AddDeviceSequencer(
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
    bool useEdgeTriggeredLatches,
    bool connect) {
    if (CLA_AddDeviceSequencer)
        return CLA_AddDeviceSequencer(id, type, ip, port, master, startDelay, clockFrequency, FPGAClockToBusClockRatio, useExternalClock, useStrobeGenerator, useEdgeTriggeredLatches, connect);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAnalogOut16bit(
    unsigned int sequencer,
    unsigned int startAddress,
    unsigned int numberChannels,
    bool signedValue,
    double minVoltage,
    double maxVoltage) {
    if (CLA_AddDeviceAnalogOut16bit)
        return CLA_AddDeviceAnalogOut16bit(sequencer, startAddress, numberChannels, signedValue, minVoltage, maxVoltage);
    else
        return false;
}

bool CControlLightAPI::AddDeviceDigitalOut(
    unsigned int sequencer,
    unsigned int address,
    unsigned int numberChannels) {
    if (CLA_AddDeviceDigitalOut)
        return CLA_AddDeviceDigitalOut(sequencer, address, numberChannels);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAD9854(
    unsigned int sequencer,
    unsigned int address,
    unsigned int version,
    double externalClockFrequency,
    uint8_t PLLReferenceMultiplier,
    unsigned int frequencyMultiplier) {
    if (CLA_AddDeviceAD9854)
        return CLA_AddDeviceAD9854(sequencer, address, version, externalClockFrequency, PLLReferenceMultiplier, frequencyMultiplier);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAD9858(
    unsigned int sequencer,
    unsigned int address,
    double externalClockFrequency,
    unsigned int frequencyMultiplier) {
    if (CLA_AddDeviceAD9858)
        return CLA_AddDeviceAD9858(sequencer, address, externalClockFrequency, frequencyMultiplier);
    else
        return false;
}

bool CControlLightAPI::AddDeviceAnalogIn12bit(
    unsigned int sequencer,
    unsigned int chipSelect,
    bool signedValue,
    double minVoltage,
    double maxVoltage) {
    if (CLA_AddDeviceAnalogIn12bit)
        return CLA_AddDeviceAnalogIn12bit(sequencer, chipSelect, signedValue, minVoltage, maxVoltage);
    else
        return false;
}

