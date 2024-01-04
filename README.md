# vviewer 
A 3D model viewer and scene editor written in Vulkan and Qt. Includes a rendering engine that can be built separately as a standalone library, and a UI editor built with Qt

## Features
* Import 3D models, textures and equirectangular HDRI maps
* Create Physically Based or Lambert materials
* Add Point/Directional/Mesh lights
* Export/Import scenes
* Launch a GPU path tracer with a denoiser 

## Dependencies 
* Vulkan. Make sure the VULKAN_SDK environment variable points to the installation folder. Version tested 1.3.204.1
* assimp. Look into src/lib/SetupEnvironment.cmake and edit the assimp paths based on your installation. Version tested 5.2.5
* [OpenImageDenoise](https://github.com/OpenImageDenoise/oidn). Install or build from source OpenImageDenoise

### vviewer binary
* Qt. Make sure the Qt6_DIR points to the installation folder. Version tested 6.2.4

## Building
* Install dependencies
* Clone the project and all its submodules
* Create build folder, run cmake and build
* Make sure that runtime dependencies can be found. If the paths are set up correctly, cmake will compile the shaders and paste assets and the shader binaries

## Images

![Alt text](images/1.png?raw=true)

![Alt text](images/2.png?raw=true)

![Alt text](images/3.png?raw=true)

![Alt text](images/4.png?raw=true)

![Alt text](images/5.png?raw=true)

![Alt text](images/6.png?raw=true)
