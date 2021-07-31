cmake_minimum_required(VERSION 3.0)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

# Project
set(NAME "vviewer")
project(${NAME})

# Configuration
set(CMAKE_AUTOMOC ON)

include_directories(${INTERNAL_LIBRARIES_ROOT})
include_directories(${QT_INCLUDE_DIRS})
include_directories(${VULKAN_INCLUDE_DIRS})
include_directories(${UTILS_INCLUDE_DIRS})
include_directories(${GLM_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)
file(GLOB ${NAME}_RESOURCES *.qrc)
QT5_ADD_RESOURCES(${NAME}_RESOURCES_RCC ${${NAME}_RESOURCES})

# Target
add_executable(${NAME} ${GUI_TYPE} ${${NAME}_HEADERS} ${${NAME}_SOURCES} ${${NAME}_RESOURCES_RCC})

# Libraries
target_link_libraries(${NAME} ${QT_LIBRARIES} ${VULKAN_LIBRARIES} ${UTILS_LIBRARIES})
