import sys

#For quick testing of latest compilation of library
#main_path = "D:\\Florian\\Firefly\\FireflyControl\\ControlLight\\"
#library_path = main_path + "out\\build\\x64-debug"

#For more permanent installation, copy 
# control_light_api.cp310-win_amd64.pyd
# ControlLight.dll
# from the library creation path, e.g. D:\Florian\Firefly\FireflyControl\ControlLight\out\build\x64-debug,
# to the "lib" subdirectory of this script

#you can also do "pip install ." in the root path of ControlLight_DLL_Test_Python_for_pip to install the library Python system wide. 
# Then you don't need the following three lines and you don't need to copy the .pyd and .dll files.
main_path = ""
library_path = "lib"
sys.path.append(library_path)

import control_light_api

AD98450Address = 16
AnalogInChannel = 0
AnalogOutBoardStartAddress = 20
DigitalOutAddress = 10

cla = control_light_api.ControlLightAPI()
print("Created:", cla.is_created())
try:
    cla.configure(display_errors = False)
    #cla.load_from_json_file(main_path+r"ControlHardwareConfig.json")
    cla.add_device_sequencer(id=0,
                            type="ControlLightSmartSequencer",
                            ip="192.168.0.109",
                            port=7,
                            master=True,
                            start_delay=0,
                            clock_frequency=100000000,
                            fpga_clock_to_bus_clock_ratio=10,
                            use_external_clock=False,
                            use_strobe_generator=True,
                            connect=True)
  
    cla.add_device_analog_out_16bit(
        sequencer=0,
        start_address=AnalogOutBoardStartAddress,
        number_channels=4,
        signed_value=True,
        min_voltage=-10.0,
        max_voltage=10.0
    )
    cla.add_device_digital_out(
        sequencer=0,
        address=DigitalOutAddress,
        number_channels=16
    )
    cla.add_device_ad9854(
        sequencer=0,
        address=AD98450Address,
        version=1,
        external_clock_frequency=300000000.0,
        pll_reference_multiplier=1,
        frequency_multiplier=1
    )
except Exception as e:
    print("Error initializing hardware configuration", e)
    sys.exit(1)

try:
    cla.initialize()
    cla.switch_debug_mode(False, "DebugSequence")
    start_time = 0
    cla.start_assembling_sequence()
    cla.sequencer_start_analog_in_acquisition(0, 0, 0, AnalogInChannel, 1, 0.1)
    cla.set_start_frequency(0,AD98450Address, 1000000.0)
    cla.set_power(0, AD98450Address, 100.0)
    cla.set_voltage(0, AnalogOutBoardStartAddress, 0.0)
    cla.wait_ms(0.01) #wait for ADC to finish conversion
    cla.set_digital_output(0, DigitalOutAddress, 0, False)
    cla.start_assembling_next_sequence()
    CycleStartBufferPosition = cla.get_next_buffer_position_of_master_sequencer()
    cla.set_digital_output(0, DigitalOutAddress, 0, True)
    cla.sequencer_start_analog_in_acquisition(0, 0, 0, AnalogInChannel, 1, 0.1)
    SetVoltageBufferPosition = cla.get_next_buffer_position_of_master_sequencer()
    cla.set_voltage(0, AnalogOutBoardStartAddress, 10.0)
    SetFrequencyBufferPosition = cla.get_next_buffer_position_of_master_sequencer()
    cla.set_frequency(0, AD98450Address, 1000000.0)
    cla.wait_ms(0.1) #wait for ADC to finish conversion
    cla.set_digital_output(0, DigitalOutAddress, 0, False)
    cla.send_sequence() #sends sequence to FPGA, but does not execute it
    
except Exception as e:
    print("Error:", e)
    sys.exit(1)

#Commands for CPU command sequence
#cla.start_assembling_cpu_command_sequence()
#cla.add_cpu_command(commandstring)
#cla.execute_cpu_command_sequence(ethernet_check_period_ms = 100) # starts execution
#cla.stop_cpu_command_sequence() # sets ContinueExecution variable to zero (needs ethernet communication to be enabled)
#cla.interrupt_cpu_command_sequence() #interrupts CPU command execution (needs ethernet communication to be enabled)
#cla.get_cpu_command_error_messages()
#cla.print_cpu_command_error_messages() #prints errors on FPGA SOM UART for debugging
#cla.print_cpu_command_sequence() #prints the CPU command sequence FPAG SOM UART for debugging

try:
    cla.start_assembling_cpu_command_sequence()
    cla.add_cpu_command("ExecuteFPGASequence(0);")
    cla.add_cpu_command("WaitTillSequenceFinished();")   
    cla.add_cpu_command("ExecuteFPGASequence("+str(CycleStartBufferPosition)+");")
    cla.add_cpu_command("WaitTillSequenceFinished();")
    cla.add_cpu_command("LoopStart:") 
    cla.add_cpu_command("ExecuteFPGASequence("+str(CycleStartBufferPosition)+");")
    #while FPGA sequence is running, use data of last run to calculate and set new frequency and voltage
    cla.add_cpu_command("GetInputBufferValue(Input, 0);")
    cla.add_cpu_command("SetVoltage("+str(SetVoltageBufferPosition)+",Input);")
    cla.add_cpu_command("Mul(ScaledInput, Input, 1.23);")
    cla.add_cpu_command("Add(FrequencyTuningWord, ScaledInput, 123);")
    cla.add_cpu_command("SetAD9854Frequency("+str(SetFrequencyBufferPosition)+",FrequencyTuningWord);")
    cla.add_cpu_command("WaitTillSequenceFinished();")
    cla.add_cpu_command("JumpIfNotZero(ContinueExecution, LoopStart:);")
    cla.get_cpu_command_error_messages()
except Exception as e:
    print("Error:", e)
    sys.exit(1)
try:
    cla.execute_cpu_command_sequence(ethernet_check_period_ms = 100)
except Exception as e:
    print("Error:", e)
    sys.exit(1)


