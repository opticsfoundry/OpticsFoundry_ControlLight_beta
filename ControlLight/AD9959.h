#pragma once
#include "MultiWriteDeviceSPI.h"
#include <cstdint> // Include this header to define uint8_t


class CDeviceSequencer;

/*! typedef enum E_AD9959_PWR
    \brief DDS Power modes.
*/
typedef enum
{
  E_AD9959_PWR_POWERED, /*!< Fully powered */
  E_AD9959_PWR_PARTIAL, /*!< Fast recovery   - Powers down digital logic and DAC digital logic */
  E_AD9959_PWR_DOWN     /*!< Full power down - Powers down all functions including DAC and PLL */
} E_AD9959_PWR;

/*! typedef enum E_AD9959_CHANNEL
    \brief DDS Channel.
*/
typedef enum
{
  E_AD9959_CHANNEL_ALL, /*!< Select both channels */
  E_AD9959_CHANNEL_0,    /*!< Select channel 0 only */
  E_AD9959_CHANNEL_1,     /*!< Select channel 1 only */
  E_AD9959_CHANNEL_2,     /*!< Select channel 2 only */
  E_AD9959_CHANNEL_3     /*!< Select channel 3 only */
} E_AD9959_CHANNEL;

// Pin defines - Change to suit
#define PWR_DWN_CTL_PORT     GPIOA
#define PWR_DWN_CTL_PIN      GPIO_PIN_1
#define MASTER_RESET_PORT    GPIOA
#define MASTER_RESET_PIN     GPIO_PIN_1
#define IO_UPDATE_PORT       GPIOA
#define IO_UPDATE_PIN        GPIO_PIN_1
#define SDIO_3_PORT          GPIOA
#define SDIO_3_PIN           GPIO_PIN_1
#define CS_GPIO_PORT         GPIOA
#define CS_PIN               GPIO_PIN_1



const unsigned int SPIBufferLength = 14;
constexpr unsigned int AD9959NumberOfRegisters = 92;

constexpr unsigned int AD9959SPIBufferLength = 14;

class CAD9959 : public CMultiWriteDeviceSPI
{
private:
    uint8_t SPIBuffer[AD9959SPIBufferLength];
    unsigned char BytesToTransmit;
    double ClockFrequency_in_Hz;
    double InputClockFrequency_in_Hz;
    double MaxFrequency;
    double FrequencyScale;
    bool UpdateRegistersModeAutomatic;
    bool AD9958;
    double version;
    bool IOUpdateEnabled;
    
    uint32_t AktValueContents[AD9959NumberOfRegisters]; //keeps track of Value, contains value after bus buffer has been finished to be written out
    unsigned char WritePrecision[AD9959NumberOfRegisters];
public:
    double FrequencyMultiplier;
private:
    unsigned char SPI_IOUpdate_bit;

public:
    CAD9959(unsigned short aBus, unsigned long aBaseAddress, double aExternalClockFrequency_in_Hz, double aFrequencyMultiplier, bool aAD9958, double aversion, CDeviceSequencer* _MyDeviceSequencer);
    
    virtual ~CAD9959();
    void SetIOUpdate(bool OnOff);
    void SetReset(bool OnOff);
    void SetPowerDown(bool OnOff, bool write_immediately = true);
    void SetSyncIO(bool OnOff);
    void SetP0(bool OnOff);
    void SetP1(bool OnOff);
    void SetP2(bool OnOff);
    void SetP3(bool OnOff);
    void SetSpare2(bool OnOff);
    void SetSpare1(bool OnOff);
    void SetSpare0(bool OnOff);
    void SetSDIO_3(bool OnOff);
    void SetSDIO_2(bool OnOff);
    void SetSDIO_1(bool OnOff);

    void Initialise(void);
    void SyncIO(void);
    void SetQSPIMode(bool OnOff);
//    void Initialise_GPIO(void);
    //void TestFunctions(void);

    /*! void SetFrequency(uint8_t channel, float frequency)
        \brief Set the frequency output on a given channel of the DDS.

		\param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param frequency Desired output frequency in MHz.
    */
    void SetFrequency(uint8_t channel, float frequency);


    /*! void SetAmplitude(uint8_t channel, float amplitude)
        \brief Set the amplitude scale factor on a given channel of the DDS.

        \param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param amplitude Desired amplitude scale factor (0-100).
    */
    void SetAmplitude(uint8_t channel, float amplitude);

	void SetAttenuation(uint8_t channel, double attenuation);

    /*! void DDS_Set_Phase_Offset(uint8_t channel, float phase)
        \brief Set the phase offset on a given channel of the DDS.

        \param channel Which channel to output on: 1: channel 0, 2: channel 1, 3: channel 0 and 1.
        \param phase Desired phase offset.
    */
    void SetPhaseOffset(uint8_t channel, float phase);

