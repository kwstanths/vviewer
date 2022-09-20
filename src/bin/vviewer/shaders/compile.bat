set glslcLocation=%VULKAN_SDK%\Bin\glslc.exe

%glslcLocation% standard.vert -o SPIRV/standardVert.spv
%glslcLocation% pbrBase.frag -o SPIRV/pbrFragBase.spv
%glslcLocation% pbrAdd.frag -o SPIRV/pbrFragAdd.spv
%glslcLocation% lambertBase.frag -o SPIRV/lambertFragBase.spv
%glslcLocation% lambertAdd.frag -o SPIRV/lambertFragAdd.spv
%glslcLocation% 3dui.vert -o SPIRV/3duiVert.spv
%glslcLocation% 3dui.frag -o SPIRV/3duiFrag.spv

%glslcLocation% skybox/skybox.vert -o SPIRV/skyboxVert.spv
%glslcLocation% skybox/skybox.frag -o SPIRV/skyboxFrag.spv
%glslcLocation% skybox/skyboxFilterCube.vert -o SPIRV/skyboxFilterCubeVert.spv
%glslcLocation% skybox/skyboxCubemapWrite.frag -o SPIRV/skyboxCubemapWriteFrag.spv
%glslcLocation% skybox/skyboxCubemapIrradiance.frag -o SPIRV/skyboxCubemapIrradianceFrag.spv
%glslcLocation% skybox/skyboxCubemapPrefilteredMap.frag -o SPIRV/skyboxCubemapPrefilteredMapFrag.spv

%glslcLocation% quad.vert -o SPIRV/quadVert.spv
%glslcLocation% genBRDFLUT.frag -o SPIRV/genBRDFLUTFrag.spv

%glslcLocation% post/highlight.frag -o SPIRV/highlightFrag.spv

%glslcLocation% --target-spv=spv1.4 rt/raygen.rgen -o SPIRV/rt/raygen.rgen.spv
%glslcLocation% --target-spv=spv1.4 rt/raychit.rchit -o SPIRV/rt/raychit.rchit.spv
%glslcLocation% --target-spv=spv1.4 rt/raymiss.rmiss -o SPIRV/rt/raymiss.rmiss.spv
%glslcLocation% --target-spv=spv1.4 rt/shadow.rmiss -o SPIRV/rt/shadow.rmiss.spv

pause
