#include "../ConfigCreator.h"

int main() {
	ConfigCreator builder;
	builder.RegisterSequencer(/*Id*/ 0, /*Type*/ "OpticsFoundrySequencerV1", /*IP*/ "192.168.1.112",
		/*Port*/ 57978, /*Master*/ true, /*StartDelay*/ 10, /*ClockFrequencyinMHz*/ 100,
		/*FPGAClockToBusClockRatio*/ 15, /*UseExternalClock*/ false, /*UseStrobeGenerator*/ true,
		/*UseEdgeTriggeredLatches*/ true,
		/*Connect*/ true, /*DebugOn*/ false); // 0.104 #90.108
	builder.Save();
	return 0;
	
	const int analog_out_configs[][4] = {
		{24, 1, -10, 10},
		{28, 1, -10, 10},
		{32, 1, -10, 10},
		{36, 1, -10, 10}
	};
	for (const auto& config : analog_out_configs) {
		builder.RegisterAnalogOutBoard16bit(/*Sequencer*/ 0, /*StartAddress*/ config[0],
			/*NumberChannels*/ 4, /*Signed*/ config[1] != 0, /*MinVoltage*/ config[2],
			/*MaxVoltage*/ config[3]);
	}

	for (const int address : {2, 21, 3, 4, 5, 6, 7}) {
		builder.RegisterDigitalOutBoard(/*Sequencer*/ 0, /*Address*/ address);
	}

	const int AD9854Board0ExternalClockFrequencyinMHz = 10;
	const int AD9854Board0PLLReferenceMultiplier = 20;
	builder.RegisterDDSAD9854Board(/*Version*/ 2, /*Sequencer*/ 0, /*Address*/ 128,
		/*ExternalClockFrequencyinMHz*/ AD9854Board0ExternalClockFrequencyinMHz,
		/*PLLReferenceMultiplier*/ AD9854Board0PLLReferenceMultiplier);
	builder.RegisterDDSAD9854Board(/*Version*/ 2, /*Sequencer*/ 0, /*Address*/ 132,
		/*ExternalClockFrequencyinMHz*/ AD9854Board0ExternalClockFrequencyinMHz,
		/*PLLReferenceMultiplier*/ AD9854Board0PLLReferenceMultiplier);

	for (int address = 136; address < 172; address += 4) {
		builder.RegisterDDSAD9854Board(/*Version*/ 2, /*Sequencer*/ 0, /*Address*/ address);
	}

	for (const int address : {52, 56, 60, 64, 68, 72, 76, 80, 84}) {
		builder.RegisterDDSAD9858Board(/*Sequencer*/ 0, /*Address*/ address);
	}

	for (const int address : {1, 10}) {
		builder.RegisterDDSAD9959Board(/*Sequencer*/ 0, /*Address*/ address, /*ClockFrequencyinMHz*/ 500);
	}

	builder.Save();
	return 0;
}
