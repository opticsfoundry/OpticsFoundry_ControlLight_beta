// ControlLight.cpp : Defines the entry point for the application.
//

//#include "std.h"
#include "ControlAPI.h"
#include "std.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include "AutoConfig.h"


using namespace std;

#ifdef API_CLASS
namespace {
	ControlLight_API* ActiveAutoConfigAPI = nullptr;

	ControlLight_API& AutoConfigAPI() {
		if (!ActiveAutoConfigAPI) {
			throw CLA_Exception("AutoConfig API instance is not set.");
		}
		return *ActiveAutoConfigAPI;
	}
}

void SetAutoConfigAPI(ControlLight_API* api) {
	ActiveAutoConfigAPI = api;
}

#define AUTO_CONFIG_API_CALL(name, ...) AutoConfigAPI().name(__VA_ARGS__)
#else
#define AUTO_CONFIG_API_CALL(name, ...) CLA_##name(__VA_ARGS__)
#endif

namespace {
	constexpr uint8_t NrSlots = 13; // "Slot" 13 is the backplane memory.
	constexpr uint8_t I2CMultAddr[2] = { 0xE0, 0xEE};
	constexpr uint8_t I2CPortNr[NrSlots] = { 6, 2, 3, 1, 0, 4, 5, 6, 7, 0, 1, 2, 5 };
	constexpr uint8_t I2CMux[NrSlots] =    { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0 };
	constexpr uint8_t I2CChainMult = 0;
	constexpr uint8_t I2CChainPortNr = 7;
	constexpr uint8_t I2CMux1PortNrOnMux0 = 4;
	constexpr uint8_t MaxSupportedRackNr = 6;
	constexpr uint8_t Write = 0;
	constexpr uint8_t Read = 1;
	constexpr uint8_t EEPROMAddress = 0xA2;
	constexpr uint8_t ConfigAddressIOExpanderAddress = 0x40;
	constexpr uint32_t I2CClockFrequencyInHz = 100000;
	constexpr size_t EEPROMSizeInBytes = 256;

	std::string GetModelPrefix(const json& board_json) {
		if (!board_json.contains("Model") || !board_json["Model"].is_string()) {
			return "";
		}

		std::string model = board_json["Model"];
		const size_t version_marker = model.find(" V");
		if (version_marker != std::string::npos) {
			model = model.substr(0, version_marker);
		}
		return model;
	}

	void CopyFieldIfPresent(const json& source, json& destination, const char* source_key, const char* destination_key = nullptr) {
		const char* target_key = destination_key ? destination_key : source_key;
		if (source.contains(source_key)) {
			destination[target_key] = source[source_key];
		}
	}

	void AddCommonMetadata(const json& source, json& destination, uint8_t sequencer_nr, uint8_t rack_nr, uint8_t slot_nr) {
		destination["Sequencer"] = sequencer_nr;
		destination["RackNr"] = rack_nr;
		destination["SlotNr"] = slot_nr;
		CopyFieldIfPresent(source, destination, "Model");
		CopyFieldIfPresent(source, destination, "SN");
	}

	void PrintEEPROMData(const char* label, const uint8_t* data, const size_t length) {
		cout << label << " (" << length << " byte(s)):" << endl;
		cout << "ASCII: ";
		for (size_t index = 0; index < length; ++index) {
			const uint8_t value = data[index];
			cout << (isprint(value) ? static_cast<char>(value) : '.');
		}
		cout << endl;

		cout << "Hex:   ";
		for (size_t index = 0; index < length; ++index) {
			cout << hex << setw(2) << setfill('0') << static_cast<unsigned int>(data[index]);
			if (index + 1 < length) {
				cout << ' ';
			}
		}
		cout << dec << setfill(' ') << endl;
	}

	std::string MakeConfigOutputFilename(const std::string& filename) {
		std::string output_filename = filename;
		if (output_filename.size() >= 5 && output_filename.substr(output_filename.size() - 5) == ".json") {
			output_filename = output_filename.substr(0, output_filename.size() - 5);
		}
		return output_filename + "_config.json";
	}

	std::string MakeConfigCreatorOutputBase(const std::string& filename) {
		std::string output_base = filename;
		if (output_base.size() >= 5 && output_base.substr(output_base.size() - 5) == ".json") {
			output_base = output_base.substr(0, output_base.size() - 5);
		}
		return output_base + "_config_creator";
	}

	std::string FormatNumber(const double value) {
		std::ostringstream stream;
		stream << std::setprecision(15) << value;
		return stream.str();
	}

	std::string JsonStringLiteral(const std::string& value) {
		return json(value).dump();
	}

	std::string CppOptionalString(const json& entry, const char* key) {
		if (!entry.contains(key) || entry[key].is_null()) {
			return "std::nullopt";
		}
		return "std::optional<std::string>(" + JsonStringLiteral(entry[key].get<std::string>()) + ")";
	}

