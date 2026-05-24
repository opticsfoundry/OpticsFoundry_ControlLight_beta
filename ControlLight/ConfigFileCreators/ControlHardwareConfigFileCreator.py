from ConfigCreator import ConfigBuilder


if __name__ == "__main__":
    builder = ConfigBuilder(filename="ControlHardwareConfig.json")
    builder.RegisterSequencer(IP="192.168.0.112", Port=57978, ClockFrequencyinMHz=100, StrobeDurationInFPGAClockPeriods=15, DebugOn=False)  # 0.104 #90.108
    builder.RegisterSerialPortBoard(Sequencer=0, Address=251, RackNr=0, SlotNr=1)
    analog_out_configs = [
        (24, True, -10, 10),  # each of these lines configures 4 analog outputs in consecutive order of addresses
        (28, True, -10, 10),
        (32, True, -10, 10),
        (36, True, -10, 10)
    ]
    for addr, signed, minv, maxv in analog_out_configs:
        builder.RegisterAnalogOutBoard16bit(StartAddress=addr, Signed=signed, MinVoltage=minv, MaxVoltage=maxv)

    for addr in [1, 2]:
        builder.RegisterDigitalOutBoard(Address=addr)

    AD9854Board0ExternalClockFrequencyinMHz = 10
    AD9854Board0PLLReferenceMultiplier = 20
    builder.RegisterDDSAD9854Board(Address=128, ExternalClockFrequencyinMHz=AD9854Board0ExternalClockFrequencyinMHz, PLLReferenceMultiplier=AD9854Board0PLLReferenceMultiplier)
    builder.RegisterDDSAD9854Board(Address=132, ExternalClockFrequencyinMHz=AD9854Board0ExternalClockFrequencyinMHz, PLLReferenceMultiplier=AD9854Board0PLLReferenceMultiplier)

    for addr in range(136, 172, 4):
        builder.RegisterDDSAD9854Board(Address=addr, ExternalClockFrequencyinMHz=300)

    for addr in [52, 56]:  # range(52, 84, 4):
        builder.RegisterDDSAD9858Board(Address=addr, ClockFrequencyinMHz=1200)

    for addr in [3, 4]:
        builder.RegisterDDSAD9959Board(Address=addr, ClockFrequencyinMHz=500, version = 0.09)

    builder.Save()
