import sys

#For quick testing of latest compilation of library
#main_path = "D:\\Florian\\Firefly\\FireflyControl\\ControlLight\\"
#library_path = main_path + "out\\build\\x64-debug"

#For more permanent installation, copy 
# control_light_api.cp310-win_amd64.pyd
# ControlLight.dll
# from the library creation path, e.g. D:\Florian\Firefly\FireflyControl\ControlLight\out\build\x64-debug,
# to the "lib" subdirectory of this script
main_path = ""
library_path = "lib"

sys.path.append(library_path)
import control_light_api

cla = control_light_api.ControlLightAPI()
print("Created:", cla.is_created())
try:
    cla.load_from_json_file(main_path+r"ControlHardwareConfig.json")
except Exception as e:
    print("Error loading JSON file:", e)
    sys.exit(1)
try:
    cla.initialize()
    cla.switch_debug_mode(True, "DebugSequence")
    start_time = 0
    for i in range(10):
        cla.start_assembling_sequence()
        cla.sequencer_start_analog_in_acquisition(
            0,
            0,  # AQuRA MCP3208 analog in board
            0,  # SPI chip-select
            0,
            100,
            0.1)
        for j in range(100):
            cla.set_voltage(0, 24, 10.0 * j / 100.0)
            cla.wait_ms(0.1)
            frequency = 1000.0 * j / 100.0
            cla.set_frequency(0, 232, frequency)

        cla.execute_sequence()

        input = cla.wait_till_end_of_sequence_then_get_input_data(timeout_in_s = 10)
        print("Input data length:", input["length"]," duration:", input["end_time_of_cycle"]-start_time)
        start_time = input["end_time_of_cycle"]
except Exception as e:
    print("Error:", e)
    sys.exit(1)
