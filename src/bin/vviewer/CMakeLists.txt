cmake_minimum_required(VERSION 3.0)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

# Project
set(NAME "vviewer")
project(${NAME})

# Configuration
set(CMAKE_AUTOMOC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INTERNAL_LIBRARIES_ROOT})
include_directories(${QT_INCLUDE_DIRS})
include_directories(${VULKAN_INCLUDE_DIRS})
include_directories(${UTILS_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})
include_directories(${STB_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)
file(GLOB ${NAME}_MATH_HEADERS math/*.hpp)
file(GLOB ${NAME}_MATH_SOURCES math/*.cpp)
file(GLOB ${NAME}_MODELS_HEADERS models/*.hpp)
file(GLOB ${NAME}_MODELS_SOURCES models/*.cpp)
file(GLOB ${NAME}_CORE_HEADERS core/*.hpp)
file(GLOB ${NAME}_CORE_SOURCES core/*.cpp)
file(GLOB ${NAME}_VULKAN_HEADERS vulkan/*.hpp)
file(GLOB ${NAME}_VULKAN_SOURCES vulkan/*.cpp)
file(GLOB ${NAME}_UI_HEADERS UI/*.hpp)
file(GLOB ${NAME}_UI_SOURCES UI/*.cpp)
file(GLOB ${NAME}_RESOURCES *.qrc)
QT5_ADD_RESOURCES(${NAME}_RESOURCES_RCC ${${NAME}_RESOURCES})

# Target
add_executable(${NAME} ${GUI_TYPE} 
	${${NAME}_HEADERS} ${${NAME}_SOURCES} 
	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES}
	${${NAME}_MODELS_HEADERS} ${${NAME}_MODELS_SOURCES}
	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES}
	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES}
	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES}
	${${NAME}_RESOURCES_RCC}
	)

# Libraries
target_link_libraries(${NAME} ${QT_LIBRARIES} ${VULKAN_LIBRARIES} ${UTILS_LIBRARIES} ${ASSIMP_LIBRARIES})

source_group("top_level\\"	FILES	${${NAME}_HEADERS} ${${NAME}_SOURCES})
source_group("math\\"		FILES	${${NAME}_MATH_HEADERS} ${${NAME}_MATH_SOURCES})
source_group("models\\"		FILES	${${NAME}_MODELS_HEADERS} ${${NAME}_MODELS_SOURCES})
source_group("core\\"		FILES	${${NAME}_CORE_HEADERS} ${${NAME}_CORE_SOURCES})
source_group("vulkan\\"		FILES	${${NAME}_VULKAN_HEADERS} ${${NAME}_VULKAN_SOURCES})
source_group("UI\\"		FILES	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES})
