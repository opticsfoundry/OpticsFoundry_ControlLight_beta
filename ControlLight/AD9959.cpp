// AD9959.cpp: implementation of the CAD9959 class.
//
//////////////////////////////////////////////////////////////////////

#include "AD9959.h"
#include "ControlAPI.h"
#include "CDeviceSequencer.h"
#include "std.h"
#include <string>
#include <format>
#include <cmath>

#define DefaultQSPIMode true

// Register addresses, writing to (MSB is 0 for a write)
#define CSR     0x00            //!< Channel select register            1 Byte
#define FR1     0x01            //!< Function register 1                3 Bytes
#define CFR     0x03            //!< Channel Function register          3 Bytes
#define CFTW0   0x04            //!< Channel Frequency Tuning Word      4 Bytes
#define CPOW    0x05            //!< Channel Phase Offset Word          2 Bytes
#define ACR     0x06            //!< Amplitude Control Register         3 Bytes







//AktValueContents[0..2] is the control register
//AktValueContents[3..24] is the channel 0 register map. This function allows writing to these registers, 
//  [AD9958: which writes to channel 0 and/or 1 depending on bit 6 and 7 of the CSR]
//  [AD9959: which writes to channel 0, 1, 2 and/or 3 depending on bit 4, 5, 6 and 7 of the CSR]. 
//  When reading it provides values of channel 0
//AktValueContents[25..46] is the channel 1 register map
//AktValueContents[47..68] is the channel 2 register map
//AktValueContents[69..90] is the channel 3 register map
//AktValueContents[91] is the digital out port (not part of the AD9959, but placed on the same circuit board)
uint32_t AD9959MasterResetValueContents[AD9959NumberOfRegisters] = { 0xF0,   0,   0,   0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0x000302,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };
unsigned char AD9959ValueLength[AD9959NumberOfRegisters] = { 1,   3,   2,          3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,           3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,           3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,           3,   4,   2,   3,   2,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   2 };
unsigned char AD9959ValueBaseAddress[AD9959NumberOfRegisters] = { 0x00,0x01,0x02,       0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,        0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,        0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,           0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19 };
const unsigned long AD9959ValueNotInBusBuffer = 99999;


// Public Functions --------------------------------------------------

void AD9959DoNothing(bool bit) {}

void AD9959WaitNothing(double time) {}

uint32_t AD9959WriteReadSPINothing(unsigned int chip_select, unsigned int number_of_bits_out, uint64_t data_high, uint64_t data_low, unsigned int number_of_bits_in) { return 0; }




//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAD9959::CAD9959(unsigned short aBus, unsigned long aBaseAddress, double aExternalClockFrequency_in_Hz, double aFrequencyMultiplier, bool aAD9958, double aversion, CDeviceSequencer* _MyDeviceSequencer)
    : CMultiWriteDeviceSPI(aBus, aBaseAddress, _MyDeviceSequencer) {
    version = aversion;
    if (version < 0.17) {
        ConfigureSPI(/* SPI_CS_bit*/14, /*SDIO_0_bit = SPI_MOSI_bit*/ 12, /*SDIO_1_bit = */ 11, /*SDIO_2_bit */ 10, /*SDIO_3_bit = Sync_IO */ 9, /*SPI_SCLK_bit*/ 13, /*SPI_clock_type*/ E_SPI_clock_bit_banged);  //SDIO_3 is the sync_io pin
        SPI_IOUpdate_bit = 15;
    }
    else {
        ConfigureSPI(/* SPI_CS_bit*/14, /*SDIO_0_bit = SPI_MOSI_bit*/ 12, /*SDIO_1_bit = */ 11, /*SDIO_2_bit */ 10, /*SDIO_3_bit = Sync_IO */ 9, /*SPI_SCLK_bit*/ 15, /*SPI_clock_type*/ E_SPI_clock_FPGA);  //SDIO_3 is the sync_io pin
        SPI_IOUpdate_bit = 13;
    }
    BytesToTransmit = 0;

    InputClockFrequency_in_Hz = 1E6 * aExternalClockFrequency_in_Hz; //conversion MHz to Hz
    FrequencyMultiplier = aFrequencyMultiplier;  //This is the external frequency doubler option
    AD9958 = aAD9958;
    ClockFrequency_in_Hz = InputClockFrequency_in_Hz;
    MaxFrequency = ClockFrequency_in_Hz * 0.45E-6;
    //4294967296=2^32
    FrequencyScale = 4294967296.0 * ((1.0 / FrequencyMultiplier) * 1.0E6 / ClockFrequency_in_Hz);
    IOUpdateEnabled = true;


    for (int i = 0; i < AD9959NumberOfRegisters; i++) {
        AktValueContents[i] = AD9959MasterResetValueContents[i];
        WritePrecision[i] = AD9959ValueLength[i];
    }
    UpdateRegistersModeAutomatic = true;

    //SetSPIFrequencyAndMode(/*SPI_frequency_in_Hz*/ 200.0/500.0*InputClockFrequency_in_Hz, /*SPI_mode*/ 0);
    SetSPIFrequencyAndMode(/*SPI_frequency_in_Hz*/ 50*1000*1000, /*SPI_mode*/ 0);
    DefineInitialConditions();
}