	std::string CppOptionalInt(const json& entry, const char* key) {
		if (!entry.contains(key) || entry[key].is_null()) {
			return "std::nullopt";
		}
		return "std::optional<int>(" + std::to_string(entry[key].get<int>()) + ")";
	}

	std::string PythonOptionalString(const json& entry, const char* key) {
		if (!entry.contains(key) || entry[key].is_null()) {
			return "None";
		}
		return JsonStringLiteral(entry[key].get<std::string>());
	}

	std::string PythonOptionalInt(const json& entry, const char* key) {
		if (!entry.contains(key) || entry[key].is_null()) {
			return "None";
		}
		return std::to_string(entry[key].get<int>());
	}

	std::string PythonBool(const bool value) {
		return value ? "True" : "False";
	}

	std::string CppBool(const bool value) {
		return value ? "true" : "false";
	}

	bool JsonBoolValue(const json& entry, const char* key, const bool default_value) {
		if (!entry.contains(key)) {
			return default_value;
		}
		if (entry[key].is_boolean()) {
			return entry[key].get<bool>();
		}
		if (entry[key].is_number_integer()) {
			return entry[key].get<int>() != 0;
		}
		if (entry[key].is_string()) {
			const std::string value = entry[key].get<std::string>();
			return value == "1" || value == "true" || value == "True";
		}
		return default_value;
	}

	double FrequencyInMHz(const json& entry, const char* mhz_key, const char* hz_key, const double default_mhz) {
		if (entry.contains(mhz_key)) {
			return entry[mhz_key].get<double>();
		}
		if (entry.contains(hz_key)) {
			return entry[hz_key].get<double>() / 1e6;
		}
		return default_mhz;
	}

	int AnalogInAddress(const json& entry) {
		if (entry.contains("ChipSelect")) {
			return entry["ChipSelect"].get<int>();
		}
		return entry.value("Address", 80);
	}

	std::filesystem::path ControlLightSourceDirectory() {
		return std::filesystem::absolute(std::filesystem::path(__FILE__)).parent_path();
	}

	void WriteAutoConfigCppCreator(const json& auto_config, const std::string& creator_filename, const std::string& config_filename) {
		std::ofstream file(creator_filename);
		if (!file.is_open()) {
			cout << "Failed to open file " << creator_filename << " for writing." << endl;
			return;
		}

		const std::string include_path = (ControlLightSourceDirectory() / "ConfigCreator.h").generic_string();
		file << "#include " << JsonStringLiteral(include_path) << "\n\n";
		file << "int main() {\n";
		file << "\tConfigCreator builder(" << JsonStringLiteral(config_filename) << ");\n\n";

		for (const auto& entry : auto_config["Sequencers"]) {
			file << "\tbuilder.RegisterSequencer(/*Id*/ " << entry.value("Id", 0)
				<< ", /*Type*/ " << JsonStringLiteral(entry.value("Type", std::string("OpticsFoundrySequencerV1")))
				<< ", /*IP*/ " << JsonStringLiteral(entry.value("IP", std::string("192.168.0.104")))
				<< ", /*Port*/ " << entry.value("Port", 7)
				<< ", /*Master*/ " << CppBool(entry.value("Master", true))
				<< ", /*StartDelay*/ " << entry.value("StartDelay", 10)
				<< ", /*ClockFrequencyinMHz*/ " << FormatNumber(entry.value("ClockFrequencyinMHz", 100.0))
				<< ", /*BusFrequencyinMHz*/ " << FormatNumber(entry.value("BusFrequencyinMHz", 2.0))
				<< ", /*UseExternalClock*/ " << CppBool(entry.value("UseExternalClock", false))
				<< ", /*UseStrobeGenerator*/ " << CppBool(entry.value("UseStrobeGenerator", true))
				<< ", /*UseEdgeTriggeredLatches*/ " << CppBool(entry.value("UseEdgeTriggeredLatches", true))
				<< ", /*Connect*/ " << CppBool(entry.value("Connect", true))
				<< ", /*DebugOn*/ " << CppBool(entry.value("DebugOn", false))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["AnalogOutBoards16bit"]) {
			file << "\tbuilder.RegisterAnalogOutBoard16bit(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*StartAddress*/ " << entry.value("StartAddress", 24)
				<< ", /*NumberChannels*/ " << entry.value("NumberChannels", 4)
				<< ", /*Signed*/ " << CppBool(entry.value("Signed", true))
				<< ", /*MinVoltage*/ " << FormatNumber(entry.value("MinVoltage", -10.0))
				<< ", /*MaxVoltage*/ " << FormatNumber(entry.value("MaxVoltage", 10.0))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["DigitalOutBoards"]) {
			file << "\tbuilder.RegisterDigitalOutBoard(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << entry.value("Address", 1)
				<< ", /*NumberChannels*/ " << entry.value("NumberChannels", 16)
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["SerialPortBoards"]) {
			file << "\tbuilder.RegisterSerialPortBoard(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << entry.value("Address", 1)
				<< ", /*RackNr*/ " << entry.value("RackNr", 0)
				<< ", /*SlotNr*/ " << entry.value("SlotNr", 0)
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN") << ");\n";
		}

