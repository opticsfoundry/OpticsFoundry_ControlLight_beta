#include "std.h"
#ifdef WIN32
#define _AFXDLL
#include <afxwin.h>         // MFC core and standard components
#include <afxsock.h>      // MFC socket extensions
#include <afx.h>
#include <atlconv.h>          // For CT2A (CString to std::string)
#endif

#include "include/json.hpp"
using json = nlohmann::json;

#include "ControlAPI.h"
#include "AutoConfig.h"

#include "CDeviceSequencer.h"
#include "CDeviceAnalogOut.h"
#include "CDeviceAnalogIn.h"
#include "CDeviceAD9854.h"
#include "CDeviceDigitalOut.h"
#include "CDeviceAD9858.h"
#include "CDeviceAD9959.h"

#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>      // for uint8_t, uint32_t, etc.

using namespace std;


#define CATCH_MFC_EX_S ;
#define CATCH_MFC_EX_E ;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif




#ifdef PYTHON_API

//Pybind11 is not compatible with MFC, so we need to use a different approach for error handling
//ToDo: either get rid of MFC, or convert MFC exceptions to Python exceptions
//It would be cleaner to get rid of MFC, i.e. use WinSock2 instead of CSocket and std::string instead of CString


#include <afx.h>              // For CException
#include <stdexcept>          // For std::runtime_error
#include <atlconv.h>          // For CT2A (CString to std::string)


//This macro would have to be used at the beginning and the end of each function.
#define CATCH_MFC_EX_S try {


#define CATCH_MFC_EX_E } catch (CException* e) { \
TCHAR errorMsg[512]; \
e->GetErrorMessage(errorMsg, 512); \
e->Delete(); \
throw std::runtime_error(CT2A(errorMsg));  \
}


#endif




#ifdef BUILDING_DLL


//trick to start AfxWinInit in a good way inside the DLL (ChatGPT recommended)

#ifdef WIN32

class CControlLightApp : public CWinApp {
public:
	virtual BOOL InitInstance() {
		return CWinApp::InitInstance();
	}
};

CControlLightApp theApp;  // Static object that initializes MFC

#endif

#endif

bool CLA_InitializeMFC(bool InitializeAfx, bool InitializeAfxSocket) {
#ifdef WIN32
	// Initialize MFC and print and error on failure
//#ifdef USING_DLL  //Debug comment: in the full version of the ControlAPI this is done in the equivalent of CLA_Create()
	if (InitializeAfx) {
		HINSTANCE hInst = GetModuleHandle(nullptr);
		if (!AfxWinInit(hInst, nullptr, ::GetCommandLine(), 0)) {
			std::cerr << "Fatal Error: MFC initialization failed" << std::endl;
			return false;
		}
	}
//#endif
//	//Debug comment: in the full version of the ControlAPI this is done in the equivalent of CLA_Create()
	if (InitializeAfxSocket) {
		if (!AfxSocketInit()) {  //Debug comment: in the full version of the ControlAPI this is done in DLLMain()
			ControlMessageBox("Failed to initialize Windows Sockets.");
			return false;
		}
	}
	//AddErrorMessage("Successfully initialized Windows Sockets.");
#endif
	return true;
}


#ifdef USE_IDLE_TICKER

#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

	std::thread idleThread;
	std::atomic<bool> keepRunning{ true };
	std::atomic<bool> isBusy{ false };
	std::mutex apiMutex;

#define API_LOCK_GUARD std::lock_guard<std::mutex> _guard(apiMutex); isBusy = true
#define API_UNLOCK isBusy = false

	//#define USE_IDLE_TICKER



	void OnIdle() {
		if (isBusy.load()) return;

		if (apiMutex.try_lock()) {
			// Do idle processing
			Sequence.Idle(ActiveDialog);
			apiMutex.unlock();
		}
	}

	// This is the thread function
	void IdleThreadFunc()
	{
		while (keepRunning.load()) {
			OnIdle();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}

	// Call this when the DLL is loaded/initialized
	void StartIdleThread()
	{
		keepRunning = true;
		idleThread = std::thread(IdleThreadFunc);
	}

	// Call this when the DLL is unloaded
	void StopIdleThread()
	{
		keepRunning = false;
		if (idleThread.joinable())
			idleThread.join();
	}

	//Example: Wrap your API entry points like this:
	//API_EXPORT void SomeDllFunction()
	//{
	//	API_LOCK_GUARD
	//	// do stuff...
	//	API_UNLOCK
	//}

#else

#define API_LOCK_GUARD
#define API_UNLOCK 

#endif // USE_IDLE_TICKER

#if defined(BUILDING_DLL) || defined(USING_DLL)
#ifdef WIN32
	BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
		case DLL_PROCESS_ATTACH:
			// Use minimal setup, i.e. don't put anything here, use CLA_Create() if you need to initialize something.
			// Minimal setup: MFC sockets for example
			// AfxSocketInit();  //Debug comment: in the full version of the ControlAPI AfxSocketInit() is called here.
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			//Don't put anything here, use CLA_Cleanup() if you need to clean up
			break;
		}
		return TRUE;
	}
#endif
#endif

	//CControlAPI internal variables, not exported
	static const unsigned int MaxSequencers = 1024;
	CDeviceSequencer* SequencerList[MaxSequencers] = {}; //initized to zero at program start
	CDeviceSequencer* ShortSequencerList[MaxSequencers] = {}; //initized to zero at program start
	CDeviceSequencer* MasterSequencer = nullptr;
	unsigned int NrSequencers = 0;
	std::string ReadConfigurationCache;
	std::string AutoConfigJSONCache;
	unsigned long PCBufferSize_in_bytes = 0;
	std::string ConfigurationName = "";
	double LineFrequency = 0;
	static bool initialized = false;
	static bool created = false;

