echo off

set compiler=%VULKAN_SDK%\Bin\glslangValidator.exe

%compiler% standard.vert.glsl -o SPIRV/standard.vert.spv
%compiler% pbrBase.frag.glsl -o SPIRV/pbrBase.frag.spv
%compiler% pbrAdd.frag.glsl -o SPIRV/pbrAdd.frag.spv
%compiler% lambertBase.frag.glsl -o SPIRV/lambertBase.frag.spv
%compiler% lambertAdd.frag.glsl -o SPIRV/lambertAdd.frag.spv
%compiler% 3dui.vert.glsl -o SPIRV/3dui.vert.spv
%compiler% 3dui.frag.glsl -o SPIRV/3dui.frag.spv

%compiler% skybox/skybox.vert.glsl -o SPIRV/skybox.vert.spv
%compiler% skybox/skybox.frag.glsl -o SPIRV/skybox.frag.spv
%compiler% skybox/skyboxFilterCube.vert.glsl -o SPIRV/skyboxFilterCube.vert.spv
%compiler% skybox/skyboxCubemapWrite.frag.glsl -o SPIRV/skyboxCubemapWrite.frag.spv
%compiler% skybox/skyboxCubemapIrradiance.frag.glsl -o SPIRV/skyboxCubemapIrradiance.frag.spv
%compiler% skybox/skyboxCubemapPrefilteredMap.frag.glsl -o SPIRV/skyboxCubemapPrefilteredMap.frag.spv

%compiler% quad.vert.glsl -o SPIRV/quad.vert.spv
%compiler% genBRDFLUT.frag.glsl -o SPIRV/genBRDFLUT.frag.spv

%compiler% post/highlight.frag.glsl -o SPIRV/highlight.frag.spv

%compiler% --target-spv=spv1.4 rt/raygen.rgen.glsl -o SPIRV/rt/raygen.rgen.spv
%compiler% --target-spv=spv1.4 rt/raychit.rchit.glsl -o SPIRV/rt/raychit.rchit.spv
%compiler% --target-spv=spv1.4 rt/raymiss.rmiss.glsl -o SPIRV/rt/raymiss.rmiss.spv
%compiler% --target-spv=spv1.4 rt/shadow.rmiss.glsl -o SPIRV/rt/shadow.rmiss.spv

