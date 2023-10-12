cmake_minimum_required(VERSION 3.16)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(NAME "vviewer")
project(${NAME})

set(CMAKE_AUTOMOC ON)

include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ENGINE_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)
file(GLOB ${NAME}_MATH_HEADERS math/*.hpp)
file(GLOB ${NAME}_MATH_SOURCES math/*.cpp)
file(GLOB ${NAME}_UTILS_HEADERS utils/*.hpp)
file(GLOB ${NAME}_UTILS_SOURCES utils/*.cpp)
file(GLOB ${NAME}_RESOURCES *.qrc)
#QT5_ADD_RESOURCES(${NAME}_RESOURCES_RCC ${${NAME}_RESOURCES})

include(core/sources.cmake)
include(UI/sources.cmake)
include(vulkan/sources.cmake)

# Target
add_executable(${NAME} ${GUI_TYPE} 
	${${NAME}_HEADERS} ${${NAME}_SOURCES} 
	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES}
	${${NAME}_MODELS_HEADERS} ${${NAME}_MODELS_SOURCES}
	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES}
	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES}
	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES}
	${${NAME}_UTILS_HEADERS} ${${NAME}_UTILS_SOURCES}
	${${NAME}_RESOURCES_RCC}
	)

target_compile_features(${NAME} PRIVATE cxx_std_17)
target_compile_definitions(${NAME} PRIVATE QT_NO_KEYWORDS)
    
# Libraries
target_link_libraries(${NAME} ${ENGINE_LIBS})

source_group("top_level\\"	FILES	${${NAME}_HEADERS} ${${NAME}_SOURCES})
source_group("math\\"		FILES	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES})
source_group("models\\"		FILES	${${NAME}_MODELS_HEADERS} ${${NAME}_MODELS_SOURCES})
source_group("core\\"		FILES	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES})
source_group("vulkan\\"		FILES	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES})
source_group("UI\\"		FILES	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES})
source_group("utils\\"		FILES	${${NAME}_UTILS_HEADERS} ${${NAME}_UTILS_SOURCES})
