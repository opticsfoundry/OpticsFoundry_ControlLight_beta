#pragma once

#include <optional>
#include <string>

#include "include/json.hpp"

using json = nlohmann::json;

class ConfigCreator {
public:
	explicit ConfigCreator(const std::string& filename = "ControlHardwareConfig_AQuRA.json");

	void RegisterSequencer(int Id = 0, const std::string& Type = "OpticsFoundrySequencerV1",
		const std::string& IP = "192.168.0.104", int Port = 7, bool Master = true,
		int StartDelay = 10, int ClockFrequencyinMHz = 100, int StrobeDurationInFPGAClockPeriods = 15,
		bool UseExternalClock = false, bool UseStrobeGenerator = true, bool UseEdgeTriggeredLatches = true, bool Connect = true,
		bool DebugOn = false, const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterAnalogOutBoard16bit(int Sequencer = 0, int StartAddress = 24, int NumberChannels = 4,
		bool Signed = true, double MinVoltage = -10, double MaxVoltage = 10,
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterDigitalOutBoard(int Sequencer = 0, int Address = 1, int NumberChannels = 16,
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterSerialPortBoard(int Sequencer = 0, int Address = 1, int RackNr = 0, int SlotNr = 0,
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt);

	void RegisterDDSAD9854Board(int Version = 2, int Sequencer = 0, int Address = 132,
		double ExternalClockFrequencyinMHz = 300, int PLLReferenceMultiplier = 1,
		double FrequencyMultiplier = 1, const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterDDSAD9858Board(int Sequencer = 0, int Address = 50, double ClockFrequencyinMHz = 1200,
		double FrequencyMultiplier = 1, const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterDDSAD9959Board(int Sequencer = 0, int Address = 21, double ClockFrequencyinMHz = 300,
		double FrequencyMultiplier = 1, bool AD9958 = false, double version = 0, 
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterAnalogInBoard12bit(int Sequencer = 0, int Address = 80, int NumberChannels = 4,
		double MinVoltage = -10, double MaxVoltage = 10,
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);

	void RegisterRackEntry(const json& entry);

	bool Save() const;

private:
	std::string filename_;
	json config_;

	static void AddOptionalHardwareFields(json& entry,
		const std::optional<std::string>& Model = std::nullopt,
		const std::optional<std::string>& SN = std::nullopt,
		const std::optional<int>& RackNr = std::nullopt,
		const std::optional<int>& SlotNr = std::nullopt);
};
