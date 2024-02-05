cmake_minimum_required(VERSION 3.16)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(NAME "vengine")
project(${NAME})

include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

include_directories(${ENGINE_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)
file(GLOB ${NAME}_MATH_HEADERS math/*.hpp)
file(GLOB ${NAME}_MATH_SOURCES math/*.cpp)
file(GLOB ${NAME}_UTILS_HEADERS utils/*.hpp)
file(GLOB ${NAME}_UTILS_SOURCES utils/*.cpp)

include(core/sources.cmake)
include(vulkan/sources.cmake)

# Target
add_library(${NAME}
	${${NAME}_HEADERS} ${${NAME}_SOURCES} 
	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES}
	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES}
	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES}
	${${NAME}_UTILS_HEADERS} ${${NAME}_UTILS_SOURCES}
	)

target_compile_features(${NAME} PRIVATE cxx_std_20)
    
# Libraries
target_link_libraries(${NAME} ${VULKAN_LIBRARIES}
		${ZIP_LIBRARIES}
		${TBB_LIBRARIES}
		${DEBUG_TOOLS_LIBRARIES}
		${ASSIMP_LIBRARIES}
		Threads::Threads)

source_group("math\\"		FILES	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES})
source_group("core\\"		FILES	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES})
source_group("vulkan\\"		FILES	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES})
source_group("utils\\"		FILES	${${NAME}_UTILS_HEADERS} ${${NAME}_UTILS_SOURCES})

# Build shaders
message("Building shaders...")
if(MSVC)
    execute_process(COMMAND "compile.bat" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/)
elseif(UNIX)
    execute_process(COMMAND "./compile.sh" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/shaders/)
endif()