    /*! void DDS_Set_Power_State(E_AD9959_PWR state)
        \brief Set the power state of the DDS.

        \param state Desired power state.
    */
    void SetPowerState(E_AD9959_PWR state);


    void SetFrequencyCh0(double frequency);
    void SetFrequencyCh1(double frequency);
    void SetFrequencyCh2(double frequency);
    void SetFrequencyCh3(double frequency);
    void SetIntensityCh0(double Intensity);
    void SetIntensityCh1(double Intensity);
    void SetIntensityCh2(double Intensity);
    void SetIntensityCh3(double Intensity);

    void SetAttenuationCh0(double Attenuation);
	void SetAttenuationCh1(double Attenuation);
    void SetAttenuationCh2(double Attenuation);
    void SetAttenuationCh3(double Attenuation);
    void SetPhaseOffsetCh0(double phase);
    void SetPhaseOffsetCh1(double phase);
    void SetPhaseOffsetCh2(double phase);
    void SetPhaseOffsetCh3(double phase);

    void SelectChannelCh0() { if (AD9958) SetRegisterBits(0, 6, 2, 1); else SetRegisterBits(0, 4, 4, 1); }
    void SelectChannelCh1() { if (AD9958) SetRegisterBits(0, 6, 2, 2); else SetRegisterBits(0, 4, 4, 2); }
    void SelectChannelCh2() { if (!AD9958) SetRegisterBits(0, 4, 4, 4); }
    void SelectChannelCh3() { if (!AD9958) SetRegisterBits(0, 4, 4, 8); }
    void SelectChannelCh0And1() { if (AD9958) SetRegisterBits(0, 6, 2, 3); else SetRegisterBits(0, 4, 4, 3); }
    void SelectChannelCh0To3() { if (!AD9958) SetRegisterBits(0, 4, 4, 0xF); }
    void SelectChannels(uint8_t channels) { if (AD9958) SetRegisterBits(0, 6, 2, 0x3 & channels); else SetRegisterBits(0, 4, 4, 0xF & channels); }

    //void SetFrequencyTuningWord(uint32_t ftw) { SetValue(4, ftw); }
    void SetFrequencyTuningWord(uint8_t channel, uint32_t ftw) { if ((AD9958 && (channel < 2)) || ((!AD9958) && (channel < 4))) { SetWriteChannels(channel);  SetValue(4, ftw); } }
    void SetFrequencyTuningWordCh0(uint32_t ftw) {
        SetFrequencyTuningWord(1, ftw);
    }
    void SetFrequencyTuningWordCh1(uint32_t ftw) {
        SetFrequencyTuningWord(2, ftw);
    }
    void SetFrequencyTuningWordCh2(uint32_t ftw) {
        if (!AD9958) SetFrequencyTuningWord(3, ftw);
    }
    void SetFrequencyTuningWordCh3(uint32_t ftw) {
        if (!AD9958) SetFrequencyTuningWord(4, ftw);
    }
    void SetIOUpdateEnabled(bool _IOUpdateEnabled);
private:
    void IO_Update_Toggle();
    //void Reset(void);
    void Disable_SYNC_CLK(void);



    void SetFrequency_SPI(uint8_t channel, float frequency);
    void SetAmplitude_SPI(uint8_t channel, uint32_t amplitude);
    void SetIntensity_SPI(uint8_t channel, uint32_t intensity);
    void SetPhaseOffset_SPI(uint8_t channel, float phase);
    void SetPowerDown_partial_SPI();
    void SetPowerDown_full_SPI();
    uint8_t GetChannelBits(E_AD9959_CHANNEL channel);
    uint32_t calcFTW(float frequency);
    float calcFrequency(uint32_t FTW);
    uint32_t calcPOW(float degrees);
    void Dev_Select(void);
    void Dev_Deselect(bool read = false, uint8_t number_of_bits_in = 0);
    void SPI_Transmit_Byte(uint8_t byte);
    void AssurePulseIsLongerThanSyncClockPeriod();
public:
    void SetWriteChannels(uint8_t channels);
    void SetWriteChannels(bool channel0, bool channel1, bool channel2, bool channel3);
    //Functions for CMultiWriteDevice
    void MasterReset();
    
    //bool SetControlBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue);
    bool SetRegisterBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue = false, bool DoIOUpdate = true);
    uint32_t SetRegisterBits(unsigned char RegisterNr, unsigned char LowestBitNr, unsigned char NrBits, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true, bool forceWrite= false);
    uint32_t SetValueDirect(unsigned char RegisterNr, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true, bool forceWrite = false);
    uint32_t SetValue(unsigned char RegisterNr, uint32_t Value, bool GetValue = false, bool DoIOUpdate = true, bool forceWrite = false);
    //void LoadLatches();
    //void UpdateRegisters();
};