		for (const auto& entry : auto_config["DDSAD9854Boards"]) {
			file << "\tbuilder.RegisterDDSAD9854Board(/*Version*/ " << entry.value("Version", 2)
				<< ", /*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << entry.value("Address", 132)
				<< ", /*ExternalClockFrequencyinMHz*/ " << FormatNumber(FrequencyInMHz(entry, "ExternalClockFrequencyinMHz", "ExternalClockFrequency", 300.0))
				<< ", /*PLLReferenceMultiplier*/ " << entry.value("PLLReferenceMultiplier", 1)
				<< ", /*FrequencyMultiplier*/ " << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["DDSAD9858Boards"]) {
			file << "\tbuilder.RegisterDDSAD9858Board(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << entry.value("Address", 50)
				<< ", /*ClockFrequencyinMHz*/ " << FormatNumber(FrequencyInMHz(entry, "ClockFrequencyinMHz", "ClockFrequency", 1200.0))
				<< ", /*FrequencyMultiplier*/ " << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["DDSAD9959Boards"]) {
			file << "\tbuilder.RegisterDDSAD9959Board(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << entry.value("Address", 21)
				<< ", /*ClockFrequencyinMHz*/ " << FormatNumber(FrequencyInMHz(entry, "ClockFrequencyinMHz", "ClockFrequency", 300.0))
				<< ", /*FrequencyMultiplier*/ " << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", /*AD9958*/ " << CppBool(JsonBoolValue(entry, "AD9958", false))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["AnalogInBoards12bit"]) {
			file << "\tbuilder.RegisterAnalogInBoard12bit(/*Sequencer*/ " << entry.value("Sequencer", 0)
				<< ", /*Address*/ " << AnalogInAddress(entry)
				<< ", /*NumberChannels*/ " << entry.value("NumberChannels", 4)
				<< ", /*MinVoltage*/ " << FormatNumber(entry.value("MinVoltage", -10.0))
				<< ", /*MaxVoltage*/ " << FormatNumber(entry.value("MaxVoltage", 10.0))
				<< ", /*Model*/ " << CppOptionalString(entry, "Model")
				<< ", /*SN*/ " << CppOptionalString(entry, "SN")
				<< ", /*RackNr*/ " << CppOptionalInt(entry, "RackNr")
				<< ", /*SlotNr*/ " << CppOptionalInt(entry, "SlotNr") << ");\n";
		}

		for (const auto& entry : auto_config["Rack"]) {
			file << "\tbuilder.RegisterRackEntry(json::parse(" << JsonStringLiteral(entry.dump()) << "));\n";
		}

