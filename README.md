# vviewer 
A real time renderer and an offline path tracer written in Vulkan and a 3D scene editor written in Qt. Uses [VK_KHR_ray_query.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_query.html) and [VK_KHR_ray_tracing_pipeline.html](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_pipeline.html) for GPU accelerated ray tracing implementing ray generation, ray closest hit, ray any hit and ray miss shaders. The rendering engine can be built separately as a standalone library, independent of the UI.

![Alt text](images/1.png?raw=true)

## Features
* Importing 3D models with assimp and corresponding materials (only .gltf and .obj materials)
* Physically based (Metallic/Roughness) and Lambert materials
    * with transparency, normal mapping, emissiveness, ambient occlusion
* Spatially varying materials
* Bindless resource management (transforms, textures, materials, lights)
* Perspective and Orthographic cameras
* Point, Directional and Mesh lights
* HDR equirectangular and cubemap environment maps
* Tone mapping
* Real time rendering
    * Image Based Lighting
    * MSAA
    * Ray traced shadows
    * Highlighting
    * Transform visualization
    * AABB visualization
* GPU path traced rendering
    * Low discrepancy sampling with progressive sequences
    * Multiple importance sampling
        * Light sampling
        * BRDF sampling
    * Next event estimation
    * Russian roulette
    * Depth of field
    * Denoising
* Scene graph
* Entity component system
* Export and Import scenes
* Separate render thread
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
