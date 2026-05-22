# CMakeList.txt : CMake project for ControlLight, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)
project(ControlLightTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Create the executable
add_executable(ControlLightTest ControlLightTest.cpp)
target_compile_definitions(ControlLightTest PRIVATE USING_DLL)
target_compile_definitions(ControlLightTest PRIVATE C_API)


# Include and link the DLL interface
target_include_directories(ControlLightTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight)
target_link_directories(ControlLightTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(ControlLightTest PRIVATE ControlLight)

# For Windows, copy the DLL to the binary dir after build
add_custom_command(TARGET ControlLightTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/bin/ControlLight.dll" 
    $<TARGET_FILE_DIR:ControlLightTest>
)


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()



if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ControlLightTest PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
