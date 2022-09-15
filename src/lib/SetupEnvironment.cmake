macro(SetupEnvironment)

	set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
	set(LIBS_ROOT $ENV{LIBS_ROOT})
    
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
    find_package(Vulkan COMPONENTS glslc REQUIRED)
    set(VULKAN_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
    set(VULKAN_LIBRARIES Vulkan::Vulkan)
    
    # GLM
    set(GLM_DIR ${PROJECT_ROOT}/src/lib/external/glm/)
    set(GLM_INCLUDE_DIRS ${GLM_DIR})
    
    #STB
    set(STB_DIR ${PROJECT_ROOT}/src/lib/external/stb)
    set(STB_INCLUDE_DIRS ${STB_DIR})
    
    #RAPIDJSON
    set(RAPIDJSON_DIR ${PROJECT_ROOT}/src/lib/external/rapidjson)
    set(RAPIDJSON_INCLUDE_DIRS ${RAPIDJSON_DIR}/include)

    # UTILS
    set(UTILS_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/utils/)
    set(UTILS_LIBRARIES utils)
    
    if(MSVC)
        #ASSIMP
        set(ASSIMP_DIR ${LIBS_ROOT}/ASSIMP_LIBS)
        set(ASSIMP_INCLUDE_DIRS ${ASSIMP_DIR}/include)
        set(ASSIMP_LIBS_DIR ${ASSIMP_DIR}/lib64)
        set(ASSIMP_LIBRARIES ${ASSIMP_LIBS_DIR}/assimp-vc143-mt.lib)
    elseif(UNIX)
        #ASSIMP
        # assimp inlude dirs are installed in /usr/include
        #set(ASSIMP_DIR ${LIBS_ROOT}/ASSIMP_LIBS)
        #set(ASSIMP_INCLUDE_DIRS ${ASSIMP_DIR}/include)
        
        set(ASSIMP_LIBRARIES /usr/lib/x86_64-linux-gnu/libassimp.so)
    endif()

endmacro()
