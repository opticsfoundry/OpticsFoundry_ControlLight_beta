// ControlLight.cpp : Defines the entry point for the application.
//

//#include "std.h"



#include "ControlAPI.h"
#include <iostream>


using namespace std;

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


void ControlMessageBox(const CString& lpszText, UINT nType = MB_OK, UINT nIDHelp = 0) {
#ifdef _AFXDLL
	if (DisplayErrors) AfxMessageBox(_T(lpszText), nType, nIDHelp);
#else
	if (DisplayErrors) MessageBox(NULL, lpszText, L"Control DLL", nType);
#endif
}


//if you use Namespace CLA, you need to put CLA:: in front of all DLL function names; you can then remove the CLA_ prefix from all functions

bool LoadControlHardwareInterface() {
	bool ControlHardwareInterfaceLoadedSuccessfully = false;
	try {
		bool success = CLA_LoadFromJSONFile("D:\\Florian\\Firefly\\FireflyControl\\ControlLight\\ControlHardwareConfig.json");
		CLA_Initialize();
		if (CLA_IsReady()) {
			if (!success) {
				ControlMessageBox("Warning: Loading of hardware configuration file worked only partially.");
			}
			ControlHardwareInterfaceLoadedSuccessfully = true;
			//AddErrorMessageCString("Hardware configuration file loaded");
		}
		else {
			if (success) {
				ControlMessageBox("Warning: Hardware configuration file did not contain a sequencer with ID=0.");
			}
		}
	}
	catch (...) {
		ControlMessageBox("Error loading hardware configuration file");
	}	
	if (!ControlHardwareInterfaceLoadedSuccessfully) {
		CLA_Cleanup();
		CLA_Create(/* InitializeAfx*/ false, /* InitializeAfxSocket*/ false);
	}
	return ControlHardwareInterfaceLoadedSuccessfully;
}


int main() {
	if (!CLA_Create(/* InitializeAfx*/ true , /* InitializeAfxSocket*/ true)) {
		return 1; // Initialization failed
	}
	if (!LoadControlHardwareInterface()) {
		ControlMessageBox("Error loading hardware configuration file");
		
		CLA_AddDeviceSequencer(0, "OpticsFoundrySequencerV1", "192.168.0.109", 7, true, 0, 100000000, 2000000, false, true, true, true);
		CLA_AddDeviceAnalogOut16bit(0, 24, 4, true, -10, 10);
		CLA_AddDeviceAnalogOut16bit(0, 552, 4, true, -10, 10);
		CLA_AddDeviceDigitalOut(0, 1, 16);
		CLA_AddDeviceDigitalOut(0, 2, 16);
		CLA_AddDeviceAD9854(0, 232, 2, 300000000, 1, 1);
		CLA_AddDeviceAD9858(0, 652, 1200000000, 1);
		CLA_Initialize();
	}
	
	
	
	CLA_SwitchDebugMode(true, "TestSequencer");
	if (!CLA_IsReady()) {
		ControlMessageBox("Not all sequencers connected");
		CLA_Cleanup();
		return -1;
	}
	//test
	uint8_t* buffer = nullptr;
	for (int i = 0; i < 10; i++) {
		cout << "Iteration " << i << ": ";
		CLA_StartAssemblingSequence();

		//start data acquisition. This is an example for a command for which we didn't yet provide a convenience function in the DLL. 
		//In this somewhat convoluted manner one can achieve anything without API interface changes
		/*
		uint8_t ChannelNumber = 0;
		uint32_t NumberOfDataPoints = 1000;
		double DelayBetweenDataPoints_in_ms_in_ms = 0.02;
		CLA_SetValueSerialDevice(0, 0, 0, (uint8_t*)&ChannelNumber, 8);
		CLA_SetValueSerialDevice(0, 0, 1, (uint8_t*)&NumberOfDataPoints, 32);
		CLA_SetValueSerialDevice(0, 0, 2, (uint8_t*)&DelayBetweenDataPoints_in_ms_in_ms, 64);
		CLA_SetValueSerialDevice(0, 0, 3, (uint8_t*)&ChannelNumber, 8); //starts the acquisition
		*/

		//this is the same command using a convenience function
		CLA_SequencerStartAnalogInAcquisition(0, 0, 0, 0, 1000, 0.02);
		
		for (int j = 1; j < 100; j++) {
			CLA_SetVoltage(0, 24, 10.0*j/100.0);
			uint16_t data = 0xffff;
			CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
			CLA_Wait_ms(0.1);
			data = 0;
			CLA_SetValue(0, 1, 0, (uint8_t*)&data, 16);
			CLA_Wait_ms(0.1);
			double Frequency = 1000.0 * j / 100.0;
			CLA_SetValue(0, 232, 0, (uint8_t*)&Frequency, 64);
		}
		CLA_Wait_ms(10);
		CLA_ExecuteSequence();
		bool running = false;
		unsigned long long DataPointsWritten = 0;
		CLA_GetSequenceExecutionStatus(running, DataPointsWritten);
		
		unsigned long buffer_length = 0;
		unsigned long EndTimeOfCycle = 0;
		CLA_WaitTillEndOfSequenceThenGetInputData(buffer, buffer_length, EndTimeOfCycle, 10);
		cout << "Buffer length: " << buffer_length << endl;

	}

	CLA_Cleanup();
	return 0;
}


//ToDo:
// Put test code into separate file to have clean header file to give to user
// Pull additional convenience commands for digital out and DDS devices from CDevice.h into the API
// Finish SetRegister and SetValue functions for DDS
//For Python and C++ APIs: throw exceptions instead of returning error codes
// For C: return error codes and messages correctly
//(optional, as we do take care of TimeDebt now): wait for startDelay tick counts to synchronize several sequencers

