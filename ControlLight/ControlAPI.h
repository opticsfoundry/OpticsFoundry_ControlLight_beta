#pragma once



//#ifndef USING_DLL
#ifdef WIN32
#define _AFXDLL
#include <afxwin.h>         // MFC core and standard components
#endif
#include <fstream>
//bool InitializeMFC();
//#endif

#include <cstdint>      // for uint8_t, uint32_t, etc.

#ifdef PYTHON_API
#undef C_API
#define THROW_EXCEPTIONS
#define API_CLASS
#define BUILDING_DLL
#endif

#ifdef API_CLASS
#undef C_API
#endif

#ifdef C_API
#ifdef __cplusplus
extern "C" {
#endif
#else
#ifdef NAMESPACE_CLA
namespace CLA { //optional, for C++ APIs, use namespace CLA, instead of preceding function names with CLA_.
#endif
#endif

#define API_EXPORT
#define API_EXPORT_CLASS


#ifdef API_CLASS

#define API_EXPORT

#ifdef BUILDING_DLL
#ifdef _WIN32
#define API_EXPORT_CLASS __declspec(dllexport)
#else
#define API_EXPORT_CLASS
#endif
#endif

#ifdef USING_DLL 
#ifdef _WIN32
#define API_EXPORT_CLASS __declspec(dllimport)
#else
#define API_EXPORT_CLASS
#endif
#endif

#else

#define API_EXPORT_CLASS
#ifdef BUILDING_DLL
#ifdef _WIN32
#define API_EXPORT __declspec(dllexport)
#else
#define API_EXPORT
#endif
#endif

#ifdef USING_DLL 
#ifdef _WIN32
#define API_EXPORT __declspec(dllimport)
#else
#define API_EXPORT
#endif
#endif

#endif



#ifdef THROW_EXCEPTIONS
#define ERROR_CODE_TYPE void

#include <stdexcept>

	class CLA_Exception : public std::runtime_error {
	public:
		explicit CLA_Exception(const std::string& message)
			: std::runtime_error(message) {}
	};

#else 
#define ERROR_CODE_TYPE bool
#endif

	constexpr int MaxLastError = 10;
	extern std::string LastError[MaxLastError];
	extern int LastErrorIndex;
	extern bool ErrorListWrapAround;
	static bool DisplayErrors = true;

	ERROR_CODE_TYPE AddErrorMessage(const std::string& error_message, bool dothrow = true);


	#if !defined(BUILDING_DLL) && defined(USING_DLL)
	#define API_EXPORT 

			//InitilizeMFC() is needed when using this code directly in an exe, not through a DLL. 
			//If you don't have MFC already initialized, you need to call this function, or a similar one adapted to your purposes.
			//This is not exported to the DLL, because it is automatically called in the DLLMain function when the DLL is loaded.
			//extern bool CLA_InitializeMFC();

	#endif



	#if defined(BUILDING_DLL) || defined(USING_DLL)


			/*
			class CControlApp : public CObject
			{
			public:
				CControlApp();
				virtual ~CControlApp();
				bool Initialize();
				HINSTANCE m_hInstance;
			};
			*/


			//typedef void* HControlLightAPI;
	#ifdef BUILDING_DLL
	#ifdef WIN32
			BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID);
	#endif
	#endif
			
	#endif



#ifdef API_CLASS

#define CLA_FN(name) name
#define CLA_FNDEF(name) ControlLight_API::##name

	class API_EXPORT_CLASS ControlLight_API {
	public:
		bool Created;
		ControlLight_API(bool InitializeAfx = true, bool InitializeAfxSocket = true);
		~ControlLight_API();
		bool IsCreated();

#else
#define CLA_FN(name) CLA_##name
#define CLA_FNDEF(name) CLA_##name
#endif



		//API_EXPORT HControlLightAPI CLA_GetInstance();

		//Call these functions in roughly this order
		/**
		* @brief Threading model.
		*
		* The ControlLight API is intended to be used from one client thread at a time.
		* Concurrent calls into the same API instance are unsupported, including calls made
		* while another thread is waiting for sequence completion or communicating with the
		* FPGA. Client code must serialize API access externally. The API may report an
		* error or behave unpredictably if it is entered concurrently.
		*/

		/**
		* @brief Return convention.
		*
		* Functions returning ERROR_CODE_TYPE return true on success and false on failure
		* in the C API build. In the Python/C++ exception build, ERROR_CODE_TYPE is void
		* and failures are reported by throwing CLA_Exception. Functions returning void
		* perform the requested action and report only through the API error state where
		* noted. Functions returning const char* return pointers owned by the API; callers
		* must copy the string if they need it after the next API call.
		*/

		/** @brief Initializes the ControlAPI.
		 * If you use a bare function C API, then this function must be called before using any other functions in the API.
		* If you didn't load MFC yet (which is the case if you use Qt), then set
		* @param InitializeAfx to true to initialize MFC core
		* @param InitializeAfxSocket to true to initialize MFC socket layer
		* The class wrapped version of this API calls this function in its constructor.
		*/
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Create)(bool InitializeAfx, bool InitializeAfxSocket); //you must call this first, otherwise the API will not work

		/**  @brief This function must be called when you are finished using the API.
		* The class wrapped version of this API calls this function in its destructor.
		*/
		/// @return See return convention above.
		API_EXPORT void CLA_FN(Cleanup)(); //You must call this before leaving your program, otherwise the API can provoke errors because memory is not freed

		/// @brief Check if an error has occurred since the last call to GetLastError
		/// @return true if an error has occurred since the last call to GetLastError
		API_EXPORT bool CLA_FN(DidErrorOccur)();

		/// @brief returns the last error messages
		/// @return See return convention above.
		API_EXPORT const char* CLA_FN(GetLastError)();

		/// @brief Configures if the ControlAPI should display error messages.
		/// @param  _DisplayErrors true to display error messages, false to suppress them.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(Configure)(bool _DisplayErrors); //optional

		/**@brief Loads a configuration from a JSON file.
		* Before you can use the control system, you either must read a json configuration file, or define the devices in the API.
		*/
		///@param filename the name of the json file to load.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(LoadFromJSONFile)(const char* filename);

		/// @brief Read the rack auto-configuration, convert it to the standard config schema, and load it.
		/// @param filename Optional base filename used for the generated output files. Pass an empty string to avoid writing files.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AutoConfigure)(const char* filename = "");

		//Once all devices have been declared, you must initialize the system, otherwise the API will not work
		/// @brief After Configuring the hardware, e.g. by loading a json configuration file, this function must be called to initialize the hardware.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Initialize)();
		//Some optional commands

		/**  @brief Switches FPGA debug mode on.
		 * In debug mode, the FPGA sequencers display more information on their USB-UART port, being slowed down a bit by that.
		 * To write a human-readable sequence file, pass a filename to SendSequence instead.
		*/
		/// @param OnOff true to switch on debug mode, false to switch it off.
		/// @param FileName retained for compatibility and ignored.
		API_EXPORT void CLA_FN(SwitchDebugMode)(bool OnOff, const char* FileName);


		/**  @brief Transmits only changes of sequence over TCP/IP to sequencer in order to safe time.
		 * Once a sequence is in the sequencer's memory, the next sequence is compared to the one in memory and only the changes are transmitted over TCP/IP, if this results in less transmission data.
		*/
		/// @param OnOff true to switch on debug mode, false to switch it off.
		API_EXPORT void CLA_FN(TransmitOnlyDifferenceBetweenCommandSequenceIfPossible)(bool OnOff);

		/// @brief Checks if the ControlAPI is ready to be used.
		/// @param IsReady true if the ControlAPI is ready to be used, false otherwise.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(IsReady)();

		//To start a sequence, first call StartAssemblingSequence, then add all the commands you want to execute in the sequence

		/// @brief Starts assembling the first sequence in the sequency buffer. Clears any previous sequence. Must be called before adding commands to the sequence.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(StartAssemblingSequence)();

		/// @brief Starts assembling the second or higher sequence in the sequence buffer. Clears any previous sequence. Must be called before adding commands to the sequence.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(StartAssemblingNextSequence)();

		//here are possible commands
		/**  @brief This function sets a register for a device on the sequencer.
		* What this means depends on the device type. This function gives us an easy way to add new functionality to a device without having to programm new DLL functions.
		* SetRegister maps to the register map given in the device's datasheet. Note: this is not currently implemented in the API.
		* @param Sequencer the sequencer to use.
		* @param Address the address of the device to set the value for.
		* @param SubAddress the subaddress of the device to set the value for.
		* @param Data the data to set.
		* @param DataLength_in_bit the length of the data in bits.
		* @param StartBit the start bit of the data to set.
		*/
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRegister)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);

		/**  @brief This function sets a value for a device on the sequencer.
		* What this means depends on the device type. This function gives us an easy way to add new functionality to a device without having to programm new DLL functions.
		* In contrast to SetRegister, SetValue can execute more complex operations, such as calculating a DDS frequency tuning word from the given frequency and then programming that.
		* There can be several SetValue functions for the same SetRegister function.
		* There can be SetValue functions that set some parameter, or that trigger some action, not even necessarily related to the registers of the device.
		* @param Sequencer the sequencer to use.
		* @param Address the address of the device to set the value for.
		* @param SubAddress the subaddress of the device to set the value for.
		* @param Data the data to set.
		* @param DataLength_in_bit the length of the data in bits.
		* @param StartBit the start bit of the data to set.
		*/
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetValue)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief As SetValue, but for serial devices.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the value for.
		/// @param SubAddress the subaddress of the device to set the value for.
		/// @param Data the data to set.
		/// @param DataLength_in_bit the length of the data in bits.
		/// @param StartBit the start bit of the data to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetValueSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief As SetRegister, but for serial devices.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the value for.
		/// @param SubAddress the subaddress of the device to set the value for.
		/// @param Data the data to set.
		/// @param DataLength_in_bit the length of the data in bits.
		/// @param StartBit the start bit of the data to set.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRegisterSerialDevice)(const unsigned int& Sequencer, const unsigned int& Address, const unsigned int& SubAddress, const uint8_t* Data, const unsigned long& DataLength_in_bit, const uint8_t& StartBit = 0);
		
		/// @brief Wait for a given time in ms.
		/// @param time_in_ms the time to wait in ms.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Wait_ms)(double time_in_ms);

		/// @brief Get the current time in the currently assembled sequence in ms.
		/// @param time_in_ms the current time in ms.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetTime_ms)(double& time_in_ms);

		/// @brief Get the number of configured sequencers.
		/// @return The current number of sequencers known to the API.
		API_EXPORT unsigned int CLA_FN(GetNumberOfSequencers)();

		/// @brief Get the current time of a specific sequencer in the currently assembled sequence in ms.
		/// @param Sequencer the sequencer to use.
		/// @param time_in_ms the current time in ms.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetTimeOfSequencer_ms)(const unsigned int& Sequencer, double& time_in_ms);
		
		/// @brief Get the time debt of a specific sequencer in the currently assembled sequence in ms.
		/// @param Sequencer the sequencer to use.
		/// @param time_debt_in_ms output parameter receiving the amount by which this sequencer is ahead of the master sequence time, in ms.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetTimeDebtOfSequencer_ms)(const unsigned int& Sequencer, double& time_debt_in_ms);

		/// @brief Get the position of the next empty buffer slot of master sequencer in the currently assembled sequence.
		/// @param next_buffer_position output parameter receiving the next writable command-buffer slot as unsigned long / uint32_t.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetNextBufferPositionOfMasterSequencer)(unsigned long& next_buffer_position);

		/// @brief Set the periodic trigger timing used by the master sequencer.
		/// @param PeriodicTriggerPeriod_in_ms the period of the periodic trigger in ms.
		/// @param PeriodicTriggerAllowedWaitTime_in_ms the maximum time in ms the master sequencer may wait for the next periodic trigger before reporting an error.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPeriodicTrigger_ms)(double PeriodicTriggerPeriod_in_ms, double PeriodicTriggerAllowedWaitTime_in_ms);
		
		/// @brief Get next cycle number of master sequencer.
		/// @param NextCycleNumber the next cycle number.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetNextCycleNumber)(long& NextCycleNumber);

		/// @brief Reset cycle number of master sequencer to 0.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ResetCycleNumber)();

		/// @brief Read data from an I2C port.
		/// @param I2C_port the I2C port to read from.
		/// @param I2C_destination If I2C_port == 0, 0 means port connected to rack configuration I2C channel, 1 means port connected to z-turn I2C.
		/// @param I2C_address the I2C address to read from.
		/// @param send_length the length of the data to send in bytes.
		/// @param send_data the data to send.
		/// @param receive_length the length of the data to receive in bytes.
		/// @param receive_data the buffer to store the received data.
		/// @param I2C_clock_frequency_in_Hz the I2C clock frequency in Hz.
		/// @param I2C_success output flag set by the sequencer to indicate whether the I2C transaction was acknowledged and completed.
		/// @param fail_silently true to suppress API error reporting for an I2C failure while still updating I2C_success.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(TransmitI2CPort)(uint8_t I2C_port, uint8_t I2C_destination, uint8_t I2C_address, uint16_t send_length, uint8_t *send_data, uint16_t receive_length, uint8_t *receive_data, uint32_t I2C_clock_frequency_in_Hz, bool& I2C_success, bool fail_silently);

		/// @brief Set PS option flags on the master sequencer controller.
		/// @param options option bitmask passed through to the controller firmware.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPSOptions)(uint8_t options);

		/// @brief Write configuration data to the EEPROM of a rack slot.
		/// @param SequencerID the sequencer to use.
		/// @param RackNr the rack number.
		/// @param SlotNr the slot number within the rack.
		/// @param data the configuration payload to write.
		/// @param length the payload length in bytes.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(WriteConfigEEPROM)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length);

		/// @brief Read the complete configuration EEPROM of a rack slot.
		/// @param SequencerID the sequencer to use.
		/// @param RackNr the rack number.
		/// @param SlotNr the slot number within the rack.
		/// @param data output buffer receiving the EEPROM contents.
		/// @param length input: available buffer size in bytes, output: number of bytes read.
		/// @param I2C_success output flag indicating whether the EEPROM read transaction succeeded.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ReadConfigEEPROM)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool &I2C_success);

		/// @brief Write the configuration address byte of a rack slot.
		/// @param SequencerID the sequencer to use.
		/// @param RackNr the rack number.
		/// @param SlotNr the slot number within the rack.
		/// @param address the address byte to write.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(WriteConfigAddress)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address);

		/// @brief Read the configuration address byte of a rack slot.
		/// @param SequencerID the sequencer to use.
		/// @param RackNr the rack number.
		/// @param SlotNr the slot number within the rack.
		/// @param address output byte receiving the current address value.
		/// @param I2C_success output flag indicating whether the address read transaction succeeded.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ReadConfigAddress)(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool& I2C_success);

		/// @brief Read the rack auto-configuration and return it as JSON text.
		/// @param filename Optional output filename. Pass an empty string to avoid writing a file.
		/// @return JSON text containing the discovered configuration.
		API_EXPORT const char* CLA_FN(ReadConfiguration)(const char* filename = "");

		/// @brief Read the rack auto-configuration, convert it to the standard config schema, and return it as JSON text.
		/// @param filename Optional base filename used for the generated output files. Pass an empty string to avoid writing files.
		/// @return JSON text containing the generated configuration.
		API_EXPORT const char* CLA_FN(GetAutoConfigJSON)(const char* filename = "");

		//The following functions enable you to assemble a CPU command sequence on the master sequencer, which can then be executed by the CPU.
		//These command sequences can start FPGA command sequence, analyze acquired data, modify the FPGA command sequence and repeat.
		//This enables for example: digital PIDs, digital VCOs,...

		/// @brief Start assembling a CPU command sequence on the master sequencer
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(StartAssemblingCPUCommandSequence)();

		/// @brief Add a CPU command to the currently assembled CPU command sequence on the master sequencer.
		/// @param command string.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddCPUCommand)(const char* command);

		/// @brief Execute the assembled CPU command sequence.
		/// @param ethernet_check_period_in_ms period in ms between Ethernet status checks while the CPU command sequence is running. Use 0 to not check the Ethernet status (increases timing precision and speed slightly).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ExecuteCPUCommandSequence)(unsigned long ethernet_check_period_in_ms);

		/// @brief Stop CPU command sequence at next programmed stop point.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(StopCPUCommandSequence)();

		/// @brief Interrupt CPU command sequence immediately.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(InterruptCPUCommandSequence)();

		/// @brief Request CPU command error messages from the master sequencer. The errors will be displayed on the FPGA UART and copied into the API error buffer.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetCPUCommandErrorMessages)();
		
		/// @brief print CPU command error messages on FPGA UART.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(PrintCPUCommandErrorMessages)();
		
		/// @brief print CPU command sequence on FPGA UART.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(PrintCPUCommandSequence)();


		//the following are convenience functions, which allow us to define nice names to the few most important functions
		//You can add as many convenience functions as you like. Make sure to copy them also into the list of convenience functions in CDevice, CDevice.h, to assure they can always be called in any device.
		//Then define them in the device that provides the function. In this way we use the inheritance mechanism to automatically check if the function is available in the device and )(optionally) produce an error if not.

		//Analog out
		// Convenience functions to easily access the most important functions of the devices
		/// @brief Sets the voltage of an analog output device.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the voltage for.
		/// @param Voltage the voltage to set.
		//Analog out
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetVoltage)(const unsigned int& Sequencer, const unsigned int& Address, double Voltage);
		
		//Digital out
		/// @brief Sets a digital output to high or low.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the digital output for.
		/// @param BitNr the bit number of the digital output to set.
		/// @param OnOff true to set the output to high, false to set it to low.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetDigitalOutput)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t BitNr, bool OnOff);

		//AD9854
		/// @brief Sets the start frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the start frequency for.
		/// @param Frequency the frequency to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStartFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the stop frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the stop frequency for.
		/// @param Frequency the frequency to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStopFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the modulation frequency of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the modulation frequency for.
		/// @param Frequency the frequency to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetModulationFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the power of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the power for.
		/// @param Power the power to set in % (0...100).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPower)(const unsigned int& Sequencer, const unsigned int& Address, double Power);

		/// @brief Sets the attenuation of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the attenuation for.
		/// @param Attenuation the attenuation to set in dB (-xxx ... 0).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetAttenuation)(const unsigned int& Sequencer, const unsigned int& Address, double Attenuation);


		/// @brief Enables or disables automatic IO update after commands sent to a DDS device that supports IO update control.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the DDS device.
		/// @param IOUpdateEnabled true to enable automatic IO update after each SPI command, false to leave IO update under explicit/manual control.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetIOUpdateEnabled)(const unsigned int& Sequencer, const unsigned int& Address, bool IOUpdateEnabled);

		/// @brief Sets the start frequency tuning word of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the start frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStartFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		/// @brief Sets the stop frequency tuning word of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the stop frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetStopFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		/// @brief Sets the FKS mode of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the FKS mode for.
		/// @param mode the FKS mode to set (0..4).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFSKMode)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t mode);

		/// @brief Sets the ramp rate clock of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the ramp rate clock for.
		/// @param rate the ramp rate clock to set (1...1048576).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetRampRateClock)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t rate);

		/// @brief Clears the ACC1 bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the ACC1 bit for.
		/// @param OnOff true to set the ACC1 bit, false to clear it.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetClearACC1)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		/// @brief Sets the triangle bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the triangle bit for.
		/// @param OnOff true to set the triangle bit, false to clear it.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetTriangleBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		/// @brief Sets the FSK bit of a DDS (for now a AD9854 DDS).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the FSK bit for.
		/// @param OnOff true to set the FSK bit, false to clear it.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFSKBit)(const unsigned int& Sequencer, const unsigned int& Address, bool OnOff);

		
		//AD9858
		/// @brief Sets the frequency of a DDS.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency for.
		/// @param Frequency the frequency to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequency)(const unsigned int& Sequencer, const unsigned int& Address, double Frequency);

		/// @brief Sets the frequency tuning word of a DDS.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set. 
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyTuningWord)(const unsigned int& Sequencer, const unsigned int& Address, uint64_t FrequencyTuningWord);

		//those two functions have been defined already in the context of the AD9854, so we don't need to redefine them here
		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPower)(const unsigned int& Sequencer, const unsigned int& Address, double Power);//same as for AD9854, no need to redefine
		//API_EXPORT ERROR_CODE_TYPE CLA_FN(SetAttenuation)(const unsigned int& Sequencer, const unsigned int& Address, double Power);

		//AD9959

		/// @brief Resets the AD9959.
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency for.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(Reset)(const unsigned int& Sequencer, const unsigned int& Address);

		/// @brief Sets the frequency of a multi channel DDS (for now the AD9959).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency for.
		/// @param channel the channel number to set the frequency for.
		/// @param Frequency the frequency to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Frequency);
		
		/// @brief Sets the frequency tuning word of a multi channel DDS (for now the AD9959).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the frequency tuning word for.
		/// @param channel the channel number to set the frequency tuning word for.
		/// @param FrequencyTuningWord the frequency tuning word to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetFrequencyTuningWordOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, uint64_t FrequencyTuningWord);
		
		/// @brief Sets the phase of a multi channel DDS (for now the AD9959).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the phase for.
		/// @param channel the channel number to set the phase for.
		/// @param Phase the phase to set.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPhaseOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Phase);

		/// @brief Sets the power of a multi channel DDS (for now the AD9959).
		/// @param Sequencer the sequencer to use.
		/// @param Address the address of the device to set the power for.
		/// @param channel the channel number to set the power for.
		/// @param Power the power to set in percent (0...100).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetPowerOfChannel)(const unsigned int& Sequencer, const unsigned int& Address, uint8_t channel, double Power);



		

		// once the sequence is assembled, we can send it to the FPGA without executing it  
		/// @brief Send the sequence that was previously assembled to FPGA SoM, but do not execute it.
		/// @param FileName optional base filename for writing a human-readable sequence debug file. Pass an empty string to disable file writing.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(SendSequence)(const char* FileName = "");

		
		// once the sequence is assembled, then execute it 
		/// @brief Send the sequence that was previously assembled to FPGA and execute it.
		/// @param FileName optional base filename for writing a human-readable sequence debug file. Pass an empty string to disable file writing.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(ExecuteSequence)(const char* FileName = "");

		// once the sequence is assembled, then execute it 
		/// @brief Execute the sequence that was sent to the FPGA.
		/// @return See return convention above.
		API_EXPORT void CLA_FN(RepeatSequence)();

		//Wait till the sequence is finished
		/// @brief Waits until the sequence is finished.
		/// @param timeout_in_s timeout in seconds. If timeout_in_s > 0.001 and the sequence is not finished within this time, the function returns false or throws an exception (depending on mode selected mode when compiling API).
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(WaitTillEndOfSequence)(double timeout_in_s = 0);

		//check how far the sequence has been executed
		/// @brief Get the current status of the sequence execution.
		/// @param  running returns true if the sequence is running, false if it is not.
		/// @param DataPointsWritten returns number of data points that were already written out by the FPGA sequencer(s).
		API_EXPORT ERROR_CODE_TYPE CLA_FN(GetSequenceExecutionStatus)(bool& running, unsigned long long& DataPointsWritten);

		//Wait till the sequence is finished, and get the data from the input devices
		/// @brief Waits until the sequence is finished, then gets the input data from the FPGA sequencer.  
		/// @param buffer pointer to the input data buffer. Don't delete this buffer, it is managed by the API.
		/// @param buffer_length length of the input data buffer in bytes.
		/// @param EndTimeOfCycle returns the end time of the cycle in ms.
		/// @param timeout_in_s timeout in seconds. If timeout_in_s > 0.001 and the sequence is not finished within this time, the function returns false or throws an exception (depending on mode selected mode when compiling API).
		API_EXPORT ERROR_CODE_TYPE CLA_FN(WaitTillEndOfSequenceThenGetInputData)(uint8_t*& buffer, unsigned long& buffer_length, unsigned  long& EndTimeOfCycle, double timeout_in_s);
		
		/// @brief Sets a guard time. If sequencer commands make the time advance more than the guard time beyond what's allowed by Wait_ms, an error will be recorded (check with DidErrorOccur() if that happened) or thrown.
		/// @param MaxTimeDebt_in_ms maximum time debt in ms. If the sequencer commands make the time advance more than this, the sequencer will stop and wait for the next command.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetTimeDebtGuard_in_ms)(const double& MaxTimeDebt_in_ms);


		/**  @brief Reset one sequencer, including the FPGA fabric.
		* @param Sequencer the sequencer to reset.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ResetSequencer)(uint8_t SequencerID);
		
		/**  @brief Reset all sequencers, including their FPGA fabric.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ResetAllSequencers)();

		/** @brief Start analog acquisition on the specified channel.
		 * This function places a commandto for the FPGA in the sequencer buffer.
		 * The analog in acquisition will start when this command is executed by the FPGA.
		 * Only one analog in acquisition can hapen at each moment.
		 * A new one can start right after the previous one is finished.
		 * The data will be returned at the end of a sequence when using
		 * CLA_WaitTillEndOfSequenceThenGetInputData.
		 * @param Sequencer the sequencer to use.
		 * @param analog_in_type Analog in board type. 0: AQuRA MCP3208 analog in board; 1: MCP3208 12-bit ADC on SerialPortBoard; 2: ADS1256 24-bit ADC.
		 * @param SPI_CS SPI chip-select setting to use. This is the 3-bit CS bit-pattern on the rack backplane. A 3-to-8 demultiplexer is used on the serial port board to create 8 CS lines. If a board only needs 3 or less CS lines, these lines can be used directly (in that case: make sure to only pull one line low at a time).
		 * @param ChannelNumber the channel number to use.
		 * @param NumberOfDataPoints the number of data points to acquire.
		 * @param DelayBetweenDataPoints_in_ms the delay between data points in ms.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerStartAnalogInAcquisition)(const unsigned int& Sequencer, const uint8_t &analog_in_type, const uint8_t &SPI_CS, const uint8_t& ChannelNumber, const uint32_t& NumberOfDataPoints, const double& DelayBetweenDataPoints_in_ms);
		
		/**  @brief Writes a value to the input memory of the sequencer.
		 * This is useful to mark the start or end of a data acquisition, or to mark the type of experimental run in the full fledged version of the ControlAPI, which can cycle sequences automatically in the background.
		* To execute this function, a command for the FPGA must be placed in the sequencer buffer.
		* @param Sequencer the sequencer to use.
		* @param input_buf_mem_data the data to write to the input memory.
		* @param write_next_address true to write the next address, false to write the address given.
		* @param input_buf_mem_address the address to write to, if write_next_address is false.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerWriteInputMemory)(const unsigned int& Sequencer, unsigned long input_buf_mem_data, bool write_next_address = 1, unsigned long input_buf_mem_address = 0);
		

		/** @brief To use a DDS like a VCO: calculates the frequency tuning word for a AD9854 DDS using a centre frequency and a detuning proportional to a ADC value.
		* The frequency tuning word is proportional to the period of the DDS output frequency, i.e. it's 1/frequency
		* As with all Analog Devices DDS devices, the value of the frequency tuning word is determined by
		* FTW = (Desired Output Frequency × 2^N)/SYSCLK
		* where:
		* N is the phase accumulator resolution (48 bits in this instance).
		* Desired Output Frequency is expressed in hertz.
		* FTW (frequency tuning word) is a decimal number. 
        * The desired frequency is
        *   f = f0 + c * voltage = f0 + deltaf
        * voltage is the 16-bit value provided by the ADC 
        * We don't want to use a multiplication. Instead we approximate, using epsilon = deltaf/f0 << 1.
        *  delta f = c* voltage
        *  ftw = 1/f = 1/(f0 + deltaf) = 1/ (f0 * (1 + deltaf/f0)) = (1/f0)*(1/(1+epsilon)) ~ ftw0 * (1-epsilon) 
        *      = ftw0 - ftw0*deltaf/f0 = ftw0 - ftw0 * ftw0 * c * voltage = ftw0 - scale * ADC_value
        * We replace the multiplication by a bitshift, i.e. we allow scale = 2^n with n=[0...32].
		* 
		* 
		* Assuming the ADC provides 16 bits
		*	min frequency change: (1 << bit_shift) SYSCLK / (2<<48)     frequency range:  (1 << (bit_shift+16)) SYSCLK / (2<<48)
		*	2^48=	281,474,976,710,656	SYSCLCK	80000000
		*	bitshift	1<<bitshift	deltaf_min	deltaf_max
		*	0	1			2.84217E-07	0.018626451
		*	1	2			5.68434E-07	0.037252903
		*	2	4			1.13687E-06	0.074505806
		*	3	8			2.27374E-06	0.149011612
		*	4	16			4.54747E-06	0.298023224
		*	5	32			9.09495E-06	0.596046448
		*	6	64			1.81899E-05	1.192092896
		*	7	128			3.63798E-05	2.384185791
		*	8	256			7.27596E-05	4.768371582
		*	9	512			0.000145519	9.536743164
		*	10	1024		0.000291038	19.07348633
		*	11	2048		0.000582077	38.14697266
		*	12	4096		0.001164153	76.29394531
		*	13	8192		0.002328306	152.5878906
		*	14	16384		0.004656613	305.1757813
		*	15	32768		0.009313226	610.3515625
		*	16	65536		0.018626451	1220.703125
		*	17	131072		0.037252903	2441.40625
		*	18	262144		0.074505806	4882.8125
		*	19	524288		0.149011612	9765.625
		*	20	1048576		0.298023224	19531.25
		*	21	2097152		0.596046448	39062.5
		*	22	4194304		1.192092896	78125
		*	23	8388608		2.384185791	156250
		*	24	16777216	4.768371582	312500
		*	25	33554432	9.536743164	625000
		*	26	67108864	19.07348633	1250000
		*	27	134217728	38.14697266	2500000
		*	28	268435456	76.29394531	5000000
		*	29	536870912	152.5878906	10000000
		*	30	1073741824	305.1757813	20000000
		*	31	2147483648	610.3515625	40000000
		* 
		* 
		* 
		* @param Sequencer the sequencer to use.
		* @param ftw0 the centre frequency tuning word.
		* @param bit_shift the bit shift to use. Default is 22, which corresponds to a maximum tuning range of 78kHz and a change of 1.19Hz per ADC value increment.
		* @return See return convention above.
		*/
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerCalcAD9854FrequencyTuningWord)(const unsigned int& Sequencer, uint64_t ftw0, uint8_t bit_shift = 22);


		/// @brief Writes the current FPGA clock ticks to the input memory of the sequencer.
		/// @param  Sequencer the sequencer to use.
		/// 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerWriteSystemTimeToInputMemory)(const unsigned int& Sequencer);

		/// @brief Switches the debug LED of the FPGA on or off.
		/// @param  Sequencer the sequencer to use.
		/// @param  OnOff true to switch on the LED, false to switch it off.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSwitchDebugLED)(const unsigned int& Sequencer, unsigned int OnOff);

		/// @brief Sets the sequencer digital output pattern.
		/// @param Sequencer the sequencer to use.
		/// @param dig_out_pattern the 8-bit digital output pattern.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetSequencerDigitalOut)(const unsigned int& Sequencer, uint8_t dig_out_pattern);

		/// @brief Sets the sequencer PL-to-PS command byte.
		/// @param Sequencer the sequencer to use.
		/// @param PL_to_PS_command the 8-bit PL-to-PS command.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SetSequencer_PL_to_PS_command)(const unsigned int& Sequencer, uint8_t PL_to_PS_command);

		/// @brief Switches the sequencer buzzer on or off.
		/// @param Sequencer the sequencer to use.
		/// @param OnOff true to switch on the buzzer, false to switch it off.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SwitchSequencerBuzzer)(const unsigned int& Sequencer, bool OnOff);

		/**  @brief Switches the sequencer to ignore TCP/IP commands.
		 * This is useful to prevent the sequencer from being interrupted by TCP/IP commands while executing a timing critical task, such as transferring input data from the FPGA BRAM to the DDR.
		 * This can be useful if lots of data is acquired at a high rate. Lot's of meaning: more than the size of the BRAM input buffer.
		 * How much that is depends how you configured the Vivado project that generates the FPGA sequencer firmware.
		 */
		 /// @param Sequencer the sequencer to use.
		 /// @param OnOff true to make the sequencer ignore TCP/IP commands, false to resume normal TCP/IP command handling.
		 /// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerIgnoreTCPIP)(const unsigned int& Sequencer, bool OnOff);

		/// @brief Configures the sequencer to use edge-triggered latches.
		/// @param Sequencer the sequencer to use.
		/// @param UseEdgeTriggeredLatches true to enable edge-triggered latches, false to use the default latch timing.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(UseEdgeTriggeredLatches)(const unsigned int& Sequencer, bool UseEdgeTriggeredLatches);

		/// @brief Places a marker in the sequencer buffer. The FPGA SOM's CPU will see this marker and can react to it, e.g. write it to the USB-UART for debugging.
		/// @param Sequencer the sequencer to use.
		/// @param marker marker byte to insert into the sequencer command stream.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerAddMarker)(const unsigned int& Sequencer, unsigned char marker);

		/// @brief Sets the time debt guard for a specific sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param MaxTimeDebt_in_ms the maximum time debt in ms.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetTimeDebtGuard_in_ms)(const unsigned int& Sequencer, const double& MaxTimeDebt_in_ms);

		/// @brief Sets the loop count for a sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param loop_count the loop count to set.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetLoopCount)(const unsigned int& Sequencer, unsigned int loop_count);
		
		/// @brief Jumps backward in the sequence.
		/// @param Sequencer the sequencer to use.
		/// @param jump_length the length of the jump in commands.
		/// @param unconditional_jump true to jump unconditionally, false to jump only if a condition is met.
		/// @param condition_0 true if condition 0 is met, false otherwise.
		/// @param condition_1 true if condition 1 is met, false otherwise.
		/// @param condition_PS true if the PS condition is met, false otherwise.
		/// @param condition_dig_in true if the digital input condition is enabled, false otherwise.
		/// @param dig_in_bit_nr the digital input bit number to test.
		/// @param loop_count_greater_zero true if the loop count is greater than zero, false otherwise.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerJumpBackward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0, bool loop_count_greater_zero = false);
		
		/// @brief Jumps forward in the sequence.
		/// @param Sequencer the sequencer to use.
		/// @param jump_length the length of the jump in commands.
		/// @param unconditional_jump true to jump unconditionally, false to jump only if a condition is met.
		/// @param condition_0 true if condition 0 is met, false otherwise.
		/// @param condition_1 true if condition 1 is met, false otherwise.
		/// @param condition_PS true if the PS condition is met, false otherwise.
		/// @param condition_dig_in true if the digital input condition is enabled, false otherwise.
		/// @param dig_in_bit_nr the digital input bit number to test.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerJumpForward)(const unsigned int& Sequencer, unsigned int jump_length, bool unconditional_jump = true, bool condition_0 = false, bool condition_1 = false, bool condition_PS = false, bool condition_dig_in = false, uint8_t dig_in_bit_nr = 0);

		/// @brief Writes an I2C command to the sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param I2C_port I2C port index on the sequencer. 1 is the I2C port exposed by the serial port board and used by external hardware. 0 is the I2C port connected to the backplane multiplexer, reading out board EEPROMS and addresses.
		/// @param I2C_length_out number of bytes to transmit from data_out.
		/// @param I2C_length_in number of bytes to read back after the write phase.
		/// @param data_out bytes to transmit. The buffer must contain at least I2C_length_out bytes.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerTransmitI2C)(const unsigned int& Sequencer, uint8_t I2C_port, uint8_t I2C_length_out, uint8_t I2C_length_in, uint8_t* data_out);

		/// @brief Writes an SPI transmit command to the sequencer.
		/// @param Sequencer the sequencer to use. (SPI port 1 will be used, which is the port used by existing control hardware and exposed by the serial port board to the user.)
		/// @param chip_select SPI chip-select line or encoded chip-select value. This is the 3-bit CS bit-pattern on the rack backplane. A 3-to-8 demultiplexer is used on the serial port board to create 8 CS lines. If a board only needs 3 or less CS lines, these lines can be used directly (in that case: make sure to only pull one line low at a time).
		/// @param number_of_bits_out number of bits to transmit from data_out.
		/// @param data_out bit payload to transmit. The buffer must contain enough bytes for number_of_bits_out.
		/// @param number_of_bits_in number of bits to read back after the write phase.
		/// @param start_now true to start the SPI transfer immediately when the command is executed; false to configure it for SequencerRepeatedOutIn without immediate start.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerTransmitSPI)(const unsigned int& Sequencer, uint8_t chip_select, uint16_t number_of_bits_out, const uint8_t* data_out, uint8_t number_of_bits_in, bool start_now);

		/// @brief Configures repeated input/output operations in the sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param number_of_datapoints the number of data points to read/write.
		/// @param delay_between_datapoints_in_ms the delay between data points in ms
		/// @param RepeatedOutInCommand the command to execute for each data point. 0: stop; 1: repeated SPI transfer; 2: repeated digital in; 3: digital in event tagger 
		/// for 2: bits 7:0 : digital input; bits 28:8 : repeat number; bits 31:29 = b010 as magic number to identify this input memory entry came from digital input; 
		/// for 3: if dig in changes, safes dig in on input memory bit 0:7, bit 8: counter overflow, bit 9: 4-entry fifo overflow, bit 10:31: clock cycle counter; runs till stopped by setting RepeatedOutInCommand to 0 with new SequencerRepeatedOutIn command.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerRepeatedOutIn)(const unsigned int& Sequencer, uint16_t number_of_datapoints, double delay_between_datapoints_in_ms, uint8_t RepeatedOutInCommand);

		/// @brief Loads SPI timing parameters into the sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param SPI_delay_CS_low_start_wait delay after chip-select goes low before writing starts, in sequencer timing units, i.e. 10ns for the sequencer internal 100MHz clock.
		/// @param SPI_delay_write delay used while writing SPI bits, in sequencer timing units.
		/// @param SPI_delay_pause_before_read delay between write and read phases, in sequencer timing units.
		/// @param SPI_delay_read delay used while reading SPI bits, in sequencer timing units.
		/// @param SPI_delay_CS_low_end_wait delay before chip-select is released, in sequencer timing units. 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetSPITiming)(const unsigned int& Sequencer, uint16_t SPI_delay_CS_low_start_wait, uint16_t SPI_delay_write, uint16_t SPI_delay_pause_before_read, uint16_t SPI_delay_read, uint16_t SPI_delay_CS_low_end_wait);

		/// @brief Sets the SPI mode for the sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param SPI_mode SPI mode number, normally 0..3 for CPOL/CPHA selection.
		///Mode	CPOL	CPHA	Sample edge
		///0	0	0	rising edge
		///1	0	1	falling edge
		///2	1	0	falling edge
		///3	1	1	rising edge
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetSPIMode)(const unsigned int& Sequencer, uint8_t SPI_mode);

		/// @brief Sets I2C parameters for the sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param I2C_0_Destination destination or routing value for I2C bus. Currently unused.
		/// @param I2C_delay_start_stop start/stop timing delay in sequencer timing units, i.e. 10ns for the sequencer internal 100MHz clock.
		/// @param I2C_delay_data_setup data setup timing delay in sequencer timing units.  
		/// @param I2C_delay_clock_high clock-high timing delay in sequencer timing units. 
		/// @param I2C_delay_clock_low clock-low timing delay in sequencer timing units. 
		/// @param I2C_delay_pause_before_read pause before read phase in sequencer timing units. 
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SequencerSetI2CParameters)(const unsigned int& Sequencer, uint8_t I2C_0_Destination, uint8_t I2C_delay_start_stop, uint8_t I2C_delay_data_setup, uint8_t I2C_delay_clock_high, uint8_t I2C_delay_clock_low, uint8_t I2C_delay_pause_before_read);


		/// @brief Selects a slot in one of the racks connected in a chain to a sequencer.
		/// @param Sequencer the sequencer to use.
		/// @param rack_nr rack number in the rack chain.
		/// @param slot_nr slot number within the selected rack.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(SelectRackSlot)(const unsigned int& Sequencer, uint8_t rack_nr, uint8_t slot_nr);


		/// @brief Resets the I2C multiplexer used for rack-slot selection on a sequencer.
		/// @param Sequencer the sequencer to use.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(ResetI2CMultiplexer)(const unsigned int& Sequencer);


		//the following functions are used to add devices to the sequencer. I placed them here to avoid clutter above. They have to be called before Initialize)().	
		/// @brief Add a device sequencer to the list.
		/// @param id the id of the device sequencer to add (this is the number that all other commands are using when sending data to a device on this sequencer).
		/// @param type the type of the device sequencer to add.
		/// @param ip the ip address of the device sequencer to add.
		/// @param port the port of the device sequencer to add.
		/// @param master true if the device sequencer is a master, false if it is a slave.
		/// @param startDelay the start delay of the device sequencer to add.
		/// @param clockFrequency the clock frequency of the device sequencer to add.
		/// @param FPGAClockToBusClockRatio the FPGA clock to bus clock ratio of the device sequencer to add.
		/// @param useExternalClock true if the device sequencer should use an external clock, false if it should use the internal clock.
		/// @param useStrobeGenerator true if the device sequencer should use a strobe generator, false if it should not.
		/// @param UseEdgeTriggeredLatches true if the sequencer should use edge-triggered latches, false for the default latch timing.
		/// @param connect true if the device sequencer should connect to the device, false if it should not. 
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceSequencer)(
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
			bool UseEdgeTriggeredLatches,
			bool connect);

		/// @brief Add a 16 bit analog output device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param startAddress the start address of the device to add.
		/// @param numberChannels the number of channels of the device to add.
		/// @param signedValue true if the device is signed, false if it is unsigned.
		/// @param minVoltage the minimum voltage of the device to add.
		/// @param maxVoltage the maximum voltage of the device to add.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAnalogOut16bit)(
			unsigned int sequencer,
			unsigned int startAddress,
			unsigned int numberChannels,
			bool signedValue,
			double minVoltage,
			double maxVoltage);

		/// @brief Add a digital output device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param numberChannels the number of channels of the device to add.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceDigitalOut)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int numberChannels);


		/// @brief Add a AD9854 device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param version the version of the device to add.
		/// @param externalClockFrequency the external clock frequency of the device to add.
		/// @param PLLReferenceMultiplier the PLL reference multiplier of the device to add.
		/// @param frequencyMultiplier the frequency multiplier of the device to add.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAD9854)(
			unsigned int sequencer,
			unsigned int address,
			unsigned int version,
			double externalClockFrequency,
			uint8_t PLLReferenceMultiplier,
			unsigned int frequencyMultiplier);

		/// @brief Add a AD9858 device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param externalClockFrequency the external clock frequency of the device to add.
		/// @param frequencyMultiplier the frequency multiplier of the device to add.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAD9858)(
			unsigned int sequencer,
			unsigned int address,
			double externalClockFrequency,
			unsigned int frequencyMultiplier);

		/// @brief Add a AD9959 device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param address the address of the device to add.
		/// @param externalClockFrequency the external clock frequency of the device to add.
		/// @param frequencyMultiplier the frequency multiplier of the device to add.
		/// @param AD9958 true if the connected device is the two-channel AD9958 variant, false for the four-channel AD9959.
		/// @param version Version of the AD9959 board. This decides on way bit-banged SPI is performed.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAD9959)(
			unsigned int sequencer,
			unsigned int address,
			double externalClockFrequency,
			unsigned int frequencyMultiplier,
			bool AD9958,
			double version);

		/// @brief Add a 12 bit analog input device to the sequencer.
		/// @param sequencer the sequencer to use.
		/// @param chipSelect the chip select of the device to add.
		/// @param signedValue true if the device is signed, false if it is unsigned.
		/// @param minVoltage the minimum voltage of the device to add.
		/// @param maxVoltage the maximum voltage of the device to add.
		/// @return See return convention above.
		API_EXPORT ERROR_CODE_TYPE CLA_FN(AddDeviceAnalogIn12bit)(
			unsigned int sequencer,
			unsigned int chipSelect,
			bool signedValue,
			double minVoltage,
			double maxVoltage);


#ifdef API_CLASS
		};

#endif
	

#ifdef C_API
#ifdef __cplusplus
}
#endif
#else
#ifdef NAMESPACE_CLA
	} //option namespace CLA
#endif
#endif