		file << "\n\treturn builder.Save() ? 0 : 1;\n";
		file << "}\n";
		cout << "Auto configuration C++ creator saved to " << creator_filename << endl;
	}

	void WriteAutoConfigPythonCreator(const json& auto_config, const std::string& creator_filename, const std::string& config_filename) {
		std::ofstream file(creator_filename);
		if (!file.is_open()) {
			cout << "Failed to open file " << creator_filename << " for writing." << endl;
			return;
		}

		const std::string module_path = (ControlLightSourceDirectory() / "ConfigFileCreators").generic_string();
		file << "import json\n";
		file << "import sys\n\n";
		file << "sys.path.insert(0, " << JsonStringLiteral(module_path) << ")\n";
		file << "from ConfigCreator import ConfigBuilder\n\n\n";
		file << "if __name__ == \"__main__\":\n";
		file << "    builder = ConfigBuilder(" << JsonStringLiteral(config_filename) << ")\n\n";

		for (const auto& entry : auto_config["Sequencers"]) {
			file << "    builder.RegisterSequencer(Id=" << entry.value("Id", 0)
				<< ", Type=" << JsonStringLiteral(entry.value("Type", std::string("OpticsFoundrySequencerV1")))
				<< ", IP=" << JsonStringLiteral(entry.value("IP", std::string("192.168.0.104")))
				<< ", Port=" << entry.value("Port", 7)
				<< ", Master=" << PythonBool(entry.value("Master", true))
				<< ", StartDelay=" << entry.value("StartDelay", 10)
				<< ", ClockFrequencyinMHz=" << FormatNumber(entry.value("ClockFrequencyinMHz", 100.0))
				<< ", BusFrequencyinMHz=" << FormatNumber(entry.value("BusFrequencyinMHz", 2.0))
				<< ", UseExternalClock=" << PythonBool(entry.value("UseExternalClock", false))
				<< ", UseStrobeGenerator=" << PythonBool(entry.value("UseStrobeGenerator", true))
				<< ", UseEdgeTriggeredLatches=" << PythonBool(entry.value("UseEdgeTriggeredLatches", true))
				<< ", Connect=" << PythonBool(entry.value("Connect", true))
				<< ", DebugOn=" << PythonBool(entry.value("DebugOn", false))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["AnalogOutBoards16bit"]) {
			file << "    builder.RegisterAnalogOutBoard16bit(Sequencer=" << entry.value("Sequencer", 0)
				<< ", StartAddress=" << entry.value("StartAddress", 24)
				<< ", NumberChannels=" << entry.value("NumberChannels", 4)
				<< ", Signed=" << PythonBool(entry.value("Signed", true))
				<< ", MinVoltage=" << FormatNumber(entry.value("MinVoltage", -10.0))
				<< ", MaxVoltage=" << FormatNumber(entry.value("MaxVoltage", 10.0))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["DigitalOutBoards"]) {
			file << "    builder.RegisterDigitalOutBoard(Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << entry.value("Address", 1)
				<< ", NumberChannels=" << entry.value("NumberChannels", 16)
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["SerialPortBoards"]) {
			file << "    builder.RegisterSerialPortBoard(Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << entry.value("Address", 1)
				<< ", RackNr=" << entry.value("RackNr", 0)
				<< ", SlotNr=" << entry.value("SlotNr", 0)
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN") << ")\n";
		}

		for (const auto& entry : auto_config["DDSAD9854Boards"]) {
			file << "    builder.RegisterDDSAD9854Board(Version=" << entry.value("Version", 2)
				<< ", Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << entry.value("Address", 132)
				<< ", ExternalClockFrequencyinMHz=" << FormatNumber(FrequencyInMHz(entry, "ExternalClockFrequencyinMHz", "ExternalClockFrequency", 300.0))
				<< ", PLLReferenceMultiplier=" << entry.value("PLLReferenceMultiplier", 1)
				<< ", FrequencyMultiplier=" << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["DDSAD9858Boards"]) {
			file << "    builder.RegisterDDSAD9858Board(Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << entry.value("Address", 50)
				<< ", ClockFrequencyinMHz=" << FormatNumber(FrequencyInMHz(entry, "ClockFrequencyinMHz", "ClockFrequency", 1200.0))
				<< ", FrequencyMultiplier=" << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["DDSAD9959Boards"]) {
			file << "    builder.RegisterDDSAD9959Board(Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << entry.value("Address", 21)
				<< ", ClockFrequencyinMHz=" << FormatNumber(FrequencyInMHz(entry, "ClockFrequencyinMHz", "ClockFrequency", 300.0))
				<< ", FrequencyMultiplier=" << FormatNumber(entry.value("FrequencyMultiplier", 1.0))
				<< ", AD9958=" << PythonBool(JsonBoolValue(entry, "AD9958", false))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["AnalogInBoards12bit"]) {
			file << "    builder.RegisterAnalogInBoard12bit(Sequencer=" << entry.value("Sequencer", 0)
				<< ", Address=" << AnalogInAddress(entry)
				<< ", NumberChannels=" << entry.value("NumberChannels", 4)
				<< ", MinVoltage=" << FormatNumber(entry.value("MinVoltage", -10.0))
				<< ", MaxVoltage=" << FormatNumber(entry.value("MaxVoltage", 10.0))
				<< ", Model=" << PythonOptionalString(entry, "Model")
				<< ", SN=" << PythonOptionalString(entry, "SN")
				<< ", RackNr=" << PythonOptionalInt(entry, "RackNr")
				<< ", SlotNr=" << PythonOptionalInt(entry, "SlotNr") << ")\n";
		}

		for (const auto& entry : auto_config["Rack"]) {
			file << "    builder.RegisterRackEntry(json.loads(" << JsonStringLiteral(entry.dump()) << "))\n";
		}

		file << "\n    builder.Save()\n";
		cout << "Auto configuration Python creator saved to " << creator_filename << endl;
	}

	void WriteAutoConfigCreators(const json& auto_config, const std::string& filename) {
		const std::string output_base = MakeConfigCreatorOutputBase(filename);
		const std::string config_filename = MakeConfigOutputFilename(filename);
		WriteAutoConfigCppCreator(auto_config, output_base + ".cpp", config_filename);
		WriteAutoConfigPythonCreator(auto_config, output_base + ".py", config_filename);
	}

	void ReadEEPROMBytes(const uint8_t start_address, uint8_t* data, const uint32_t length, bool &I2C_success) {
		if (length == 0) {
			return;
		}

		uint8_t address = start_address;
		AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, EEPROMAddress + Write, /*send_length*/ 1, &address, /*receive_length*/ 0, data, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
		if (I2C_success) AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, EEPROMAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ static_cast<uint16_t>(length), data, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
	}
}

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


