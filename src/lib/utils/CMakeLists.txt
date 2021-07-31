cmake_minimum_required(VERSION 3.0)

# Project
set(NAME "utils")
project(${NAME})

# Sources
file(GLOB ${NAME}_HEADERS *.hpp)
file(GLOB ${NAME}_SOURCES *.cpp)

# Target
add_library(${NAME} ${${NAME}_HEADERS} ${${NAME}_SOURCES})