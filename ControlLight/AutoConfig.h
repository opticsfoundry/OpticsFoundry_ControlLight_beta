#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "include/json.hpp"

using json = nlohmann::json;

#ifdef API_CLASS
class ControlLight_API;
void SetAutoConfigAPI(ControlLight_API* api);
#endif

bool WriteConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, const char* data, size_t length);
void ReadConfigEEPROM(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, char* data, size_t& length, bool& I2C_success);
void WriteConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t address);
void ReadConfigAddress(uint8_t SequencerID, uint8_t RackNr, uint8_t SlotNr, uint8_t& address, bool& I2C_success);
json ReadConfiguration(const std::string& filename = "");
json GetAutoConfigJSON(const std::string& filename = "");