void ResetRackI2CMultiplexers(const uint8_t SequencerID) {
	//This function resets the I2C multiplexers of the specified rack, by writing 0 to the corresponding configuration register of the sequencer.
	//This is needed before writing to the EEPROM, to make sure that the I2C communication is working and that we are writing to the correct device.
	AUTO_CONFIG_API_CALL(StartAssemblingSequence);
	AUTO_CONFIG_API_CALL(ResetI2CMultiplexer, SequencerID);
	AUTO_CONFIG_API_CALL(ExecuteSequence, "");
	uint8_t *buffer = nullptr;
	unsigned long buffer_length = 0;
	unsigned long EndTimeOfCycle = 0;
	AUTO_CONFIG_API_CALL(WaitTillEndOfSequenceThenGetInputData, buffer, buffer_length, EndTimeOfCycle, 10);
}

bool SelectRackI2CSlot(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr) {
	//SlotNr 0..11 are rack slots.
	//SlotNr 12 is the memory on the rack backplane.
	if (SlotNr >= NrSlots) {
		cout << "SelectRackI2CSlot failed: slot number " << static_cast<unsigned int>(SlotNr) << " too high (0..11 are rack slots, 12 is the backplane memory)." << endl;
		return false;
	}

	if (RackNr > 6) {
		cout << "SelectRackI2CSlot failed: RackNr " << static_cast<unsigned int>(RackNr) << " is > 6 and therefore not supported. The rack still works, but auto-config doesn't." << endl;
		return false;
	}

	ResetRackI2CMultiplexers(SequencerID);
	bool I2C_overall_success = true;
	bool I2C_success;
	//Mux 0 of Rack N has address N. It's port I2CMultRackAddr is connected to the next rack. Let's select the target rack.
	uint8_t mux_select = static_cast<uint8_t>(1u << I2CChainPortNr);
	for (uint8_t I2CMultRackAddr=0; I2CMultRackAddr < RackNr; I2CMultRackAddr++) {
		//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port to access the chain rack; repeat till we reach the correct rack
		AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, 0xE0 + (I2CMultRackAddr << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
		I2C_overall_success &= I2C_success;
	}
	uint8_t mux_address = RackNr;
	//If the desired slot is 
	if (I2CMux[SlotNr] == 1) {
		mux_select = static_cast<uint8_t>(1u << I2CMux1PortNrOnMux0);
		AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, 0xE0 + (RackNr << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
		I2C_overall_success &= I2C_success;
		mux_address = 1+2+4;
	}

	//Set the I2C multiplexer (TCA9548A, see folder datasheet) to the correct port for the slot, or the backplane memory (for SlotNr == 12).
	mux_select = static_cast<uint8_t>(1u << I2CPortNr[SlotNr]);
	AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, 0xE0 + (mux_address << 1) + Write, /*send_length*/ 1, &mux_select, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
	I2C_overall_success &= I2C_success;
	return I2C_overall_success;
}

bool WriteConfigEEPROM(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, const char* data, const size_t length) {
	if (data == nullptr) {
		cout << "EEPROM write failed: data pointer is null." << endl;
		return false;
	}

	if (SlotNr >= NrSlots) {
		cout << "EEPROM write failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return false;
	}

	if (length > EEPROMSizeInBytes) {
		cout << "EEPROM write failed: length " << length << " exceeds EEPROM size of " << EEPROMSizeInBytes << " bytes." << endl;
		return false;
	}

	if (length == 0 ) {
		cout << "EEPROM write failed: nothing to write as length is zero." << endl;
		return false;
	}


	if (!SelectRackI2CSlot(SequencerID, RackNr, SlotNr)) return false;
	
	// Now write the data to the EEPROM of type M24C02-F (2kbit I2C EEPROM), see datasheet in folder datasheet, starting from memory address 0.

	constexpr size_t EEPROMPageSizeInBytes = 16;
	size_t address = 0;
	while (address < length) {
		const size_t write_length = (length - address >= EEPROMPageSizeInBytes) ? EEPROMPageSizeInBytes : (length - address);
		uint8_t write_buffer[EEPROMPageSizeInBytes + 1] = {};
		write_buffer[0] = static_cast<uint8_t>(address);
		memcpy(&write_buffer[1], &data[address], write_length);

		bool I2C_success;

		AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, EEPROMAddress + Write, /*send_length*/ static_cast<uint16_t>(write_length + 1), write_buffer, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);
		this_thread::sleep_for(chrono::milliseconds(1));
		address += write_length;
	}


	//Now read the data back to verify that it was written correctly
	vector<uint8_t> read_back(length);
	bool I2C_success = false;
	ReadEEPROMBytes(/*start_address*/ 0, read_back.data(), read_back.size(), I2C_success);

	if (I2C_success && read_back.size() == length && memcmp(data, read_back.data(), read_back.size()) == 0) {
		cout << "Wrote: " << string(data, length) << endl;
		cout << "EEPROM write verification succeeded for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ", " << length << " byte(s)." << endl;
	}
	else {
		cout << "EEPROM write verification failed for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< "." << endl;
		PrintEEPROMData("EEPROM data expected", reinterpret_cast<const uint8_t*>(data), length);
		PrintEEPROMData("EEPROM data read back", read_back.data(), read_back.size());
	}

	return true;
}


void ReadConfigEEPROM(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, char* data, size_t &length, bool &I2C_success) {
	if (data == nullptr) {
		cout << "EEPROM read failed: data pointer is null." << endl;
		return;
	}

	if (SlotNr >= NrSlots) {
		cout << "EEPROM read failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	if (length < EEPROMSizeInBytes) {
		cout << "EEPROM read failed: output buffer is too small. Need " << EEPROMSizeInBytes << " bytes." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);


	//Read the complete EEPROM contents starting from memory address 0.
	vector<uint8_t> read_back(EEPROMSizeInBytes);
	ReadEEPROMBytes(/*start_address*/ 0, read_back.data(), read_back.size(), I2C_success);

	memcpy(data, read_back.data(), read_back.size());
	//length = read_back.size();

	const void* endofstring = memchr(data, 0, read_back.size());
	length = endofstring ? static_cast<const char*>(endofstring) - data + 1 : 0;

	
}


void WriteConfigAddress(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, const uint8_t address) {

	if (SlotNr >= NrSlots) {
		cout << "Config address write failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Now write the address to the I2C 8-bit IO chip PCF8574AP, which has all 3 address lines on ground. See datasheet in folder datasheet.
	uint8_t write_value = address;
	bool I2C_success;
	AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, ConfigAddressIOExpanderAddress + Write, /*send_length*/ 1, &write_value, /*receive_length*/ 0, nullptr, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);

	// Now verify by reading the address back. Display an error message if no success.
	uint8_t read_back = 0;
	AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, ConfigAddressIOExpanderAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ 1, &read_back, I2CClockFrequencyInHz, I2C_success, /*fail_silently*/ true);

	if (read_back == address) {
		cout << "Config address write verification succeeded for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ": 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
	}
	else {
		cout << "Config address write verification failed for rack " << static_cast<unsigned int>(RackNr)
			<< ", slot " << static_cast<unsigned int>(SlotNr)
			<< ". Wrote 0x" << hex << static_cast<unsigned int>(address)
			<< ", read back 0x" << static_cast<unsigned int>(read_back) << dec << "." << endl;
	}

}


