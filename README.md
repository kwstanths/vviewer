# vviewer 
A real time rendering and offline volumetric path tracing engine library written in Vulkan, and a 3D UI scene editor application written in Qt. Uses [VK_KHR_ray_query.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_query.html) and [VK_KHR_ray_tracing_pipeline.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_pipeline.html) for GPU accelerated ray tracing implementing ray generation, ray closest hit, ray any hit and ray miss shaders. Requires a GPU capable of hardware accelerated ray tracing (tested on RTX)

![Alt text](images/1.png?raw=true)

## Features
* Spatially varying Physically based (Metallic/Roughness) and Lambert materials
    * alpha, normal, emissiveness and ambient occlusion
* Bindless descriptors
* Point, Directional and Mesh lights
* HDR equirectangular and cubemap environment maps
* Perspective and Orthographic cameras
* Tone mapping
* Real time rendering
    * Deferred rendering
    * Ray traced shadows
    * Intancing
    * Image Based Lighting
* GPU path traced rendering
    * Multiple importance sampling
        * Light sampling
        * BRDF sampling
    * Low discrepancy samplers
    * Next event estimation
    * Homogeneous participating media
    * Russian roulette
    * Depth of field
    * Denoising
* Entity component system
* Export and Import scenes
* Cross platform build

## Dependencies
### vengine library
* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home). Set the VULKAN_SDK environment variable. Version tested 1.3.204.1
* [assimp](https://github.com/assimp/assimp). Set the ASSIMP_ROOT_DIR environment variable. Version tested 5.2.5
* [OpenImageDenoise](https://github.com/OpenImageDenoise/oidn). Set the OIDN_ROOT_DIR environment variable. Version tested 2.1.0

### vviewer binary
* Qt. Set the Qt6_DIR environment variable. Version tested 6.2.4

## Building
* Install dependencies
* Clone the project and all its submodules
* Create build folder, run cmake and build
* Runtime dependencies like the shaders and the assets folders are copied automatically, but in any case, make sure they are besides the binary

## Usage
* Run [vviewer](/src/bin/vviewer/) binary for the scene editor. Needs Qt dependency
* See [offlinerender](src/bin/offlinerender/) to use the rendering library without UI

## Images
![Alt text](images/2.png?raw=true)

![Alt text](images/3.png?raw=true)

![Alt text](images/4.png?raw=true)

![Alt text](images/5.png?raw=true)

![Alt text](images/6.png?raw=true)

## Known issues
* If a render fails with Device Lost, restart the application and render the scene with a smaller batch size
