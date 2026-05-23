#pragma once

#include <cstdint>


//#define DebugSPI

#ifdef DebugSPI
#include <fstream>
#endif

class CDeviceSequencer;

class CMultiIO;

typedef enum
{
	E_SPI_clock_bit_banged,
	E_SPI_clock_FPGA
} E_SPI_clock_type;


const unsigned int MultiWriteDeviceSPIMaxBusBuffer = 1024*128;

class CMultiWriteDeviceSPI
{
//things formerly inherited from CMultiWriteDevice
public:
	bool ForceWriting;
	unsigned short MultiIOAddress;
	bool Enabled;
public:
	void SwitchForceWritingMode(bool OnOff);
	void Enable(bool OnOff);
//end things from CMultiWriteDevice

private:
	unsigned long BaseAddress;
	unsigned short Bus;
	//Ring buffer for bus writing
	uint16_t BusBuffer[MultiWriteDeviceSPIMaxBusBuffer];
	bool BusBufferSPICreateClock[MultiWriteDeviceSPIMaxBusBuffer];
	unsigned long BusBufferStart;
	unsigned long BusBufferEnd;
	unsigned long BusBufferLength;

	unsigned char SPI_CS_bit; 
	unsigned char SDIO_0_bit;
	unsigned char SDIO_1_bit;
	unsigned char SDIO_2_bit;
	unsigned char SDIO_3_bit;
	unsigned char SPI_SCLK_bit;
	bool QSPIMode;
	bool SPI_CPHA;
	bool SPI_CPOL;
	E_SPI_clock_type SPI_clock_type;
	double SPI_frequency_in_Hz; //0 means half of parallel bus speed
	uint8_t SPI_mode;
	E_SPI_clock_type Current_SPI_clock_type;
#ifdef DebugSPI
	std::ofstream* DebugFile;
#endif
public:
	unsigned short ControlRegisterContent;
	CDeviceSequencer* MyDeviceSequencer;
public:		
	virtual bool WriteToBus();
	void WriteAllToBus(bool End_SPI_clock_node = true);
	CMultiWriteDeviceSPI(unsigned short aBus, unsigned long aBaseAddress, CDeviceSequencer* _MyDeviceSequencer);
	~CMultiWriteDeviceSPI();
	void ConfigureSPI(unsigned char _SPI_CS_bit, unsigned char  _SDIO_0_bit, unsigned char _SDIO_1_bit, unsigned char _SDIO_2_bit, unsigned char _SDIO_3_bit, unsigned char _SPI_SCLK_bit, E_SPI_clock_type _SPI_type = E_SPI_clock_FPGA);
	void AddToBusBuffer(unsigned short value, bool CreateSPIClock = false);
	void SetControlRegister(unsigned char start_bit, unsigned char nr_bits, unsigned short value, bool write_immediately = true, bool CreateSPIClock = false);
	void SetSPIClock(bool clock, bool write_immediately = true);
	void SetSPIChipSelect(bool clock, bool write_immediately);
	void SetSPIDataOut(bool clock);
	void SetSPIFrequencyAndMode(double _SPI_frequency_in_Hz, const uint8_t _SPI_mode);
	void WriteSPIBitBanged(unsigned int number_of_bits_out, uint64_t data);
	void WriteSPIBitBangedFPGAClock(unsigned int number_of_bits_out, uint64_t data);
	void WriteSPIBitBangedMode0Simple(unsigned int number_of_bits_out, uint64_t data);
	virtual void SetQSPIMode(bool OnOff);
private:
	void AssureMinimumSPIClockPeriodLength();
};



//virtual bool HasSomethingToWriteToBus() { //inline code for speed
//	if (!Enabled) return false;
//	if (BusBufferLength == 0) return false;
///	return true;
//}