#ifdef C_API
#ifdef __cplusplus
	extern "C" {
#endif
#else
#ifdef NAMESPACE_CLA
	namespace CLA { //optional, for C++ APIs, use namespace CLA, instead of preceding function names with CLA_.
#endif
#endif


		unsigned long ErrorsDroppedSinceLastRead = 0;
		std::string LastErrorsCache;

		std::string PeekLastErrors() {
			std::string LastErrors = "";
			if (ErrorsDroppedSinceLastRead > 0) {
				LastErrors += "[" + std::to_string(ErrorsDroppedSinceLastRead) +
					" earlier error(s) dropped]\n";
			}
			if (!ErrorListWrapAround) {
				for (int e = 0; e < LastErrorIndex; e++) {
					LastErrors += LastError[e];
					LastErrors += "\n";
				}
			}
			else {
				for (int e = LastErrorIndex; e < MaxLastError; e++) {
					LastErrors += LastError[e];
					LastErrors += "\n";
				}
				for (int e = 0; e < LastErrorIndex; e++) {
					LastErrors += LastError[e];
					LastErrors += "\n";
				}
			}
			return LastErrors;
		}


		//constexpr int MaxLastError = 5;
		std::string LastError[MaxLastError] = {};
		int LastErrorIndex = 0;
		bool ErrorListWrapAround = false;
		bool NewErrorOccured = false;

		ERROR_CODE_TYPE AddErrorMessage(const std::string& error_message, bool dothrow) {
			if (ErrorListWrapAround) ErrorsDroppedSinceLastRead++;
			LastError[LastErrorIndex] = error_message;
			LastErrorIndex++;
			if (LastErrorIndex >= MaxLastError) {
				LastErrorIndex = 0;
				ErrorListWrapAround = true;
			}
			std::string all_error_messages = PeekLastErrors();
			NewErrorOccured = true;
			if (DisplayErrors && dothrow) {
				ControlMessageBox(all_error_messages);
			}
#ifdef THROW_EXCEPTIONS
			if (dothrow) {
				throw CLA_Exception(all_error_messages);
			}
#else 
			return false;
#endif
		}



#ifdef API_CLASS

		ControlLight_API::ControlLight_API(bool InitializeAfx , bool InitializeAfxSocket ) {
			Created = false;
			try {  //happens in class constructor
#ifdef THROW_EXCEPTIONS
				Create(InitializeAfx, InitializeAfxSocket);
				Created = true;
#else
				Created = Create(InitializeAfx, InitializeAfxSocket);
#endif
				SetAutoConfigAPI(this);
			}
			catch (const std::exception& e) {
				AddErrorMessage(std::string("ControlLight_API ctor: ") + e.what(), false);
			}
			catch (...) {
				AddErrorMessage("ControlLight_API ctor: unknown exception", false);
			}
		}

		ControlLight_API::~ControlLight_API() {
			Cleanup();
			SetAutoConfigAPI(nullptr);
		}

		bool ControlLight_API::IsCreated() { 
			return Created; 
		}
#endif

		
		API_EXPORT bool CLA_FNDEF(DidErrorOccur)() {
			return NewErrorOccured;
		}

		

		API_EXPORT const char* CLA_FNDEF(GetLastError)() {
			NewErrorOccured = false;
			LastErrorsCache = PeekLastErrors();
			ErrorListWrapAround = false;
			LastErrorIndex = 0;
			ErrorsDroppedSinceLastRead = 0;
			return LastErrorsCache.c_str();
		}


#ifdef THROW_EXCEPTIONS

#define API_UNLOCK_RETURN_ERROR(success, error_message) \
    API_UNLOCK; \
    if (!success) throw CLA_Exception(error_message); \
	return;

#define RETURN_ERROR(success, error_message) \
    if (!success) throw CLA_Exception(error_message); \
	return success;


#else

#define API_UNLOCK_RETURN_ERROR(success, error_message) \
    bool __ret = (success) ? true : AddErrorMessage(error_message); \
    API_UNLOCK; \
    return __ret;

#define RETURN_ERROR(success, error_message) \
    return (success) ? true : AddErrorMessage(error_message); \

#endif


		


		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(Create)(bool InitializeAfx, bool InitializeAfxSocket) {
			CATCH_MFC_EX_S
				API_LOCK_GUARD;
				if (created) {
					API_UNLOCK_RETURN_ERROR(false, "CLA_Create error: already created.");
				}
				created = true;
				bool success = CLA_InitializeMFC(InitializeAfx, InitializeAfxSocket);
				//if (success) AfxMessageBox("ControlAPI DLL loaded successfully and MFC initialized.");
				//else AfxMessageBox("ControlAPI DLL loaded successfully, but MFC not initialized.");
				for (unsigned int i = 0; i < MaxSequencers; i++) {
					SequencerList[i] = nullptr;
					ShortSequencerList[i] = nullptr;
				}
				PCBufferSize_in_bytes = 128 * 1024 * 1024; // Default buffer size
				ConfigurationName = "DefaultConfigName";
				LineFrequency = 50.0;
				initialized = false;
				DisplayErrors = true;
				API_UNLOCK_RETURN_ERROR(success, "CLA_Create error: CLA_InitializeMFC failed.");
			CATCH_MFC_EX_E
		}


		
		API_EXPORT void CLA_FNDEF(Cleanup)() {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
#ifdef USE_IDLE_TICKER
			StopIdleThread();
#endif 
			initialized = false;
			created = false;
			for (unsigned int i = 0; i < MaxSequencers; i++) {
				if (SequencerList[i]) {
					delete SequencerList[i];
					SequencerList[i] = nullptr;
				}
				ShortSequencerList[i] = nullptr;
			}
			MasterSequencer = nullptr;
			NrSequencers = 0;
			API_UNLOCK;
			CATCH_MFC_EX_E
		}

		
		API_EXPORT void CLA_FNDEF(Configure)(bool _DisplayErrors) {
			API_LOCK_GUARD;
			DisplayErrors = _DisplayErrors;
			API_UNLOCK;
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(Initialize)() {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!created) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_Initialize: ControlAPI not created.");
			}
			if (initialized) {
				API_UNLOCK_RETURN_ERROR(true, "CLA_Initialize: ControlAPI already initialized.");
			}
			initialized = true;
			NrSequencers = 0;
			MasterSequencer = NULL;
			for (unsigned int i = 0; i < MaxSequencers; i++) {
				if (SequencerList[i]) {
					ShortSequencerList[NrSequencers] = SequencerList[i];
					if (SequencerList[i]->IsMaster()) {
						MasterSequencer = SequencerList[i];
					}
					NrSequencers++;
				}
			}
			unsigned long PCBufferSize_in_u64 = PCBufferSize_in_bytes / 8;
			PCBufferSize_in_bytes = PCBufferSize_in_u64 * 8; //to make sure it's a multiple of 8
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->Initialize(PCBufferSize_in_u64);
			}
			bool master_found = MasterSequencer != nullptr;
			API_UNLOCK_RETURN_ERROR(master_found, "CLA_Initialize: no master sequencer found.");
			CATCH_MFC_EX_E
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(IsReady)() {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_IsReady error: not initialized.");
			}
			bool AllConnected = true;
			for (int i = 0; i < NrSequencers; i++) {
				AllConnected &= ShortSequencerList[i]->IsSequencerConnected();
			}
			bool success = AllConnected && (MasterSequencer != nullptr);
			API_UNLOCK_RETURN_ERROR(success, "CLA_IsReady error: not all sequencers connected.");
			CATCH_MFC_EX_E
		}

		
		API_EXPORT void CLA_FNDEF(SwitchDebugMode)(bool OnOff, const char* FileName) {
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->SwitchDebugMode(OnOff, FileName ? FileName : "");
			}
			API_UNLOCK;
		}

		API_EXPORT void CLA_FNDEF(TransmitOnlyDifferenceBetweenCommandSequenceIfPossible)(bool OnOff) {
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->TransmitOnlyDifferenceBetweenCommandSequenceIfPossible(OnOff);
			}
			API_UNLOCK;
		}
		
		API_EXPORT void CLA_FNDEF(StartAssemblingSequence)() {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			CLA_FNDEF(Initialize)();
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->StartAssemblingSequence();
			}
			API_UNLOCK;
			CATCH_MFC_EX_E
		}

		API_EXPORT void CLA_FNDEF(StartAssemblingNextSequence)() {
			CATCH_MFC_EX_S
				API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->StartAssemblingNextSequence();
			}
			API_UNLOCK;
			CATCH_MFC_EX_E
		}

		API_EXPORT void CLA_FNDEF(SendSequence)(const char* FileName) {
			CATCH_MFC_EX_S
				API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->SendSequence(FileName ? FileName : "");
			}
			API_UNLOCK;
			CATCH_MFC_EX_E
		}
		
		API_EXPORT void CLA_FNDEF(RepeatSequence)() {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			MasterSequencer->SendStartSequenceCommand();
			API_UNLOCK;
			CATCH_MFC_EX_E
		}

		API_EXPORT void CLA_FNDEF(ExecuteSequence)(const char* FileName) {
			CATCH_MFC_EX_S
				API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK;
				return;
			}
			for (int i = 0; i < NrSequencers; i++) {
				ShortSequencerList[i]->SendSequence(FileName ? FileName : "");
			}
			MasterSequencer->SendStartSequenceCommand();
			API_UNLOCK;
			CATCH_MFC_EX_E
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(WaitTillEndOfSequence)(double timeout_in_s) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (MasterSequencer) {
				bool ret = MasterSequencer->WaitTillEndOfSequence(timeout_in_s);
				API_UNLOCK_RETURN_ERROR(ret, "CLA_WaitTillEndOfSequence error: Master sequencer error.");
			}
			else {
				API_UNLOCK_RETURN_ERROR(false, "CLA_WaitTillEndOfSequence error: No master sequencer defined.");
			}
			CATCH_MFC_EX_E
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetSequenceExecutionStatus)(bool& running, unsigned long long& DataPointsWritten) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (MasterSequencer) {
				bool ret = MasterSequencer->IsSequenceRunning(running, DataPointsWritten);
				API_UNLOCK_RETURN_ERROR(ret, "CLA_GetSequenceExecutionStatus error: Master sequencer error.");
			}
			else {
				running = false;
				API_UNLOCK_RETURN_ERROR(false, "CLA_GetSequenceExecutionStatus error: No master sequencer defined.");
			}
			CATCH_MFC_EX_E
		}



		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(WaitTillEndOfSequenceThenGetInputData)(uint8_t*& buffer, unsigned long& buffer_length, unsigned long& EndTimeOfCycle, double timeout_in_s) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			//MasterSequencer is initialized to zero at program start, so no need to check if it is initialized
			if (MasterSequencer) {
				bool ret = MasterSequencer->WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, timeout_in_s);
				API_UNLOCK_RETURN_ERROR(ret, "CLA_WaitTillEndOfSequenceThenGetInputData error: Master sequencer error.");
			}
			else
			{
				buffer = nullptr;
				buffer_length = 0;
				EndTimeOfCycle = 0;
				API_UNLOCK_RETURN_ERROR(false, "CLA_WaitTillEndOfSequenceThenGetInputData error: No master sequencer defined.");
			}
			CATCH_MFC_EX_E
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetTimeDebtGuard_in_ms)(const double& MaxTimeDebt_in_ms) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetTimeDebtGuard_in_ms error: ControlAPI not initialized.");
			}
			for (int i = 0; i < NrSequencers; i++) ShortSequencerList[i]->SequencerSetTimeDebtGuard_in_ms(MaxTimeDebt_in_ms);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetTimeDebtGuard_in_ms: error");
			CATCH_MFC_EX_E
		}

		
		inline CDeviceSequencer* GetSequencer(const unsigned int& Sequencer) {
			if (!initialized) return nullptr;
			if (Sequencer >= MaxSequencers) return nullptr;
			return SequencerList[Sequencer];
		}

		inline CDeviceSequencer* GetSequencerBeforeInitialization(const unsigned int& Sequencer) {
			if (Sequencer >= MaxSequencers) return nullptr;
			return SequencerList[Sequencer];
		}
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ResetSequencer)(uint8_t SequencerID) {
		API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(SequencerID);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_ResetSequencer : Invalid sequencer");
			}
			bool ret = sequencer->ResetSequencer();
			if (!ret) {API_UNLOCK_RETURN_ERROR(false, "CLA_ResetSequencer error: sequencer error.");}
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured , "CLA_ResetSequencer: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ResetAllSequencers)() {
		API_LOCK_GUARD;
			bool ret = true;
			for (int i = 0; i < NrSequencers; i++) {
				ret = ShortSequencerList[i]->ResetSequencer();
				if (!ret) {API_UNLOCK_RETURN_ERROR(false, "CLA_ResetAllSequencers error: sequencer error.");}
			}
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured , "CLA_ResetAllSequencers: error");
		}
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerStartAnalogInAcquisition)(const unsigned int& Sequencer, const uint8_t &analog_in_type, const uint8_t &SPI_CS, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerStartAnalogInAcquisition error: Invalid sequencer.");
			}
			sequencer->SequencerStartAnalogInAcquisition(analog_in_type, SPI_CS, ChannelNumber, NumberOfDataPoints, DelayBetweenDataPoints_in_ms);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerStartAnalogInAcquisition: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerWriteSystemTimeToInputMemory)(const unsigned int& Sequencer) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerWriteSystemTimeToInputMemory: Invalid sequencer");
			}
			sequencer->SequencerWriteSystemTimeToInputMemory();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerWriteSystemTimeToInputMemory: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerWriteInputMemory)(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address, unsigned long input_buf_mem_address) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerWriteInputMemory: Invalid sequencer");
			}
			sequencer->SequencerWriteInputMemory(input_buf_mem_data, write_next_address, input_buf_mem_address);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerWriteInputMemory: error");
		}
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerCalcAD9854FrequencyTuningWord)(const unsigned int& Sequencer, uint64_t ftw0, uint8_t bit_shift) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerCalcAD9854FrequencyTuningWord: Invalid sequencer");
			}
			sequencer->SequencerCalcAD9854FrequencyTuningWord(ftw0, bit_shift);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerCalcAD9854FrequencyTuningWord: error");
		}
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSwitchDebugLED)(const unsigned int& Sequencer, unsigned int OnOff) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSwitchDebugLED: Invalid sequencer");
			}
			sequencer->SequencerSwitchDebugLED(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSwitchDebugLED: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetSequencerDigitalOut)(const unsigned int& Sequencer, uint8_t dig_out_pattern) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetSequencerDigitalOut: Invalid sequencer");
			}
			sequencer->SetSequencerDigitalOut(dig_out_pattern);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetSequencerDigitalOut: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetSequencer_PL_to_PS_command)(const unsigned int& Sequencer, uint8_t PL_to_PS_command) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetSequencer_PL_to_PS_command: Invalid sequencer");
			}
			sequencer->SetSequencer_PL_to_PS_command(PL_to_PS_command);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetSequencer_PL_to_PS_command: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SwitchSequencerBuzzer)(const unsigned int& Sequencer, bool OnOff) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SwitchSequencerBuzzer: Invalid sequencer");
			}
			sequencer->SwitchSequencerBuzzer(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SwitchSequencerBuzzer: error");
		}

		 
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerIgnoreTCPIP)(const unsigned int& Sequencer, bool OnOff) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerIgnoreTCPIP: Invalid sequencer");
			}
			sequencer->SequencerIgnoreTCPIP(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerIgnoreTCPIP: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(UseEdgeTriggeredLatches)(const unsigned int& Sequencer, bool UseEdgeTriggeredLatches) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_UseEdgeTriggeredLatches: Invalid sequencer");
			}
			sequencer->UseEdgeTriggeredLatches(UseEdgeTriggeredLatches);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_UseEdgeTriggeredLatches: error");
		}

		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerAddMarker)(const unsigned int& Sequencer, unsigned char marker);
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerAddMarker)(const unsigned int& Sequencer, unsigned char marker) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerAddMarker: Invalid sequencer");
			}
			sequencer->SequencerAddMarker(marker);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerAddMarker: error");
		}

		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetTimeDebtGuard_in_ms)(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms);
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSetTimeDebtGuard_in_ms)(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSetTimeDebtGuard_in_ms: Invalid sequencer");
			}
			sequencer->SequencerSetTimeDebtGuard_in_ms(MaxTimeDebt_in_ms);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSetTimeDebtGuard_in_ms: error");
		}

		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetLoopCount)(const unsigned int& Sequencer, unsigned int loop_count);
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSetLoopCount)(const unsigned int& Sequencer, unsigned int loop_count) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSetLoopCount: Invalid sequencer");
			}
			sequencer->SequencerSetLoopCount(loop_count);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSetLoopCount: error");
		}
		 
		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerJumpBackward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0, bool loop_count_greater_zero = false);
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerJumpBackward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr, bool loop_count_greater_zero) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerJumpBackward: Invalid sequencer");
			}
			sequencer->SequencerJumpBackward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr, loop_count_greater_zero);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerJumpBackward: error");
		}

		//API_EXPORT ERROR_CODE_TYPE CLA_FN(AddCommandJumpForward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0);
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerJumpForward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump, bool condition_0, bool condition_1, bool condition_PS, bool condition_dig_in, uint8_t dig_in_bit_nr) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			//SequencerList is initialized to zero at program start, so no need to check if it is initialized
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerJumpForward: Invalid sequencer");
			}
			sequencer->SequencerJumpForward(jump_length, unconditional_jump, condition_0, condition_1, condition_PS, condition_dig_in, dig_in_bit_nr);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerJumpForward: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerTransmitI2C)(const unsigned int& Sequencer, uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t* data_out) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerTransmitI2C: Invalid sequencer");
			}
			sequencer->SequencerTransmitI2C(I2C_port, I2C_length_out, I2C_length_in, data_out);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerTransmitI2C: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerTransmitSPI)(const unsigned int& Sequencer, uint8_t chip_select, uint16_t number_of_bits_out, const uint8_t* data_out, uint8_t number_of_bits_in, bool start_now) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerTransmitSPI: Invalid sequencer");
			}
			sequencer->SequencerTransmitSPI(chip_select, number_of_bits_out, data_out, number_of_bits_in, start_now);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerTransmitSPI: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerRepeatedOutIn)(const unsigned int& Sequencer, uint16_t number_of_datapoints, double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerRepeatedOutIn: Invalid sequencer");
			}
			sequencer->SequencerRepeatedOutIn(number_of_datapoints, delay_between_datapoints_in_ms, RepeatedOutInCommand);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerRepeatedOutIn: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSetSPITiming)(const unsigned int& Sequencer, uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSetSPITiming: Invalid sequencer");
			}
			sequencer->SequencerSetSPITiming(SPI_delay_CS_low_start_wait, SPI_delay_write, SPI_delay_pause_before_read, SPI_delay_read, SPI_delay_CS_low_end_wait);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSetSPITiming: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSetSPIMode)(const unsigned int& Sequencer, uint8_t SPI_mode) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSetSPIMode: Invalid sequencer");
			}
			sequencer->SequencerSetSPIMode(SPI_mode);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSetSPIMode: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SequencerSetI2CParameters)(const unsigned int& Sequencer, uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SequencerSetI2CParameters: Invalid sequencer");
			}
			sequencer->SequencerSetI2CParameters(I2C_0_Destination, I2C_delay_start_stop, I2C_delay_data_setup, I2C_delay_clock_high, I2C_delay_clock_low, I2C_delay_pause_before_read);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SequencerSetI2CParameters: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SelectRackSlot)(const unsigned int& Sequencer, uint8_t rack_nr, uint8_t slot_nr) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SelectRackSlot: Invalid sequencer");
			}
			sequencer->SequencerSelectRackSlot(rack_nr, slot_nr);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SelectRackSlot: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ResetI2CMultiplexer)(const unsigned int& Sequencer) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_ResetI2CMultiplexer: Invalid sequencer");
			}
			sequencer->SequencerResetI2CMultiplexer();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_ResetI2CMultiplexer: error");
		}


		


		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetRegister)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetRegister: Invalid sequencer");
			}
			sequencer->SetRegister_Sequencer(Address, SubAddress, Data, DataLength_in_bit, StartBit);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetRegister: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetValue)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetValue: Invalid sequencer");
			}
			bool success = sequencer->SetValue_Sequencer(Address, SubAddress, Data, DataLength_in_bit, StartBit);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetValue: error");
		}

		
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetValueSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetValueSerialDevice: Invalid sequencer");
			}
			sequencer->SetValueSerialDevice_Sequencer(Address, SubAddress, Data, DataLength_in_bit, StartBit);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetValueSerialDevice: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetRegisterSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit) {
			API_LOCK_GUARD;
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetRegisterSerialDevice: Invalid sequencer");
			}
			sequencer->SetRegisterSerialDevice_Sequencer(Address, SubAddress, Data, DataLength_in_bit, StartBit);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetRegisterSerialDevice: error");
		}


		inline CDevice* GetParallelBusDevice(const unsigned int& Sequencer, const unsigned int& Address) {
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				return nullptr;
			}
			return sequencer->GetParallelBusDevice(Address);
		}

		inline CDevice* GetSerialBusDevice(const unsigned int& Sequencer, const unsigned int& Address) {
			CDeviceSequencer* sequencer = GetSequencer(Sequencer);
			if (!sequencer) {
				return nullptr;
			}
			return sequencer->GetSerialBusDevice(Address);
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetVoltage)(const unsigned int& Sequencer, const unsigned int& Address, double Voltage) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetVoltage: output not found");
			}
			device->SetVoltage(Voltage);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetVoltage: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetDigitalOutput)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetDigitalOutput: output not found");
			}
			device->SetDigitalOutput(BitNr, OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetDigitalOutput: error");
		}


		//AD9854
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetStartFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetStartFrequency: output not found");
			}
			device->SetStartFrequency(Frequency);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetStartFrequency: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetStopFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetStopFrequency: output not found");
			}
			device->SetStopFrequency(Frequency);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetStopFrequency: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetModulationFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetModulationFrequency: output not found");
			}
			device->SetModulationFrequency(Frequency);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetModulationFrequency: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetPower)(const unsigned int& Sequencer, const unsigned int& Address, double Power) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetPower: output not found");
			}
			device->SetPower(Power);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetPower: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetAttenuation)(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetAttenuation: output not found");
			}
			device->SetAttenuation(Attenuation);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetAttenuation: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetStartFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetStartFrequencyTuningWord: output not found");
			}
			device->SetStartFrequencyTuningWord(FrequencyTuningWord);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetStartFrequencyTuningWord: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetStopFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetStopFrequencyTuningWord: output not found");
			}
			device->SetStopFrequencyTuningWord(FrequencyTuningWord);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetStopFrequencyTuningWord: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFSKMode)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFSKMode: output not found");
			}
			device->SetFSKMode(mode);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFSKMode: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetRampRateClock)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetRampRateClock: output not found");
			}
			device->SetRampRateClock(rate);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetRampRateClock: error");
		}

				API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetClearACC1)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetClearACC1: output not found");
			}
			device->SetClearACC1(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetClearACC1: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetTriangleBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetTriangleBit: output not found");
			}
			device->SetTriangleBit(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetTriangleBit: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFSKBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFSKBit: output not found");
			}
			device->SetFSKBit(OnOff);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFSKBit: error");
		}

		//AD9858
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFrequency: output not found");
			}
			device->SetFrequency(Frequency);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFrequency: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFrequencyTuningWord: output not found");
			}
			device->SetFrequencyTuningWord(FrequencyTuningWord);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFrequencyTuningWord: error");
		}

	


		//AD9959
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(Reset)(const unsigned int& Sequencer, const unsigned int& Address) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_Reset output not found");
			}
			device->Reset();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_Reset: error");
		}
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFrequencyOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency) {
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFrequencyOfChannel: output not found");
			}
			device->SetFrequency(channel, Frequency);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFrequencyOfChannel: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetFrequencyTuningWordOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord)
		{
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetFrequencyTuningWordOfChannel: output not found");
			}
			device->SetFrequencyTuningWord(channel, FrequencyTuningWord);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetFrequencyTuningWordOfChannel: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetPhaseOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase)
		{
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetPhaseOfChannel: output not found");
			}
			device->SetPhase(channel, Phase);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetPhaseOfChannel: error");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetPowerOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power)
		{
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetPowerOfChannel: output not found");
			}
			device->SetPower(channel, Power);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetPowerOfChannel: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetIOUpdateEnabled)(const unsigned int& Sequencer, const unsigned int& Address, bool EnableIOUpdate)
		{
			API_LOCK_GUARD;
			CDevice* device = GetParallelBusDevice(Sequencer, Address);
			if (!device) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_SetIOUpdateEnabled: output not found");
			}
			device->SetIOUpdateEnabled(EnableIOUpdate);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "CLA_SetIOUpdateEnabled: error");
		}

		//End of convenience functions


		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(Wait_ms)(double time_in_ms) {
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_Wait_ms: ControlAPI not initialized.");
			}
			bool ok = true;
			for (int i = 0; i < NrSequencers; i++) {
				ok |= ShortSequencerList[i]->Wait_ms(time_in_ms);
			}
			API_UNLOCK_RETURN_ERROR(ok, "CLA_Wait_ms: time debt > time debt guard");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetTime_ms)(double &time_in_ms) {
			API_LOCK_GUARD;
			if (!initialized) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_GetTime_ms: ControlAPI not initialized.");
			}
			time_in_ms = 0;
			for (int i = 0; i < NrSequencers; i++) {
				double SequencerTime = ShortSequencerList[i]->GetTime_ms();
				if (SequencerTime > time_in_ms) time_in_ms = SequencerTime;
			}
			API_UNLOCK_RETURN_ERROR(true, "No error");
		}

		API_EXPORT unsigned int CLA_FNDEF(GetNumberOfSequencers)() {
			API_LOCK_GUARD;
			unsigned int number_of_sequencers = NrSequencers;
			API_UNLOCK;
			return number_of_sequencers;
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetTimeOfSequencer_ms)(const unsigned int& Sequencer, double& time_in_ms) {
			API_LOCK_GUARD;
			if (!ShortSequencerList[Sequencer]) {
				time_in_ms = 0;
				API_UNLOCK_RETURN_ERROR(false, "GetTimeOfSequencer_ms: output not found");
			}
			time_in_ms = ShortSequencerList[Sequencer]->GetTime_ms();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetTimeOfSequencer_ms: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetTimeDebtOfSequencer_ms)(const unsigned int& Sequencer, double& time_debt_in_ms) {
			API_LOCK_GUARD;
			if (!ShortSequencerList[Sequencer]) {
				time_debt_in_ms = 0;
				API_UNLOCK_RETURN_ERROR(false, "GetTimeDebtOfSequencer_ms: output not found");
			}
			time_debt_in_ms = ShortSequencerList[Sequencer]->GetTimeDebt_ms();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetTimeDebtOfSequencer_ms: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetNextBufferPositionOfMasterSequencer)(unsigned long& next_buffer_position) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				next_buffer_position = 0;
				API_UNLOCK_RETURN_ERROR(false, "GetNextBufferPositionOfMasterSequencer: no master sequencer not found");
			}
			next_buffer_position = MasterSequencer->GetNextBufferPosition();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetNextBufferPositionOfMasterSequencer: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetBufferLength)(const unsigned int& Sequencer, uint32_t& FilledBufferLength, uint32_t& MaxBufferLength) {
			API_LOCK_GUARD;
			if (Sequencer >= MaxSequencers || !ShortSequencerList[Sequencer]) {
				FilledBufferLength = 0;
				MaxBufferLength = 0;
				API_UNLOCK_RETURN_ERROR(false, "GetBufferLength: sequencer not found");
			}
			ShortSequencerList[Sequencer]->GetBufferLength(FilledBufferLength, MaxBufferLength);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetBufferLength: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetPeriodicTrigger_ms)(double PeriodicTriggerPeriod_in_ms, double PeriodicTriggerAllowedWaitTime_in_ms) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "SetPeriodicTrigger: no master sequencer not found");
			}
			MasterSequencer->SetPeriodicTrigger_ms(PeriodicTriggerPeriod_in_ms, PeriodicTriggerAllowedWaitTime_in_ms);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "SetPeriodicTrigger: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetNextCycleNumber)(long& NextCycleNumber) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "GetNextCycleNumber: no master sequencer not found");
			}
			MasterSequencer->GetNextCycleNumber(NextCycleNumber);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetNextCycleNumber: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ResetCycleNumber)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "ResetCycleNumber: no master sequencer not found");
			}
			MasterSequencer->ResetCycleNumber();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "ResetCycleNumber: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(TransmitI2CPort)(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "TransmitI2CPort: no master sequencer not found");
			}
			MasterSequencer->TransmitI2CPort(I2C_port, I2C_destination, I2C_address, send_length, send_data, receive_length, receive_data, I2C_clock_frequency_in_Hz, I2C_success, fail_silently);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "TransmitI2CPort: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(SetPSOptions)(uint8_t options) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "SetPSOptions: no master sequencer not found");
			}
			MasterSequencer->SetPSOptions(options);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "SetPSOptions: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(WriteConfigEEPROM)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "WriteConfigEEPROM: no master sequencer not found");
			}
			::WriteConfigEEPROM(SequencerID, RackNr, SlotNr, data, length);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "WriteConfigEEPROM: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ReadConfigEEPROM)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool& I2C_success) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "ReadConfigEEPROM: no master sequencer not found");
			}
			::ReadConfigEEPROM(SequencerID, RackNr, SlotNr, data, length, I2C_success);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "ReadConfigEEPROM: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(WriteConfigAddress)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "WriteConfigAddress: no master sequencer not found");
			}
			::WriteConfigAddress(SequencerID, RackNr, SlotNr, address);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "WriteConfigAddress: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ReadConfigAddress)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool &I2C_success) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "ReadConfigAddress: no master sequencer not found");
			}
			::ReadConfigAddress(SequencerID, RackNr, SlotNr, address, I2C_success);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "ReadConfigAddress: error");
		}

		API_EXPORT const char* CLA_FNDEF(ReadConfiguration)(const char* filename) {
			API_LOCK_GUARD;
			const std::string output_filename = filename ? filename : "";
			ReadConfigurationCache = ::ReadConfiguration(output_filename).dump(4);
			API_UNLOCK;
			return ReadConfigurationCache.c_str();
		}

		API_EXPORT const char* CLA_FNDEF(GetAutoConfigJSON)(const char* filename) {
			const std::string output_filename = filename ? filename : "";
			AutoConfigJSONCache = ::GetAutoConfigJSON(output_filename).dump(4);
			return AutoConfigJSONCache.c_str();
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(StartAssemblingCPUCommandSequence)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "StartAssemblingCPUCommandSequence: no master sequencer not found");
			}
			MasterSequencer->StartAssemblingCPUCommandSequence();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "StartAssemblingCPUCommandSequence: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddCPUCommand)(const char* command) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "AddCPUCommand: no master sequencer not found");
			}
			if (!command || strlen(command) == 0) {
				API_UNLOCK_RETURN_ERROR(false, "AddCPUCommand: command is empty");
			}
			MasterSequencer->AddCPUCommand(command);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "AddCPUCommand: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(ExecuteCPUCommandSequence)(unsigned long ethernet_check_period_in_ms) {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "ExecuteCPUCommandSequence: no master sequencer not found");
			}
			MasterSequencer->ExecuteCPUCommandSequence(ethernet_check_period_in_ms);
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "ExecuteCPUCommandSequence: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(StopCPUCommandSequence)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "StopCPUCommandSequence: no master sequencer not found");
			}
			MasterSequencer->StopCPUCommandSequence();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "StopCPUCommandSequence: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(InterruptCPUCommandSequence)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "InterruptCPUCommandSequence: no master sequencer not found");
			}
			MasterSequencer->InterruptCPUCommandSequence();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "InterruptCPUCommandSequence: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(GetCPUCommandErrorMessages)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "GetCPUCommandErrorMessages: no master sequencer not found");
			}
			MasterSequencer->GetCPUCommandErrorMessages();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "GetCPUCommandErrorMessages: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(PrintCPUCommandErrorMessages)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "PrintCPUCommandErrorMessages: no master sequencer not found");
			}
			MasterSequencer->PrintCPUCommandErrorMessages();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "PrintCPUCommandErrorMessages: error");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(PrintCPUCommandSequence)() {
			API_LOCK_GUARD;
			if (!MasterSequencer) {
				API_UNLOCK_RETURN_ERROR(false, "PrintCPUCommandSequence: no master sequencer not found");
			}
			MasterSequencer->PrintCPUCommandSequence();
			API_UNLOCK_RETURN_ERROR(!NewErrorOccured, "PrintCPUCommandSequence: error");
		}