void ReadConfigAddress(const uint8_t SequencerID, const uint8_t RackNr, const uint8_t SlotNr, uint8_t &address, bool &I2C_success) {

	if (SlotNr >= NrSlots) {
		cout << "Config address read failed: invalid slot number " << static_cast<unsigned int>(SlotNr) << "." << endl;
		return;
	}

	SelectRackI2CSlot(SequencerID, RackNr, SlotNr);
	
	// Read the address from the I2C 8-bit IO chip PCF8574AP, which has all 3 address lines on ground. See datasheet in folder datasheet.
	vector<uint8_t> read_back(1);
	AUTO_CONFIG_API_CALL(TransmitI2CPort, /*I2C_port*/ 0, /*I2C_destination*/ 0, ConfigAddressIOExpanderAddress + Read, /*send_length*/ 0, nullptr, /*receive_length*/ static_cast<uint16_t>(read_back.size()), read_back.data(), I2CClockFrequencyInHz, I2C_success, /* fail_silently */ true);
	address = read_back[0];
	
	// Display the address on cout.
	//cout << "Config address read succeeded for rack " << static_cast<unsigned int>(RackNr)
	//	<< ", slot " << static_cast<unsigned int>(SlotNr)
	//	<< ": 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
	//cout << " Address: 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
}

