cmake_minimum_required(VERSION 3.0)

project(vviewer)

add_subdirectory(src/bin/vviewer)
add_subdirectory(src/lib)

# Build shaders
message("Building shaders...")
if(MSVC)
    execute_process(COMMAND "compile.bat" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/vviewer/shaders/)
elseif(UNIX)
    execute_process(COMMAND "./compile.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/bin/vviewer/shaders/)
endif()
# Copy shaders to build directory
file(COPY src/bin/vviewer/shaders/ DESTINATION src/bin/vviewer/shaders/)