#define GET_VALUE(key, default_val) ((device_json.contains(key)) ? device_json[key] : default_val)
#define LOAD_VALUE(target, key, default_val) if (device_json.contains(key)) target = device_json[key]; else target = default_val;


		//We need two functions to add a device: 
		//the one that's exported into the DLL
		//one that creates the object with parameters taken from a json structure, which is used internally when loading a json configuration file.
		//unfortunately the second can't simply call the first, as this could create a problem with API_LOCK_GUARD;


		bool CLA_AddDeviceAD9854FromJSON(const json& device_json) {
			//only called from CLA_FNDEF(LoadFromJSON, which already checked if  CLA_FNDEF(Created() was called
			if (!device_json.contains("Address")) return false;

			unsigned int address = device_json["Address"];
			unsigned int sequencer;
			unsigned int version;
			double externalClockFrequency;
			uint8_t PLLReferenceMultiplier;
			unsigned int frequencyMultiplier;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(version, "Version", 2);
			LOAD_VALUE(externalClockFrequency, "ExternalClockFrequency", 300000000);
			LOAD_VALUE(PLLReferenceMultiplier, "PLLReferenceMultiplier", 1);
			LOAD_VALUE(frequencyMultiplier, "FrequencyMultiplier", 1);

			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceAD9854: Invalid sequencer");
			}
			new CDeviceAD9854(
				SequencerList[sequencer],
				address,
				version,
				externalClockFrequency,
				PLLReferenceMultiplier,
				frequencyMultiplier);
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9854: error on initialization");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceAD9854)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int version,
			double externalClockFrequency,
			uint8_t PLLReferenceMultiplier,
			unsigned int frequencyMultiplier) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceAD9854: Invalid sequencer");
			}
			new CDeviceAD9854(
				SequencerList[sequencer],
				address,
				version,
				externalClockFrequency,
				PLLReferenceMultiplier,
				frequencyMultiplier);
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9854: error on initialization");
		}

		

		bool CLA_AddDeviceAD9858FromJSON(const json& device_json) {
			if (!device_json.contains("Address")) return false;
			unsigned int address = device_json["Address"];
			unsigned int sequencer;
			double externalClockFrequency;
			unsigned int frequencyMultiplier;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(externalClockFrequency, "ClockFrequency", 300000000);
			LOAD_VALUE(frequencyMultiplier, "FrequencyMultiplier", 1);
	
			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceAD9858FromJSON: Invalid sequencer");
			}
			new CDeviceAD9858(
				SequencerList[sequencer],
				address,
				externalClockFrequency,
				frequencyMultiplier);
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9858FromJSON: error on initialization");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceAD9858)(
			unsigned int sequencer,
			unsigned int address,
			double externalClockFrequency,
			unsigned int frequencyMultiplier) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceAD9858: Invalid sequencer");
			}			
			new CDeviceAD9858(
				SequencerList[sequencer],
				address,
				externalClockFrequency,
				frequencyMultiplier);
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9858: error on initialization");
		}

		bool CLA_AddDeviceAD9959FromJSON(const json& device_json) {
			if (!device_json.contains("Address")) return false;
			unsigned int address = device_json["Address"];
			unsigned int sequencer;
			double externalClockFrequency_in_MHz;
			unsigned int frequencyMultiplier;
			unsigned int AD9958;
			double version;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(externalClockFrequency_in_MHz, "ClockFrequencyinMHz", 300000000);
			LOAD_VALUE(frequencyMultiplier, "FrequencyMultiplier", 1);
			LOAD_VALUE(AD9958, "AD9958", 0);
			LOAD_VALUE(version, "Version", 0);

			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceAD9959FromJSON: Invalid sequencer");
			}
			new CDeviceAD9959(
				SequencerList[sequencer],
				address,
				externalClockFrequency_in_MHz,
				frequencyMultiplier,
				AD9958 == 1,
				version);
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9959FromJSON: error on initialization");
		}


		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceAD9959)(
			unsigned int sequencer,
			unsigned int address,
			double externalClockFrequency,
			unsigned int frequencyMultiplier,
			bool AD9958,
			double version) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceAD9959: Invalid sequencer");
			}
			new CDeviceAD9959(
				SequencerList[sequencer],
				address,
				externalClockFrequency,
				frequencyMultiplier,
				AD9958,
				version);
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAD9959: error on initialization");
		}

		bool CLA_AddDeviceDigitalOutFromJSON(const json& device_json) {
			if (!device_json.contains("Address")) return false;
			unsigned int address = device_json["Address"];
			unsigned int sequencer;
			unsigned int numberChannels;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(numberChannels, "NumberChannels", 16);

			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceDigitalOutFromJSON: Invalid sequencer");
			}
			new CDeviceDigitalOut(
				SequencerList[sequencer],
				address,
				numberChannels);
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceDigitalOutFromJSON: error on initialization");

		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceDigitalOut)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int numberChannels) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceDigitalOut: Invalid sequencer");
			}
			new CDeviceDigitalOut(
				SequencerList[sequencer],
				address,
				numberChannels);
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceDigitalOut: error on initialization");
		}

		

		bool CLA_AddDeviceAnalogIn12bitFromJSON(const json& device_json) {
			unsigned int sequencer;
			unsigned int chipSelect;
			bool signedValue;
			double minVoltage;
			double maxVoltage;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(chipSelect, "ChipSelect", 1);
			LOAD_VALUE(signedValue, "Signed", false);
			LOAD_VALUE(minVoltage, "MinVoltage", 0);
			LOAD_VALUE(maxVoltage, "MaxVoltage", 10);
			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceAnalogIn12bitFromJSON: Invalid sequencer");
			}
			new CDeviceAnalogIn(
				SequencerList[sequencer],
				chipSelect,
				signedValue,
				minVoltage,
				maxVoltage);
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAnalogIn12bitFromJSON: error on initialization");
		}

		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceAnalogIn12bit)(
			unsigned int sequencer,
			unsigned int chipSelect,
			bool signedValue,
			double minVoltage,
			double maxVoltage) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceAnalogIn12bit: Invalid sequencer");
			}
			new CDeviceAnalogIn(
				SequencerList[sequencer],
				chipSelect,
				signedValue,
				minVoltage,
				maxVoltage);
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAnalogIn12bit: error on initialization");
		}


		bool CLA_AddDeviceAnalogOut16bitFromJSON(const json& device_json) {
			if (!device_json.contains("StartAddress")) return false;
			unsigned int startAddress = device_json["StartAddress"];
			unsigned int sequencer;
			unsigned int numberChannels;
			bool signedValue;
			double minVoltage;
			double maxVoltage;
			LOAD_VALUE(sequencer, "Sequencer", 0);
			LOAD_VALUE(numberChannels, "NumberChannels", 4);
			LOAD_VALUE(signedValue, "Signed", true);
			LOAD_VALUE(minVoltage, "MinVoltage", -10);
			LOAD_VALUE(maxVoltage, "MaxVoltage", 10);
			if (!GetSequencerBeforeInitialization(sequencer)) {
				RETURN_ERROR(false, "CLA_AddDeviceAnalogOut16bitFromJSON: Invalid sequencer");
			}
			for (unsigned int i = startAddress; i < startAddress + numberChannels; i++) {
				new CDeviceAnalogOut(
					SequencerList[sequencer],
					i,
					signedValue,
					minVoltage,
					maxVoltage);
			}
			RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAnalogOut16bitFromJSON: error on initialization");
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceAnalogOut16bit)(
			unsigned int sequencer,
			unsigned int startAddress,
			unsigned int numberChannels,
			bool signedValue,
			double minVoltage,
			double maxVoltage) {
			API_LOCK_GUARD;
			if (!GetSequencerBeforeInitialization(sequencer)) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceAnalogOut16bit: Invalid sequencer");
			}
			for (unsigned int i = startAddress; i < startAddress + numberChannels; i++) {
				new CDeviceAnalogOut(
					SequencerList[sequencer],
					i,
					signedValue,
					minVoltage,
					maxVoltage);
			}
			API_UNLOCK_RETURN_ERROR(!SequencerList[sequencer]->ErrorOccured(), "CLA_AddDeviceAnalogOut16bit: error on initialization");
		}

		bool CLA_AddDeviceSequencerFromJSON(const json& device_json) {
			if (!device_json.contains("IP")) return false;
			std::string ip = device_json["IP"];
			unsigned int id;
			std::string type;
			unsigned int port;
			bool master;
			unsigned int startDelay;
			double clockFrequency;
			unsigned long StrobeDurationInFPGAClockPeriods;
			bool useExternalClock;
			bool useStrobeGenerator;
			bool useEdgeTriggeredLatches;
			bool connect;
			LOAD_VALUE(id, "Id", 0);
			LOAD_VALUE(type, "Type", "OpticsFoundrySequencerV1");
			LOAD_VALUE(port, "Port", 7);
			LOAD_VALUE(master, "Master", true);
			LOAD_VALUE(startDelay, "StartDelay", 0);
			LOAD_VALUE(clockFrequency, "ClockFrequency", 100000000);
			LOAD_VALUE(StrobeDurationInFPGAClockPeriods, "StrobeDurationInFPGAClockPeriods", 10);
			LOAD_VALUE(useExternalClock, "UseExternalClock", false);
			LOAD_VALUE(useStrobeGenerator, "UseStrobeGenerator", true);
			LOAD_VALUE(useEdgeTriggeredLatches, "UseEdgeTriggeredLatches", true);
			LOAD_VALUE(connect, "Connect", true);

			if (!created) {
				RETURN_ERROR(false, "CLA_AddDeviceSequencerFromJSON: ControlAPI not created.");
			}
			if (SequencerList[id]) {
				RETURN_ERROR(false, "CLA_AddDeviceSequencerFromJSON: sequencer id already in use.");
			}
			// Create a new device sequencer and add it to the list
			SequencerList[id] = new CDeviceSequencer(id, type, ip, port, master, startDelay, clockFrequency, StrobeDurationInFPGAClockPeriods, useExternalClock, useStrobeGenerator, useEdgeTriggeredLatches, connect);
			RETURN_ERROR(!SequencerList[id]->ErrorOccured(), "CLA_AddDeviceSequencerFromJSON: error on initialization");
		}

			
		
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AddDeviceSequencer)(
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
			bool connect) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!created) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceSequencer: ControlAPI not created.");
			}
			if (SequencerList[id]) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AddDeviceSequencer: sequencer id already in use.");
			}
			// Create a new device sequencer and add it to the list
			SequencerList[id] = new CDeviceSequencer(id, type, ip, port, master, startDelay, clockFrequency, StrobeDurationInFPGAClockPeriods, useExternalClock, useStrobeGenerator, useEdgeTriggeredLatches, connect);
			API_UNLOCK_RETURN_ERROR(!SequencerList[id]->ErrorOccured(), "CLA_AddDeviceSequencer: error on initialization");
			CATCH_MFC_EX_E
		}




		//#define GET_VALUE(key, default_val) ((device_json.contains(key)) ? device_json[key].get<decltype(default_val)>() : default_val)