json ReadConfiguration(const std::string& filename) {
	json config;

	AUTO_CONFIG_API_CALL(SwitchDebugMode, false, ""); //Debug mode slows I2C communication considerably. Only keep on if you really do debug it.
	//go over every rack slot and the backplane memory, constructs json file containing whole configuration, including addresses stored in EEPROMS, sequencer, rack and slot number of each board or rack beackplane.
	//store in file if filename is not empty.
	for (uint8_t SequencerNr = 0 ; SequencerNr < AUTO_CONFIG_API_CALL(GetNumberOfSequencers); ++SequencerNr) {
		uint8_t RackNr = 0;
		bool LastRackEncountered = false;
		while ((RackNr <= MaxSupportedRackNr) & (!LastRackEncountered)) {
			uint8_t SlotNr = NrSlots - 1;
			bool FinalSlotEncounterd = false;
			while (!FinalSlotEncounterd) {

				char buffer[EEPROMSizeInBytes] = {};
				size_t length = sizeof(buffer);
				bool I2C_success_EEPROM = false;
				ReadConfigEEPROM(SequencerNr, RackNr, SlotNr, buffer, length, I2C_success_EEPROM);
				if (SlotNr == (NrSlots - 1)) {
					LastRackEncountered = (!I2C_success_EEPROM) || (length == 1);
				}
				if (length > 1) {

					cout << "Rack " << static_cast<unsigned int>(RackNr)
						<< ", slot " << static_cast<unsigned int>(SlotNr)
						<< ", EEPROM read length: " << length << " byte(s): ";
					cout << buffer;//PrintEEPROMData("EEPROM data read back", read_back.data(), read_back.size());


					uint8_t address = 0;
					bool I2C_success_Address = false;
					ReadConfigAddress(SequencerNr, RackNr, SlotNr, address, I2C_success_Address);
					if (I2C_success_Address) cout << " Address: 0x" << hex << static_cast<unsigned int>(address) << dec << "." << endl;
					else cout << " No address." << endl;
					size_t json_length = 0;
					while (json_length < length && buffer[json_length] != '\0') {
						++json_length;
					}

					std::string json_str(buffer, json_length);
					if (!json_str.empty()) {
						try {
							json slot_config = json::parse(json_str);
							if (I2C_success_Address) {
								slot_config["Address"] = address;
							}
							if (SlotNr == NrSlots - 1) {
								config["Sequencer" + std::to_string(SequencerNr)]["Rack" + std::to_string(RackNr)] = slot_config;
							}
							else {
								config["Sequencer" + std::to_string(SequencerNr)]["Rack" + std::to_string(RackNr)]["Slot" + std::to_string(SlotNr)] = slot_config;
							}
						}
						catch (const json::parse_error& e) {
							cout << "Failed to parse JSON from EEPROM of sequencer " << static_cast<unsigned int>(SequencerNr)
								<< ", rack " << static_cast<unsigned int>(RackNr)
								<< ", slot " << static_cast<unsigned int>(SlotNr)
								<< ": " << e.what() << endl;
						}
					}
				}
				if (SlotNr == (NrSlots - 1)) SlotNr = 0;
				else if (SlotNr < (NrSlots-2)) SlotNr++;
				else FinalSlotEncounterd = true;
			}
			RackNr++;
		}
	}

	if (!filename.empty()) {
		//add ".json" to the filename if it doesn't already end with ".json"
		std::string output_filename = filename;
		if (output_filename.size() < 5 || output_filename.substr(output_filename.size() - 5) != ".json") {
			output_filename += ".json";
		}
		std::ofstream file(output_filename);
		if (file.is_open()) {
			file << config.dump(4); // pretty print with 4 spaces indent
			file.close();
			cout << "Configuration saved to " << output_filename << endl;
		}
		else {
			cout << "Failed to open file " << output_filename << " for writing." << endl;
		}
	}
	
	return config;
}

