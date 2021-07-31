macro(SetupEnvironment)

	set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
	set(LIBS_ROOT F:/Documents/dev/LIBS)
    
	# Check compiler for c++11
	include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
    endif()
	
    set(INTERNAL_LIBRARIES_ROOT ${PROJECT_ROOT}/src/lib)
    
    # QT
    find_package(Qt5 COMPONENTS Widgets Xml REQUIRED)
    set(QT_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})
    set(QT_LIBRARIES Qt5::Widgets Qt5::Xml)
    
    # VULKAN
    set(VULKAN_DIR ${LIBS_ROOT}/VulkanSDK/1.2.162.0)
    set(VULKAN_INCLUDE_DIRS ${VULKAN_DIR}/Include)
    set(VULKAN_LIBS_DIR ${VULKAN_DIR}/Lib32)
    set(VULKAN_LIBRARIES ${VULKAN_LIBS_DIR}/vulkan-1.lib)
    
    # GLM
    set(GLM_DIR ${LIBS_ROOT}/GLM/glm-0.9.9.8)
    set(GLM_INCLUDE_DIRS ${GLM_DIR}/glm)
    
    # UTILS
    set(UTILS_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/utils/)
    set(UTILS_LIBRARIES utils)
    
	if(MSVC)
		
	elseif(UNIX)

		
	endif()

endmacro()