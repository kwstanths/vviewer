cmake_minimum_required(VERSION 3.5)

project(vviewer)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(src/lib)
add_subdirectory(src/bin/vviewer)
add_subdirectory(src/bin/offlinerender)
add_subdirectory(src/bin/unittests)

# Copy shaders to vviewer directory
file(COPY src/lib/vengine/shaders DESTINATION src/bin/vviewer/)
file(COPY src/lib/vengine/shaders DESTINATION src/bin/offlinerender/)
file(COPY src/lib/vengine/shaders DESTINATION src/bin/unittests/)

# Copy assets to build directory
file(COPY assets DESTINATION src/bin/vviewer/)
file(COPY assets DESTINATION src/bin/offlinerender/)
file(COPY assets DESTINATION src/bin/unittests/)
