# CMakeList.txt : CMake project for ControlLight, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.14)

project(ControlLight)
project ("ControlLight")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add custom option for build mode
set(BUILD_MODE "EXE" CACHE STRING "Choose build mode: EXE, DLL or PYD")
set_property(CACHE BUILD_MODE PROPERTY STRINGS EXE DLL PYD)

# Default build mode is EXE. Override with -DBUILD_MODE=DLL or -DBUILD_MODE=PYD when building the DLL or Python module.
option(CONTROL_LIGHT_STAGE_EXAMPLES "Copy built artifacts into the in-repository example folders after building DLL or PYD modes" OFF)
option(CONTROL_LIGHT_STAGE_QT_EXAMPLES "When staging examples, also copy ControlLight.dll into the configured Qt Creator build folders" OFF)
set(CONTROL_LIGHT_QT_CONSOLE_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Qt_console/build/Desktop_Qt_6_7_2_MinGW_64_bit-Release/release" CACHE PATH "Qt console release build output folder")
set(CONTROL_LIGHT_QT_CONSOLE_DEBUG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Qt_console/build/Desktop_Qt_6_7_2_MinGW_64_bit-Debug/debug" CACHE PATH "Qt console debug build output folder")
set(CONTROL_LIGHT_QT_GUI_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Qt_GUI/build/Desktop_Qt_6_7_2_MinGW_64_bit-Release/release" CACHE PATH "Qt GUI release build output folder")
set(CONTROL_LIGHT_QT_GUI_DEBUG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Qt_GUI/build/Desktop_Qt_6_7_2_MinGW_64_bit-Debug/debug" CACHE PATH "Qt GUI debug build output folder")

function(control_light_stage_file build_target source_file destination_dir)
    add_custom_command(TARGET ${build_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source_file}" "${destination_dir}"
        VERBATIM)
endfunction()

function(control_light_stage_file_as build_target source_file destination_file)
    get_filename_component(destination_dir "${destination_file}" DIRECTORY)
    add_custom_command(TARGET ${build_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${destination_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${source_file}" "${destination_file}"
        VERBATIM)
endfunction()


# Use dynamic (DLL) runtime libraries, needed for _AFXDLL
if (MSVC)
    set(CMAKE_MFC_FLAG 2)  # 1 = static, 2 = shared DLL
    foreach(flag_var
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL)
      string(REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
    endforeach()
else()
    # additional warnings
    add_compile_options(-O3)
endif()

# Conditional target generation
if(BUILD_MODE STREQUAL "EXE")

    # Enable Hot Reload for MSVC compilers if supported.
    if (MSVC)
        if (POLICY CMP0141)
          cmake_policy(SET CMP0141 NEW)
          set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
        endif()
    endif()

    #add all sources
    file(GLOB SOURCES "*.cpp" "*.h")

    #To create an executable for testing
    add_executable(ControlLight ${SOURCES})

    if (MSVC)
        target_compile_definitions(ControlLight PRIVATE WIN32 _WINDOWS)
        target_compile_options(ControlLight PRIVATE /W3 /GR /EHsc $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Debug>:/ZI> $<$<CONFIG:Debug>:/Ob0> $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Debug>:/RTC1> $<$<NOT:$<CONFIG:Debug>>:/MD>)
        target_link_options(ControlLight PRIVATE /machine:$<IF:$<STREQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86> $<$<CONFIG:Debug>:/debug> $<$<CONFIG:Debug>:/INCREMENTAL>)
    endif()

    target_compile_definitions(ControlLight PRIVATE USING_DLL)
    target_compile_definitions(ControlLight PRIVATE _AFXDLL)
    target_compile_definitions(ControlLight PRIVATE C_API) #if you do not want a C++ API; comment out if you want C++ API
    #target_compile_definitions(ControlLight PRIVATE THROW_EXCEPTIONS) #Use this only for a C++ API; if you want to use C API, comment this line out
    #target_compile_definitions(ControlLight PRIVATE API_CLASS) #Use this only for a C++ API; if you want to use C API, comment this line out

    #set(CMAKE_BUILD_TYPE Debug)

elseif(BUILD_MODE STREQUAL "DLL")

    # Enable Hot Reload for MSVC compilers if supported.
    if (MSVC)
        if (POLICY CMP0141)
          cmake_policy(SET CMP0141 NEW)
          set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
        endif()
    endif()

    #add all sources
    file(GLOB SOURCES "*.cpp" "*.h")
    #To create DLL for C or C++
    add_library(ControlLight SHARED ${SOURCES})
    if (MSVC)
        target_compile_definitions(ControlLight PRIVATE WIN32 _WINDOWS)
        target_compile_options(ControlLight PRIVATE /W3 /GR /EHsc $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Debug>:/ZI> $<$<CONFIG:Debug>:/Ob0> $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Debug>:/RTC1> $<$<NOT:$<CONFIG:Debug>>:/MD>)
        target_link_options(ControlLight PRIVATE /machine:$<IF:$<STREQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86> $<$<CONFIG:Debug>:/debug> $<$<CONFIG:Debug>:/INCREMENTAL>)
    endif()
    target_compile_definitions(ControlLight PRIVATE BUILDING_DLL)
    target_compile_definitions(ControlLight PRIVATE C_API) #if you do not want a C++ API; comment out if you want C++ API
    target_compile_definitions(ControlLight PRIVATE _AFXDLL)
    #target_compile_definitions(ControlLight PRIVATE API_CLASS) #Use this only for a C++ API; if you want to use C API, as needed for e.g. Qt, comment this line out

    
    #target_compile_definitions(ControlLight PRIVATE NAMESPACE_CLA) #only for C++ API, if you want to keep DLL functions in namespace CLA
    #if you use Namespace CLA, you need to put CLA:: in front of all DLL function names; you can then remove the CLA_ prefix from all functions

    if(CONTROL_LIGHT_STAGE_EXAMPLES)
        set(VS_CONSOLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_VS_console")
        control_light_stage_file(ControlLight "$<TARGET_FILE:ControlLight>" "${VS_CONSOLE_DIR}/bin")
        control_light_stage_file(ControlLight "$<TARGET_LINKER_FILE:ControlLight>" "${VS_CONSOLE_DIR}/lib")

        if(CONTROL_LIGHT_STAGE_QT_EXAMPLES)
            add_custom_command(TARGET ControlLight POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                    "-Dconfig=$<CONFIG>"
                    "-Dsrc=$<TARGET_FILE:ControlLight>"
                    "-Dconsole_release_dir=${CONTROL_LIGHT_QT_CONSOLE_RELEASE_DIR}"
                    "-Dconsole_debug_dir=${CONTROL_LIGHT_QT_CONSOLE_DEBUG_DIR}"
                    "-Dgui_release_dir=${CONTROL_LIGHT_QT_GUI_RELEASE_DIR}"
                    "-Dgui_debug_dir=${CONTROL_LIGHT_QT_GUI_DEBUG_DIR}"
                    -P "${CMAKE_CURRENT_LIST_DIR}/cmake/stage_qt_dll.cmake"
                VERBATIM)
        endif()
    endif()

    #optional: move results to target folder
    #install(TARGETS ControlLight DESTINATION lib)
    #install(FILES ControlLight.h DESTINATION include)

elseif(BUILD_MODE STREQUAL "PYD")

    # Collect sources
    file(GLOB SOURCES "*.cpp" "*.h")

    # Add pybind11 from local source
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/pybind11 ${CMAKE_CURRENT_BINARY_DIR}/pybind11_build)

    # Add include path for json.hpp
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Add ControlLight core library
    add_library(ControlLight SHARED ${SOURCES})
    if (MSVC)
        target_compile_definitions(ControlLight PRIVATE WIN32 _WINDOWS)
        target_compile_options(ControlLight PRIVATE /W3 /GR /EHsc $<$<CONFIG:Debug>:/MDd> $<$<CONFIG:Debug>:/ZI> $<$<CONFIG:Debug>:/Ob0> $<$<CONFIG:Debug>:/Od> $<$<CONFIG:Debug>:/RTC1> $<$<NOT:$<CONFIG:Debug>>:/MD>)
        target_link_options(ControlLight PRIVATE /machine:$<IF:$<STREQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86> $<$<CONFIG:Debug>:/debug> $<$<CONFIG:Debug>:/INCREMENTAL>)
    endif()
    target_compile_definitions(ControlLight PRIVATE PYTHON_API THROW_EXCEPTIONS API_CLASS _AFXDLL)
    target_include_directories(ControlLight PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Add Python module
    pybind11_add_module(control_light_api bindings/ControlAPI_Python.cpp)
    target_link_libraries(control_light_api PRIVATE ControlLight)
    target_compile_definitions(control_light_api PRIVATE PYTHON_API THROW_EXCEPTIONS API_CLASS _AFXDLL)

    if(CONTROL_LIGHT_STAGE_EXAMPLES)
        set(PYTHON_EXAMPLE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Python/lib")
        set(PIP_PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ControlLight_DLL_Test_Python_for_pip/control_light_api")
        control_light_stage_file(control_light_api "$<TARGET_FILE:control_light_api>" "${PYTHON_EXAMPLE_LIB_DIR}")
        control_light_stage_file(control_light_api "$<TARGET_FILE:ControlLight>" "${PYTHON_EXAMPLE_LIB_DIR}")
        control_light_stage_file(control_light_api "$<TARGET_FILE:ControlLight>" "${PIP_PACKAGE_DIR}")
        control_light_stage_file_as(control_light_api "$<TARGET_FILE:control_light_api>" "${PIP_PACKAGE_DIR}/__init__.pyd")
    endif()

else()
    message(FATAL_ERROR "Invalid BUILD_MODE: must be EXE, DLL, or PYD")
endif()