json GetAutoConfigJSON(const std::string& filename) {

	json discovered_config = ReadConfiguration(filename);
	json auto_config = {
		{"FileOrigin", "This file is automatically generated by GetAutoConfigJSON. Do not edit it manually."},
		{"ConfigurationName", "AutoConfig"},
		{"PCSequenceBufferSize", 134217728},
		{"LineFrequency", 50},
		{"Sequencers", json::array()},
		{"Rack", json::array()},
		{"AnalogOutBoards16bit", json::array()},
		{"DigitalOutBoards", json::array()},
		{"SerialPortBoards", json::array()},
		{"DDSAD9854Boards", json::array()},
		{"DDSAD9858Boards", json::array()},
		{"DDSAD9959Boards", json::array()},
		{"AnalogInBoards12bit", json::array()}
	};

	for (uint8_t SequencerNr = 0; SequencerNr < AUTO_CONFIG_API_CALL(GetNumberOfSequencers); ++SequencerNr) {
		const std::string sequencer_key = "Sequencer" + std::to_string(SequencerNr);
		if (!discovered_config.contains(sequencer_key)) {
			continue;
		}

		for (uint8_t RackNr = 0; RackNr <= MaxSupportedRackNr; ++RackNr) {
			const std::string rack_key = "Rack" + std::to_string(RackNr);
			if (!discovered_config[sequencer_key].contains(rack_key)) {
				continue;
			}

			const json& rack_json = discovered_config[sequencer_key][rack_key];

			for (uint8_t SlotNr = 0; SlotNr < NrSlots; ++SlotNr) {
				const json* board_json = nullptr;
				if (SlotNr == NrSlots - 1) {
					if (rack_json.is_object() && rack_json.contains("Address")) {
						board_json = &rack_json;
					}
				}
				else {
					const std::string slot_key = "Slot" + std::to_string(SlotNr);
					if (rack_json.contains(slot_key)) {
						board_json = &rack_json[slot_key];
					}
				}

				if (board_json == nullptr || !board_json->is_object()) {
					continue;
				}

				const std::string model_prefix = GetModelPrefix(*board_json);
				if (model_prefix.empty()) {
					continue;
				}

				if (model_prefix == "AnalogOut16bit") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["StartAddress"] = (*board_json).value("Address", 24);
					entry["NumberChannels"] = (*board_json).value("NumberChannels", 4);
					entry["Signed"] = (*board_json).value("Signed", true);
					entry["MinVoltage"] = (*board_json).value("MinVoltage", -10.0);
					entry["MaxVoltage"] = (*board_json).value("MaxVoltage", 10.0);
					auto_config["AnalogOutBoards16bit"].push_back(entry);
				}
				else if (model_prefix == "DigitalOut") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 1);
					entry["NumberChannels"] = (*board_json).value("NumberChannels", 16);
					auto_config["DigitalOutBoards"].push_back(entry);
				}
				else if (model_prefix == "SerialPort") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 1);
					auto_config["SerialPortBoards"].push_back(entry);
				}
				else if (model_prefix == "DDSAD9854") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 132);
					entry["Version"] = (*board_json).value("Version", 2);
					if (board_json->contains("ExternalClockFrequency")) {
						entry["ExternalClockFrequency"] = (*board_json)["ExternalClockFrequency"];
					}
					else if (board_json->contains("ExternalClockFrequencyinMHz")) {
						entry["ExternalClockFrequency"] = (*board_json)["ExternalClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ExternalClockFrequencyinMHz"] = (*board_json)["ExternalClockFrequencyinMHz"];
					}
					else {
						entry["ExternalClockFrequency"] = 300000000.0;
					}
					entry["PLLReferenceMultiplier"] = (*board_json).value("PLLReferenceMultiplier", 1);
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9854Boards"].push_back(entry);
				}
				else if (model_prefix == "DDSAD9858") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 50);
					if (board_json->contains("ClockFrequency")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequency"];
					}
					else if (board_json->contains("ClockFrequencyinMHz")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ClockFrequencyinMHz"] = (*board_json)["ClockFrequencyinMHz"];
					}
					else {
						entry["ClockFrequency"] = 300000000.0;
					}
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9858Boards"].push_back(entry);
				}
				else if ((model_prefix == "DDSAD9958") || (model_prefix == "DDSAD9959")) {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["Address"] = (*board_json).value("Address", 21);
					entry["Version"] = (*board_json).value("Version", 21);
					if (board_json->contains("ClockFrequency")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequency"];
					}
					else if (board_json->contains("ClockFrequencyinMHz")) {
						entry["ClockFrequency"] = (*board_json)["ClockFrequencyinMHz"].get<double>() * 1e6;
						entry["ClockFrequencyinMHz"] = (*board_json)["ClockFrequencyinMHz"];
					}
					else {
						entry["ClockFrequency"] = 300000000.0;
					}
					entry["FrequencyMultiplier"] = (*board_json).value("FrequencyMultiplier", 1);
					auto_config["DDSAD9959Boards"].push_back(entry);
				}
				else if (model_prefix == "AnalogIn12bit") {
					json entry;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					entry["ChipSelect"] = (*board_json).value("ChipSelect", (*board_json).value("Address", 1));
					entry["Signed"] = (*board_json).value("Signed", false);
					entry["MinVoltage"] = (*board_json).value("MinVoltage", 0.0);
					entry["MaxVoltage"] = (*board_json).value("MaxVoltage", 10.0);
					auto_config["AnalogInBoards12bit"].push_back(entry);
				}
				else {
					json entry = *board_json;
					AddCommonMetadata(*board_json, entry, SequencerNr, RackNr, SlotNr);
					auto_config["Rack"].push_back(entry);
				}
			}
		}
	}

	if (!filename.empty()) {
		const std::string output_filename = MakeConfigOutputFilename(filename);
		std::ofstream file(output_filename);
		if (file.is_open()) {
			file << auto_config.dump(4);
			file.close();
			cout << "Auto configuration saved to " << output_filename << endl;
			WriteAutoConfigCreators(auto_config, filename);
		}
		else {
			cout << "Failed to open file " << output_filename << " for writing." << endl;
		}
	}

	return auto_config;
}
