if(WIN32)
    set(OIDN_ROOT_DIR $ENV{OIDN_ROOT_DIR})
	
    find_path(OIDN_INCLUDE_DIRS
		NAMES
			OpenImageDenoise/config.h
		HINTS
			${OIDN_ROOT_DIR}/include/
	)
    
    find_library(OIDN_LIBRARIES OpenImageDenoise.lib PATHS ${OIDN_ROOT_DIR}/lib/)
	
else(WIN32)

	find_path(OIDN_INCLUDE_DIRS
		NAMES
			OpenImageDenoise/config.h
		HINTS
			/usr/local/include/
	)

    find_library(OIDN_LIBRARIES libOpenImageDenoise.so PATHS /usr/local/lib/)
	
endif(WIN32)

if (OIDN_INCLUDE_DIRS AND OIDN_LIBRARIES)
    SET(OIDN_FOUND TRUE)
ENDIF (OIDN_INCLUDE_DIRS AND OIDN_LIBRARIES)

if (OIDN_FOUND)
    message(STATUS "Found OIDN library: ${OIDN_LIBRARIES}")
else (OIDN_FOUND)
	message(FATAL_ERROR "Could not find OIDN library")	
endif (OIDN_FOUND)
