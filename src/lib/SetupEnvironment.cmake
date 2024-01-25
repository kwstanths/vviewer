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
        
    # VULKAN
    find_package(Vulkan COMPONENTS glslc REQUIRED)
    set(VULKAN_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
    set(VULKAN_LIBRARIES Vulkan::Vulkan)
    
    # GLM
    set(GLM_DIR ${PROJECT_ROOT}/src/lib/external/glm/)
    set(GLM_INCLUDE_DIRS ${GLM_DIR})
    
    # STB
    set(STB_DIR ${PROJECT_ROOT}/src/lib/external/stb)
    set(STB_INCLUDE_DIRS ${STB_DIR})
    
    # RAPIDJSON
    set(RAPIDJSON_DIR ${PROJECT_ROOT}/src/lib/external/rapidjson)
    set(RAPIDJSON_INCLUDE_DIRS ${RAPIDJSON_DIR}/include)

    # kuba-zip
    set(ZIP_DIR ${PROJECT_ROOT}/src/lib/external/zip)
    set(ZIP_INCLUDE_DIRS ${ZIP_DIR}/src)
    set(ZIP_LIBRARIES zip)

    # unused
    # TBB
    #set(TBB_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/external/oneTBB/include)
    #set(TBB_LIBRARIES tbb)

    # OIDN
    find_package(OpenImageDenoise REQUIRED)
    set(OIDN_LIBRARIES OpenImageDenoise)

    # DEBUG_TOOLS
    set(DEBUG_TOOLS_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/debug_tools/)
    set(DEBUG_TOOLS_LIBRARIES debug_tools)
    
    # ASSIMP
    find_package(assimp REQUIRED)
    set(ASSIMP_INCLUDE_DIRS ${assimp_INCLUDE_DIRS})
    set(ASSIMP_LIBRARIES ${assimp_LIBRARIES})
    
    # VENGINE
    set(VENGINE_INCLUDE_DIRS ${PROJECT_ROOT}/src/lib/vengine/)
    set(VENGINE_LIBRARIES vengine)
   
    set(ENGINE_INCLUDE_DIRS 
        ${INTERNAL_LIBRARIES_ROOT}        
        ${VULKAN_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
        ${STB_INCLUDE_DIRS}
        ${RAPIDJSON_INCLUDE_DIRS}
        ${ZIP_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}
        ${DEBUG_TOOLS_INCLUDE_DIRS}
        ${VENGINE_INCLUDE_DIRS}
        ${ASSIMP_INCLUDE_DIRS}
    )
    
    set(ENGINE_LIBS
        ${VULKAN_LIBRARIES}
        ${ZIP_LIBRARIES}
        ${TBB_LIBRARIES}
        ${DEBUG_TOOLS_LIBRARIES}
        ${VENGINE_LIBRARIES}
        ${ASSIMP_LIBRARIES}
        ${OIDN_LIBRARIES}
    )

endmacro()
