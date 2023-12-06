cmake_minimum_required(VERSION 3.7)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(NAME "vviewer")
project(${NAME})

set(CMAKE_AUTOMOC ON)

include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

# QT
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{Qt6_DIR})
find_package(Qt6 COMPONENTS Widgets Concurrent REQUIRED)
set(QT_INCLUDE_DIRS ${Qt6Widgets_INCLUDE_DIRS})
set(QT_LIBRARIES Qt6::Widgets)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ENGINE_INCLUDE_DIRS})
include_directories(${QT_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)
file(GLOB ${NAME}_RESOURCES *.qrc)
#QT5_ADD_RESOURCES(${NAME}_RESOURCES_RCC ${${NAME}_RESOURCES})

include(UI/sources.cmake)

# Target
add_executable(${NAME} ${GUI_TYPE} 
	${${NAME}_HEADERS} ${${NAME}_SOURCES} 
	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES}
	${${NAME}_RESOURCES_RCC}
	)

target_compile_features(${NAME} PRIVATE cxx_std_17)
target_compile_definitions(${NAME} PRIVATE QT_NO_KEYWORDS)
    
# Libraries
target_link_libraries(${NAME} ${ENGINE_LIBS} ${QT_LIBRARIES})

source_group("top_level\\"	FILES	${${NAME}_HEADERS} ${${NAME}_SOURCES})
source_group("UI\\"		FILES	${${NAME}_UI_HEADERS} ${${NAME}_UI_SOURCES})
