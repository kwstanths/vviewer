cmake_minimum_required(VERSION 3.16)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(NAME "unittests")
project(${NAME})

include(${PROJECT_ROOT}/src/lib/SetupEnvironment.cmake)
SetupEnvironment()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${ENGINE_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)

# Target
add_executable(${NAME} 
	${${NAME}_HEADERS} ${${NAME}_SOURCES} 
	)

target_compile_features(${NAME} PRIVATE cxx_std_20)
    
# Libraries
target_link_libraries(${NAME} ${ENGINE_LIBS} GTest::gtest_main)

