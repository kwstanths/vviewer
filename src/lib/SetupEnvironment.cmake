macro(SetupEnvironment)

	set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
	set(LIBS_ROOT C:/Users/konstantinos/Documents/LIBS)
    
	# Check compiler for c++11
	include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
    if (_cpp_latest_flag_supported)
        add_compile_options("/std:c++latest")
        add_compile_options("/Zc:__cplusplus")
    endif()
	
    set(INTERNAL_LIBRARIES_ROOT ${PROJECT_ROOT}/src/lib)
    
    # QT
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{Qt6_DIR})
    find_package(Qt6 COMPONENTS Widgets REQUIRED)
    set(QT_INCLUDE_DIRS ${Qt6Widgets_INCLUDE_DIRS})
    set(QT_LIBRARIES Qt6::Widgets)
    
    # VULKAN
    set(VULKAN_DIR ${LIBS_ROOT}/VulkanSDK/1.3.204.1)
    set(VULKAN_INCLUDE_DIRS ${VULKAN_DIR}/Include)
    set(VULKAN_LIBS_DIR ${VULKAN_DIR}/Lib)
    set(VULKAN_LIBRARIES ${VULKAN_LIBS_DIR}/vulkan-1.lib)
    
    # GLM
    set(GLM_DIR ${PROJECT_ROOT}/src/lib/external/glm/)
    set(GLM_INCLUDE_DIRS ${GLM_DIR})
    
    #STB
    set(STB_DIR ${PROJECT_ROOT}/src/lib/external/stb)
    set(STB_INCLUDE_DIRS ${STB_DIR})
    
    #ASSIMP
    set(ASSIMP_DIR ${LIBS_ROOT}/ASSIMP_LIBS)
    set(ASSIMP_INCLUDE_DIRS ${ASSIMP_DIR}/include)
    set(ASSIMP_LIBS_DIR ${ASSIMP_DIR}/lib64)
    set(ASSIMP_LIBRARIES ${ASSIMP_LIBS_DIR}/assimp-vc143-mt.lib)
    
    # UTILS
    set(UTILS_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/utils/)
    set(UTILS_LIBRARIES utils)
    
	if(MSVC)
		
	elseif(UNIX)

		
	endif()

endmacro()