CAD9959::~CAD9959() {

}

//AD9959 DDS0 digital port control lines

void CAD9959::SetIOUpdate(bool OnOff) {
    SetControlRegister(SPI_IOUpdate_bit, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSDIO_3(bool OnOff) {
    SetControlRegister(9, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSDIO_2(bool OnOff) {
    SetControlRegister(10, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSDIO_1(bool OnOff) {
    SetControlRegister(11, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSyncIO(bool OnOff) {  //identical to SDIO_3
    SetControlRegister(2, 1, (OnOff) ? 1 : 0); //ToDo: check this bit, it should probably be 9, not 2.
}

void CAD9959::SetP0(bool OnOff) {
    SetControlRegister(8, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetP1(bool OnOff) {
    SetControlRegister(7, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetP2(bool OnOff) {
    SetControlRegister(6, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetP3(bool OnOff) {
    SetControlRegister(5, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetReset(bool OnOff) {
    SetControlRegister(4, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetPowerDown(bool OnOff, bool write_immediately) {
    SetControlRegister(3, 1, (OnOff) ? 1 : 0, write_immediately);
}

void CAD9959::SetSpare2(bool OnOff) {
    SetControlRegister(2, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSpare1(bool OnOff) {
    SetControlRegister(1, 1, (OnOff) ? 1 : 0);
}

void CAD9959::SetSpare0(bool OnOff) {
    SetControlRegister(0, 1, (OnOff) ? 1 : 0);
}


void CAD9959::SyncIO(void) {
    SetSyncIO(true);
    SetSyncIO(false);
}

// Select the output frequency of desired channel/s
void CAD9959::SetFrequency(uint8_t channel, float frequency) //frequency in MHz
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;
    //float freq_Hz = (frequency * 1000000.0);
    SetFrequency_SPI(channel, frequency);
}

// Set the amplitude scale factor (max 1023)
void CAD9959::SetAmplitude(uint8_t channel, float amplitude) //amplitude in %
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;
    uint32_t amp = (uint32_t)(amplitude * 10.23);
    if (amp > 1023) amp = 1023;
    SetAmplitude_SPI(channel, amp);
}

void CAD9959::SetAttenuation(uint8_t channel, double Attenuation) {
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;
    if (Attenuation > 0) Attenuation = 0;
    double Intensity = (unsigned int)(0x03FF * pow(10.0, Attenuation / 20.0));
    SetAmplitude(channel, Intensity);
}

// Offset the desired channel from the other
void CAD9959::SetPhaseOffset(uint8_t channel, float phase) //phase in degree
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;
    SetPhaseOffset_SPI(channel, phase);
}



// DDS Power control
void CAD9959::SetPowerState(E_AD9959_PWR state)
{
    switch (state)
    {
    case E_AD9959_PWR_POWERED:
    {
        // Set PWR-DWN-CTL pin LOW to disable power-down control
        SetPowerDown(false);
    }
    break;

    case E_AD9959_PWR_PARTIAL:
    {
        SetPowerDown_partial_SPI();

        // Set PWR-DWN-CTL pin HIGH to enable the selected power-down mode
        SetPowerDown(true);
    }
    break;

    case E_AD9959_PWR_DOWN:
    {
        SetPowerDown_full_SPI();

        // Set PWR-DWN-CTL pin HIGH to enable the selected power-down mode
        SetPowerDown(true);
    }
    break;

    default:
        break;
    }
}



void CAD9959::SetFrequencyCh0(double frequency) {
    SetFrequency_SPI(1, frequency);
}

void CAD9959::SetFrequencyCh1(double frequency) {
    SetFrequency_SPI(2, frequency);
}

void CAD9959::SetFrequencyCh2(double frequency) {
    if (!AD9958) SetFrequency_SPI(3, frequency);
}

void CAD9959::SetFrequencyCh3(double frequency) {
    if (!AD9958) SetFrequency_SPI(4, frequency);
}

void CAD9959::SetIntensityCh0(double Intensity) {
    SetIntensity_SPI(1, Intensity);
}

void CAD9959::SetIntensityCh1(double Intensity) {
    SetIntensity_SPI(2, Intensity);
}

void CAD9959::SetIntensityCh2(double Intensity) {
    if (!AD9958) SetIntensity_SPI(3, Intensity);
}

void CAD9959::SetIntensityCh3(double Intensity) {
    if (!AD9958) SetIntensity_SPI(4, Intensity);
}

void CAD9959::SetAttenuationCh0(double Attenuation) {
    SetAttenuation(1, Attenuation);
}

void CAD9959::SetAttenuationCh1(double Attenuation) {
    SetAttenuation(2, Attenuation);
}

void CAD9959::SetAttenuationCh2(double Attenuation) {
    if (!AD9958) SetAttenuation(3, Attenuation);
}

void CAD9959::SetAttenuationCh3(double Attenuation) {
    if (!AD9958) SetAttenuation(4, Attenuation);
}

void CAD9959::SetPhaseOffsetCh0(double phase) {
    SetPhaseOffset_SPI(1, phase);
}

void CAD9959::SetPhaseOffsetCh1(double phase) {
    SetPhaseOffset_SPI(2, phase);
}

void CAD9959::SetPhaseOffsetCh2(double phase) {
    if (!AD9958) SetPhaseOffset_SPI(3, phase);
}

void CAD9959::SetPhaseOffsetCh3(double phase) {
    if (!AD9958) SetPhaseOffset_SPI(4, phase);
}

void CAD9959::SetQSPIMode(bool OnOff) {
    /*
    from datasheet:
    Note that when programming the device for 4-bit serial mode, it is important to keep the SDIO_3 pin at Logic 0 until the device is
    programmed out of the single-bit serial mode. Failure to do so can result in the serial I/O port controller being out of sequence.
    */
    SetSDIO_3(false);
    SetRegisterBits(/*RegisterNr*/CSR, /*LowestBitNr*/ 1, /* NrBits*/ 2, (OnOff) ? 3 : 0, /*GetValue*/false, /*DoIOUpdate*/ true, /*ForceWrite*/ false);
    DefineQSPIMode(OnOff);
}

// Private Functions --------------------------------------------------

/*! static uint8_t GetChannelBits(E_AD9959_CHANNEL channel)
    \brief Convert enum to bit pattern

    \param channel Desired DDS channel.
    \return Register bit pattern for a given channel.
*/


uint8_t CAD9959::GetChannelBits(E_AD9959_CHANNEL channel)
{
    //if (!((AD9958 && (channel < 2)) || ((!AD9958) && (channel < 4)))) return 0;

    uint8_t Channel = 0;

    switch (channel)
    {
    case E_AD9959_CHANNEL_ALL:
    {
        Channel = (AD9958) ? 3 : 0xF;
    }
    break;
    case E_AD9959_CHANNEL_0:
    {
        Channel = 1;
    }
    break;
    case E_AD9959_CHANNEL_1:
    {
        Channel = 2;
    }
    break;
    case E_AD9959_CHANNEL_2:
    {
        Channel = 3;
    }
    break;
    case E_AD9959_CHANNEL_3:
    {
        Channel = 4;
    }
    break;
    default:
        break;
    }

    return Channel;
}


#define PHASE_OFF_BITS  16384.0         //!< 2^14
#define MAX_DEGREES     360.0           //!< For phase calculation

/*! static uint32_t calcPOW(f32 degrees)
    \brief Calculates the Phase Offset Word.

    \param degrees (0-360).
    \return Phase Offset Word.
*/
uint32_t CAD9959::calcPOW(float degrees)
{
    uint32_t pow = 0;

    pow = (uint32_t)(degrees * (PHASE_OFF_BITS / MAX_DEGREES));

    return pow;
}


#define TUNEWORD_BITS   4294967296.0    //!< 2^32

/*! static uint32_t calcFTW(float frequency)
    \brief Calculates the Frequency Tuning Word.

    \param frequency Desired frequency.
    \return Frequency Tuning Word.
*/
uint32_t CAD9959::calcFTW(float frequency)
{
    uint32_t setPoint = 0;

    setPoint = (uint32_t)(frequency / (ClockFrequency_in_Hz / TUNEWORD_BITS) + 0.5);

    return setPoint;
}


float CAD9959::calcFrequency(uint32_t FTW)
{
    float frequency = 0;
    frequency = (FTW * (ClockFrequency_in_Hz / TUNEWORD_BITS));
    return frequency;
}



void CAD9959::SetIOUpdateEnabled(bool _IOUpdateEnabled) {
    IOUpdateEnabled = _IOUpdateEnabled;
}

/*! static void IO_Update_Toggle()
    \brief Toggles I/O Update pin on DDS to force register changes.

    \warning Minimum pulse width of > 1 SYNC_CLK period (~160ns) required.
*/
void CAD9959::IO_Update_Toggle(void)
{
    if (IOUpdateEnabled) {
        SetIOUpdate(true);
        // Minimum pulse width needs to be > 1 SYNC_CLK period (~160ns)
        AssurePulseIsLongerThanSyncClockPeriod();
        SetIOUpdate(false);
    }
}


// SPI Functions --------------------------------------------------


void CAD9959::Dev_Select(void)
{
    BytesToTransmit = 0;
}

void CAD9959::SPI_Transmit_Byte(uint8_t byte) {
    if (BytesToTransmit >= SPIBufferLength) {
        AddErrorMessage("CAD9959::SPI_Transmit_Byte : SPIBuffer full, dropping byte");
        return;
    }
    SPIBuffer[BytesToTransmit] = byte;
    BytesToTransmit++;
}

void CAD9959::Dev_Deselect(bool read, uint8_t number_of_bits_in)
{
    //SPIBuffer[0] is MSB, SPIBuffer[BytesToTransmit-1] is LSB
    //data_low must contain data in MSB first order
    uint64_t data_low = 0;
    for (unsigned char i = 0; i < BytesToTransmit; i++) {
        uint64_t help = SPIBuffer[i];
        data_low |= (help << (8 * ( BytesToTransmit - 1 - i ) ));
    }

    WriteSPIBitBanged(/*  number_of_bits_out*/BytesToTransmit * 8, /*uint64_t*/ data_low);
}

void CAD9959::Initialise(void)
{
    MasterReset();
}

void CAD9959::Disable_SYNC_CLK(void)
{
    SetRegisterBit(/*RegisterNr*/FR1, /*BitNr*/ 6, /*Value*/ false, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9959::SetFrequency_SPI(uint8_t channel, float frequency) //channel: 1...4 for AD9959 or 1..2 for AD9958 //frequency in MHz
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;
    float freq_Hz = (frequency * 1000000.0);
    uint32_t FTW = calcFTW(freq_Hz);
    SetWriteChannels(1 << (channel - 1));
    SetValueDirect(CFTW0, FTW,/*GetValue*/ false, /*DoIOUpdate*/true);
}

#define MAN_AMP         0x1000    //!< Enable manual amplitude control
void CAD9959::SetAmplitude_SPI(uint8_t channel, uint32_t amplitude)
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;


    SetWriteChannels(1 << (channel - 1));
    //SetValueDirect(ACR, (MAN_AMP << 8), /*GetValue*/ false, /*DoIOUpdate*/ false);
    SetValueDirect(ACR, (amplitude & 0x3ff) | MAN_AMP, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9959::SetIntensity_SPI(uint8_t channel, uint32_t intensity)
{
    if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;


    double Amplitude = sqrt(intensity) * 102.3;
    if (Amplitude > 1023) Amplitude = 1023;
    if (Amplitude < 0) Amplitude = 0;
    SetAmplitude_SPI(channel, Amplitude);
}


void CAD9959::SetPhaseOffset_SPI(uint8_t channel, float phase)
{
     if (channel < 1) return;
    if (!((AD9958 && (channel <= 2)) || ((!AD9958) && (channel <= 4)))) return;

    uint32_t POW = calcPOW(phase);
    SetWriteChannels(1 << (channel - 1));
    SetValueDirect(CPOW, POW, /*GetValue*/ false, /*DoIOUpdate*/ false);
    //clear phase accumulators of both channels
    SetWriteChannels((AD9958) ? 3 : 0xF);
    SetValueDirect(CFR, 0x000302, /*GetValue*/ false, /*DoIOUpdate*/ true);
    SetValueDirect(CFR, 0x000300, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

#define PWR_DWN_FULL    0x2    //!< Select full power down mode
#define PWR_DWN_PART    0x0    //!< Select partial power down mode
#define SYNC_DISABLE    0x1    //!< Disable the SYNC_CLK signal
void CAD9959::SetPowerDown_partial_SPI(void)
{
    SetRegisterBits(/*RegisterNr*/ FR1, /*LowestBitNr*/ 5, /* NrBits*/ 2, PWR_DWN_PART | SYNC_DISABLE, /*GetValue*/ false, /*DoIOUpdate*/ true);
}

void CAD9959::SetPowerDown_full_SPI(void)
{
    SetRegisterBits(/*RegisterNr*/ FR1, /*LowestBitNr*/ 5, /* NrBits*/ 2, PWR_DWN_FULL | SYNC_DISABLE, /*GetValue*/ false, /*DoIOUpdate*/ true);
}


void CAD9959::AssurePulseIsLongerThanSyncClockPeriod() {
    double MinimumResetPulseDuration_in_ms = 1000.0 / (ClockFrequency_in_Hz / 4.0);//SYNC_CLK frequency = ClockFrequency_in_Hz/4.0
    double BusPeriod_in_ms = 1000.0 / MyDeviceSequencer->BusFrequency_in_Hz;
    constexpr double safetymargin = 1.5;
    if (BusPeriod_in_ms < safetymargin * MinimumResetPulseDuration_in_ms) {
        WriteAllToBus();
        MyDeviceSequencer->Add_non_user_wait_ms(safetymargin * MinimumResetPulseDuration_in_ms);
        //CLA_Wait_ms(safetymargin * MinimumResetPulseDuration_in_ms);  
    }
}

void CAD9959::DefineInitialConditions() {
    for (int i = 0; i < AD9959NumberOfRegisters; i++) {
        AktValueContents[i] = AD9959MasterResetValueContents[i];
        WritePrecision[i] = AD9959ValueLength[i];
    }
    SetIOUpdateEnabled(true);
    DefineQSPIMode(DefaultQSPIMode);
    ControlRegisterContent = 0;
    //DefineSyncIO(false);
    //DefineReset(false);
    //DefinePowerDown(false);
}

void CAD9959::MasterReset() {
    if (!Enabled) return;
    for (int i = 0; i < AD9959NumberOfRegisters; i++) {
        AktValueContents[i] = AD9959MasterResetValueContents[i];
        WritePrecision[i] = AD9959ValueLength[i];
    }
    SetIOUpdateEnabled(true);
    DefineQSPIMode(false);
    // Toggle MASTER_RESET
    SetReset(true);
    // Minimum pulse width needs to be > 1 SYNC_CLK period (~160ns)
    AssurePulseIsLongerThanSyncClockPeriod();
    
    SetReset(false);
    // Set PWR-DWN-CTL pin LOW to disable power-down control
    SetPowerDown(false);
    SetSyncIO(false);
    // Toggle external power down pin
    // This resets the digital logic in the DDS to ensure SPI comms is possible
    SetPowerState(E_AD9959_PWR_DOWN);
    SetPowerState(E_AD9959_PWR_POWERED);
    // Not using multiple DDS devices so not needed
    Disable_SYNC_CLK();

    //Wait(10);
    //ToDo: check if SetSyncIO needed, seems not to have been on correct pin

    SetSyncIO(true);
    //Wait(0.001);
    //ToDo: check if SetSyncIO needed, seems not to have been on correct pin
    // sync_io low (To enable SPI reception)
    SetSyncIO(false);
    //Switch to QSPI mode
    SetQSPIMode(DefaultQSPIMode);
}

void CAD9959::SetWriteChannels(bool channel0, bool channel1, bool channel2, bool channel3) {
    uint8_t channelcode;
    channelcode = ((channel0) ? 1 : 0) + ((channel1) ? 2 : 0) + ((channel2) ? 4 : 0) + ((channel3) ? 8 : 0);
    SetWriteChannels(channelcode);
}

//channels = 1: write to channel 0
//channels = 2: write to channel 1
//channels = 4: write to channel 2
//channels = 8: write to channel 3
//or these channels, e.g. channels = 3: write to channel 0 and 1
void CAD9959::SetWriteChannels(uint8_t channels) {

    SetRegisterBits(/*RegisterNr*/CSR, /*LowestBitNr*/ (AD9958) ? 6 : 4, /* NrBits*/ (AD9958) ? 2 : 4, channels, /*GetValue*/false, /*DoIOUpdate*/ false, /*ForceWrite*/ false);
}

//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
//RegisterNr in [47..68]: write channel 0 and 1 register map 
//RegisterNr = 69: write & read digital out port of this board
bool CAD9959::SetRegisterBit(unsigned char RegisterNr, unsigned char BitNr, bool Value, bool GetValue, bool DoIOUpdate)
{
    return SetRegisterBits(RegisterNr, BitNr, 1, (Value) ? 1 : 0, GetValue) > 0;
}

//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
// //RegisterNr in [47..68]: write & read channel 2 register map
// //RegisterNr in [69..90]: write & read channel 3 register map
//RegisterNr in [91..112]: write channel 0, 1, 2 and 3 register map 
//RegisterNr = 113: write & read digital out port of this board
constexpr uint8_t ControlRegisterNr = 113;
uint32_t CAD9959::SetRegisterBits(unsigned char RegisterNr, unsigned char LowestBitNr, unsigned char NrBits, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return false;
    if (((RegisterNr > ControlRegisterNr) && (!GetValue)) || ((RegisterNr > 90) && (RegisterNr != ControlRegisterNr) && (GetValue))) {
        std::string  buf;
        if (GetValue) {
            buf = std::format(_T("CAD9959::SetRegisterBits : RegisterNr ({}) is not in range for reading, [0..90] or 113."), static_cast<unsigned int>(RegisterNr));
        }
        else {
            buf = std::format(_T("CAD9959::SetRegisterBits : RegisterNr ({}) is not in range for writing, [0..113]"), static_cast<unsigned int>(RegisterNr));
        }
        ControlMessageBox(buf);
        return 0;
    }
    uint8_t LengthTableRegisterNr = (RegisterNr < 91) ? RegisterNr : (RegisterNr < ControlRegisterNr) ? RegisterNr - 44 : 91;

    if (AD9959ValueLength[LengthTableRegisterNr] * 8 < NrBits) {
        std::string buf;
        buf = std::format(_T("CAD9959::SetRegisterBits : NrBits ({}) exceeds RegisterNr ({}) length ({})"),
            static_cast<unsigned int>(NrBits),
            static_cast<unsigned int>(RegisterNr),
            static_cast<unsigned int>(AD9959ValueLength[LengthTableRegisterNr] * 8));
        ControlMessageBox(buf);
        return 0;
    }
    uint32_t mask = 0xFFFFFFFF >> (32 - NrBits);
    uint8_t ReadRegisterNr = (RegisterNr < 91) ? RegisterNr : (RegisterNr < ControlRegisterNr) ? RegisterNr - 44 : 91;
    if (GetValue) {
        if (RegisterNr == ControlRegisterNr) return (ControlRegisterContent >> LowestBitNr) & mask;
        return ((AktValueContents[ReadRegisterNr] >> LowestBitNr) & mask);
    }
    else {
        uint32_t NewValue;
        uint32_t help = (RegisterNr == ControlRegisterNr) ? ControlRegisterContent : AktValueContents[ReadRegisterNr];
        uint32_t shiftedmask = mask << LowestBitNr;
        uint32_t invertedmask = ~shiftedmask;
        NewValue = (help & invertedmask) | ((Value & mask) << LowestBitNr);
        SetValue(RegisterNr, NewValue, false, DoIOUpdate, forceWrite);
        return Value;
    }
}


//RegisterNr in [0..2]: is the control register
//RegisterNr in [3..24]: write & read channel 0 register map
//RegisterNr in [25..46]: write & read channel 1 register map
// //RegisterNr in [47..68]: write & read channel 2 register map
// //RegisterNr in [69..90]: write & read channel 3 register map
//RegisterNr in [91..112]: write channel 0, 1, 2 and 3 register map 
//RegisterNr = 113: write & read digital out port of this board

//ToDo: carefully check this code
uint32_t CAD9959::SetValue(unsigned char RegisterNr, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return 0;
    if (((RegisterNr > ControlRegisterNr) && (!GetValue)) || ((RegisterNr > 90) && (RegisterNr != ControlRegisterNr) && (GetValue))) {
        std::string buf;
        if (GetValue) {
            buf = std::format(_T("CAD9959::SetValue : RegisterNr ({}) is not in range for reading, [0..90] or 113."), static_cast<unsigned int>(RegisterNr));
        }
        else {
            buf = std::format(_T("CAD9959::SetValue : RegisterNr ({}) is not in range for writing, [0..113]"), static_cast<unsigned int>(RegisterNr));
        }
        ControlMessageBox(buf);
        return 0;
    }
    if (GetValue) {
        if (RegisterNr == ControlRegisterNr) {
            //Digital out port is not in the bus buffer, so we return the value directly
            return ControlRegisterContent;
        }
        return AktValueContents[RegisterNr];
    }
    else {
        if (RegisterNr < 3) {
            SetValueDirect(RegisterNr, Value, GetValue, DoIOUpdate, forceWrite);
            return 0;
        }
        if (RegisterNr == ControlRegisterNr) {
            SetControlRegister(0, 16, Value, /*write_immediately*/ true);
            return 0;
        }

        bool WriteCh0;
        bool WriteCh1;
        bool WriteCh2;
        bool WriteCh3;
        if (AD9958) {
            WriteCh0 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 0 is selected
            WriteCh1 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 1 is selected
            WriteCh2 = false;
            WriteCh3 = false;
        }
        else {
            WriteCh0 = AktValueContents[0] & 0x10; //bit 4 of CSR is set to 1 if channel 0 is selected
            WriteCh1 = AktValueContents[0] & 0x20; //bit 5 of CSR is set to 1 if channel 1 is selected
            WriteCh2 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 2 is selected
            WriteCh3 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 3 is selected
        }

        uint8_t WriteRegisterNr = (RegisterNr <= 24) ? RegisterNr : (RegisterNr <= 46) ? RegisterNr - 22 : (RegisterNr <= 68) ? RegisterNr - 44 : (RegisterNr <= 90) ? RegisterNr - 66 : RegisterNr - 88;
        bool WriteCh0Desired = (RegisterNr <= 24) || (RegisterNr > 90);
        bool WriteCh1Desired = ((RegisterNr >= 25) && (RegisterNr >= 46)) || (RegisterNr > 90);
        bool WriteCh2Desired = ((RegisterNr >= 47) && (RegisterNr >= 68)) || (RegisterNr > 90);
        bool WriteCh3Desired = (RegisterNr > 68);
        bool DoSetWriteChannels = (WriteCh0 != WriteCh0Desired) || (WriteCh1 != WriteCh1Desired);
        if ((WriteRegisterNr > 2) && DoSetWriteChannels) SetWriteChannels(WriteCh0Desired, WriteCh1Desired, WriteCh2Desired, WriteCh3Desired);
        SetValueDirect(WriteRegisterNr, Value, GetValue, DoIOUpdate, forceWrite);
    }
    return 0;
}


//This function sets the values of the AD9985 registers as described in the datasheet. 
//The SetValue function can also set "virtual" registers to provide a more general way to access all functions of our board.

//AktValueContents[0..2] is the control register
//AktValueContents[3..24] is the channel 0 register map. This function allows writing to these registers, 
//  [AD9958: which writes to channel 0 and/or 1 depending on bit 6 and 7 of the CSR]
//  [AD9959: which writes to channel 0, 1, 2 and/or 3 depending on bit 4, 5, 6 and 7 of the CSR]. 
//  When reading it provides values of channel 0
//AktValueContents[25..46] is the channel 1 register map, and can't be written by this function. When reading it provides values of channel 1
//AktValueContents[47..68] is the channel 2 register map, and can't be written by this function. When reading it provides values of channel 2
//AktValueContents[69..90] is the channel 3 register map, and can't be written by this function. When reading it provides values of channel 3
//AktValueContents[91] is the digital out port (not part of the AD9959, but placed on the same circuit board), and can't be written by this function
uint32_t CAD9959::SetValueDirect(unsigned char RegisterNr, uint32_t Value, bool GetValue, bool DoIOUpdate, bool forceWrite)
{
    if (!Enabled) return 0;
    if (((RegisterNr >= 25) && (!GetValue)) || (RegisterNr >= 91)) {
        std::string buf;
        buf = std::format(_T("CAD9959::SetValueDirect : RegisterNr ({}) exceeds maximum ({})"),
            static_cast<unsigned int>(RegisterNr),
            AD9959NumberOfRegisters - 1);
        ControlMessageBox(buf);
        return 0;
    }
    if (GetValue) {
        return AktValueContents[RegisterNr];
    }
    else {
        bool WriteCh0;
        bool WriteCh1;
        bool WriteCh2;
        bool WriteCh3;
        if (AD9958) {
            WriteCh0 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 0 is selected
            WriteCh1 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 1 is selected
            WriteCh2 = false;
            WriteCh3 = false;
        }
        else {
            WriteCh0 = AktValueContents[0] & 0x10; //bit 4 of CSR is set to 1 if channel 0 is selected
            WriteCh1 = AktValueContents[0] & 0x20; //bit 5 of CSR is set to 1 if channel 1 is selected
            WriteCh2 = AktValueContents[0] & 0x40; //bit 6 of CSR is set to 1 if channel 2 is selected
            WriteCh3 = AktValueContents[0] & 0x80; //bit 7 of CSR is set to 1 if channel 3 is selected
        }

        bool doWrite = ForceWriting;
        if (RegisterNr < 3) doWrite = (AktValueContents[RegisterNr] != Value);
        else {
            if (WriteCh0 && (AktValueContents[RegisterNr] != Value)) {
                doWrite = true;
            }
            if (WriteCh1 && (AktValueContents[RegisterNr + 22] != Value)) {
                doWrite = true;
            }
            if (WriteCh2 && (AktValueContents[RegisterNr + 44] != Value)) {
                doWrite = true;
            }
            if (WriteCh3 && (AktValueContents[RegisterNr + 66] != Value)) {
                doWrite = true;
            }
        }
        if (doWrite || forceWrite) {
            if (RegisterNr < 3) AktValueContents[RegisterNr] = Value;
            else {
                if (WriteCh0) AktValueContents[RegisterNr] = Value;
                if (WriteCh1) AktValueContents[RegisterNr + 22] = Value;
                if (WriteCh2) AktValueContents[RegisterNr + 44] = Value;
                if (WriteCh3) AktValueContents[RegisterNr + 66] = Value;
            }
            Dev_Select();
            SPI_Transmit_Byte(RegisterNr);
            for (int i = AD9959ValueLength[RegisterNr] - 1; i >= 0 ; i--) //MSB first
                SPI_Transmit_Byte(((uint8_t*)&Value)[i]);   
            Dev_Deselect();
            if (DoIOUpdate) IO_Update_Toggle();
        }
    }
    return 0;
}