#define GET_CONFIG_VALUE(key, default_val) ((config.contains(key)) ? config[key] : default_val)
#define LOAD_CONFIG_VALUE(target, key, default_val) if (config.contains(key)) target = config[key]; else target = default_val;

		bool CLA_LoadFromJSON(const json& config) {
			if (!created) return false;
			/*std::string test;
			try {
				test = config["ConfigurationName"];
			}
			catch (json::exception& e) {
				AddErrorMessage("JSON parse error");
				return false;
			}*/



			ConfigurationName = GET_CONFIG_VALUE("ConfigurationName", "DefaultConfigName");
			LOAD_CONFIG_VALUE(PCBufferSize_in_bytes, "PCBufferSize_in_bytes", 128 * 1024 * 1024);
			LOAD_CONFIG_VALUE(LineFrequency, "LineFrequency", 50.0);

			//AddSequencers
			bool success = true;
			if (config.contains("Sequencers")) {
				for (const auto& device_json : config["Sequencers"])
					success &= CLA_AddDeviceSequencerFromJSON(device_json);
			}
			//Add racks
			if (config.contains("Rack")) {
				for (const auto& device_json : config["Rack"])
					success &= CLA_AddDeviceAD9854FromJSON(device_json);
			}
			//Add 16-bit analog out boards
			if (config.contains("AnalogOutBoards16bit")) {
				for (const auto& device_json : config["AnalogOutBoards16bit"])
					success &= CLA_AddDeviceAnalogOut16bitFromJSON(device_json);
			}
			//Add digital out boards
			if (config.contains("DigitalOutBoards")) {
				for (const auto& device_json : config["DigitalOutBoards"])
					success &= CLA_AddDeviceDigitalOutFromJSON(device_json);
			}
			//Add AD9854 DDS boards
			if (config.contains("DDSAD9854Boards")) {
				for (const auto& device_json : config["DDSAD9854Boards"])
					success &= CLA_AddDeviceAD9854FromJSON(device_json);
			}
			//Add AD9858 DDS boards
			if (config.contains("DDSAD9858Boards")) {
				for (const auto& device_json : config["DDSAD9858Boards"])
					success &= CLA_AddDeviceAD9858FromJSON(device_json);
			}
			//Add AD9959 DDS boards
			if (config.contains("DDSAD9959Boards")) {
				for (const auto& device_json : config["DDSAD9959Boards"])
					success &= CLA_AddDeviceAD9959FromJSON(device_json);
			}
			//Add 12-bit Analog in boards
			if (config.contains("AnalogInBoards12bit")) {
				for (const auto& device_json : config["AnalogInBoards12bit"])
					success &= CLA_AddDeviceAnalogIn12bitFromJSON(device_json);
			}
			return success;
		}


		/// @brief Load a configuration from a JSON file.
		/// @param filename the name of the file to load the configuration from.
		/// @return
		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(LoadFromJSONFile)(const char* filename) {
			CATCH_MFC_EX_S
			API_LOCK_GUARD;
			if (!created) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_LoadFromJSONFile: ControlAPI not created.");
			}
			std::ifstream file(filename);
			if (!file) {
				std::string help(filename);
				help = "CLA_LoadFromJSONFile: Failed to open config file " + help;
				//AddErrorMessage(help);
				API_UNLOCK_RETURN_ERROR(false, help);
			}

			json config;
			try {
				file >> config;
			}
			catch (json::parse_error& e) {
				//AddErrorMessage("JSON parse error");
				API_UNLOCK_RETURN_ERROR(false, "CLA_LoadFromJSONFile: JSON parse error");
			}
			bool ret = CLA_LoadFromJSON(config);
			API_UNLOCK_RETURN_ERROR(ret, "CLA_LoadFromJSONFile: Could not load from JSON");
			CATCH_MFC_EX_E
		}

		API_EXPORT ERROR_CODE_TYPE CLA_FNDEF(AutoConfigure)(const char* filename) {
			CATCH_MFC_EX_S
			const std::string output_filename = filename ? filename : "";
			json config = ::GetAutoConfigJSON(output_filename);

			API_LOCK_GUARD;
			if (!created) {
				API_UNLOCK_RETURN_ERROR(false, "CLA_AutoConfigure: ControlAPI not created.");
			}
			bool ret = CLA_LoadFromJSON(config);
			API_UNLOCK_RETURN_ERROR(ret, "CLA_AutoConfigure: Could not load from auto-config JSON");
			CATCH_MFC_EX_E
		}


#ifdef C_API
#ifdef __cplusplus
}
#endif
#else
#ifdef NAMESPACE_CLA
	} //option namespace CLA
#endif
#endif
