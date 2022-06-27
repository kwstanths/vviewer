set glslcLocation=C:\Users\konstantinos\Documents\LIBS\VulkanSDK\1.3.204.1\Bin\glslc.exe

%glslcLocation% standard.vert -o SPIRV/standardVert.spv
%glslcLocation% pbr.frag -o SPIRV/pbrFrag.spv
%glslcLocation% lambert.frag -o SPIRV/lambertFrag.spv

%glslcLocation% skybox/skybox.vert -o SPIRV/skyboxVert.spv
%glslcLocation% skybox/skybox.frag -o SPIRV/skyboxFrag.spv
%glslcLocation% skybox/skyboxFilterCube.vert -o SPIRV/skyboxFilterCubeVert.spv
%glslcLocation% skybox/skyboxCubemapWrite.frag -o SPIRV/skyboxCubemapWriteFrag.spv
%glslcLocation% skybox/skyboxCubemapIrradiance.frag -o SPIRV/skyboxCubemapIrradianceFrag.spv
%glslcLocation% skybox/skyboxCubemapPrefilteredMap.frag -o SPIRV/skyboxCubemapPrefilteredMapFrag.spv

%glslcLocation% quad.vert -o SPIRV/quadVert.spv
%glslcLocation% genBRDFLUT.frag -o SPIRV/genBRDFLUTFrag.spv

%glslcLocation% --target-spv=spv1.4 rt/raygen.rgen -o SPIRV/rt/raygen.rgen.spv
%glslcLocation% --target-spv=spv1.4 rt/raychit.rchit -o SPIRV/rt/raychit.rchit.spv
%glslcLocation% --target-spv=spv1.4 rt/raymiss.rmiss -o SPIRV/rt/raymiss.rmiss.spv
%glslcLocation% --target-spv=spv1.4 rt/shadow.rmiss -o SPIRV/rt/shadow.rmiss.spv

pause