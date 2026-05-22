# CMakeList.txt : CMake project for OpticsFoundryCommandSystemTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("OpticsFoundryCommandSystemTest")

# Add source to this project's executable.
add_executable (OpticsFoundryCommandSystemTest "CPUCommandSequencer.cpp" "CPUCommandSequencer.h"  "command_defs.cpp" "command_defs.h" "command_errors.cpp" "command_impl.cpp" "executor.cpp" "parser.cpp" "OpticsFoundryFPGA.h" "OpticsFoundryFPGA.cpp" "label_table.cpp" "label_table.h" "executor.h" "pid.cpp" "pid.h" "name_table.cpp" "name_table.h" "command_errors.h" "registers.cpp" "registers.h" )




if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET OpticsFoundryCommandSystemTest PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.

