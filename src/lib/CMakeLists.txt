if(MSVC AND NOT "${MSVC_VERSION}" LESS 1400)
	add_definitions( "/MP" )
endif()

add_subdirectory(debug_tools)
add_subdirectory(external/zip)

set(TBB_TEST OFF)
add_subdirectory(external/oneTBB)

add_subdirectory(vengine)

