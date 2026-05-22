if(config STREQUAL "Debug")
    set(destination_dirs "${console_debug_dir}" "${gui_debug_dir}")
else()
    set(destination_dirs "${console_release_dir}" "${gui_release_dir}")
endif()

get_filename_component(file_name "${src}" NAME)

foreach(destination_dir IN LISTS destination_dirs)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${destination_dir}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${src}" "${destination_dir}/${file_name}")
endforeach()
