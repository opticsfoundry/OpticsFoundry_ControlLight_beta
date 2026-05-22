#pragma once

#include "MultiWriteDevice.h"
#include <cstdint>


//#define DebugSPI

#ifdef DebugSPI
#include <fstream>
#endif

class CDeviceSequencer;

class CMultiIO;

const unsigned int MultiWriteDeviceSPIMaxBusBuffer = 1024*128;

class CMultiWriteDeviceSPI : public CMultiWriteDevice
{
private:
	unsigned long BaseAddress;
	unsigned short Bus;
	//Ring buffer for bus writing
	unsigned short BusBuffer[MultiWriteDeviceSPIMaxBusBuffer];
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
	double SPI_frequency_in_Hz; //0 means half of parallel bus speed
	uint8_t SPI_mode;
#ifdef DebugSPI
	std::ofstream* DebugFile;
#endif
public:
	unsigned short ControlRegisterContent;
	CDeviceSequencer* MyDeviceSequencer;
public:		
	virtual bool WriteToBus();
	void WriteAllToBus();
	CMultiWriteDeviceSPI(unsigned short aBus, unsigned long aBaseAddress, CDeviceSequencer* _MyDeviceSequencer);
	~CMultiWriteDeviceSPI();
	void SetSPIPortBits(unsigned char _SPI_CS_bit, unsigned char  _SDIO_0_bit, unsigned char _SDIO_1_bit, unsigned char _SDIO_2_bit, unsigned char _SDIO_3_bit, unsigned char _SPI_SCLK_bit);
	void AddToBusBuffer(unsigned short value);
	virtual bool HasSomethingToWriteToBus() { //inline code for speed
		if (!Enabled) return false;
		if (BusBufferLength == 0) return false;
		return true;
	}
	void SetControlRegister(unsigned char start_bit, unsigned char nr_bits, unsigned short value, bool write_immediately = true);
	void SetSPIClock(bool clock, bool write_immediately = true);
	void SetSPIChipSelect(bool clock);
	void SetSPIDataOut(bool clock);
	void SetSPIFrequencyAndMode(double _SPI_frequency_in_Hz, const uint8_t _SPI_mode);
	void WriteSPIBitBanged(unsigned int number_of_bits_out, uint64_t data);
	void WriteSPIBitBangedMode0Simple(unsigned int number_of_bits_out, uint64_t data);
	virtual void SetQSPIMode(bool OnOff);
private:
	void AssureMinimumSPIClockPeriodLength();
};
