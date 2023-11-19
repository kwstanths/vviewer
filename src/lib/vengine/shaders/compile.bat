echo off

set compiler=%VULKAN_SDK%\Bin\glslangValidator.exe

%compiler% -V standard.vert.glsl -o SPIRV/standard.vert.spv
%compiler% -V pbrBase.frag.glsl -o SPIRV/pbrBase.frag.spv
%compiler% -V lambertBase.frag.glsl -o SPIRV/lambertBase.frag.spv
%compiler% -V 3dui.vert.glsl -o SPIRV/3dui.vert.spv
%compiler% -V 3dui.frag.glsl -o SPIRV/3dui.frag.spv

%compiler% -V skybox/skybox.vert.glsl -o SPIRV/skybox.vert.spv
%compiler% -V skybox/skybox.frag.glsl -o SPIRV/skybox.frag.spv
%compiler% -V skybox/skyboxFilterCube.vert.glsl -o SPIRV/skyboxFilterCube.vert.spv
%compiler% -V skybox/skyboxCubemapWrite.frag.glsl -o SPIRV/skyboxCubemapWrite.frag.spv
%compiler% -V skybox/skyboxCubemapIrradiance.frag.glsl -o SPIRV/skyboxCubemapIrradiance.frag.spv
%compiler% -V skybox/skyboxCubemapPrefilteredMap.frag.glsl -o SPIRV/skyboxCubemapPrefilteredMap.frag.spv

%compiler% -V quad.vert.glsl -o SPIRV/quad.vert.spv
%compiler% -V genBRDFLUT.frag.glsl -o SPIRV/genBRDFLUT.frag.spv

%compiler% -V post/highlight.frag.glsl -o SPIRV/highlight.frag.spv

%compiler% -V --target-env spirv1.4 pt/raygen.rgen.glsl -o SPIRV/pt/raygen.rgen.spv
%compiler% -V --target-env spirv1.4 pt/raychitLambert.rchit.glsl -o SPIRV/pt/raychitLambert.rchit.spv
%compiler% -V --target-env spirv1.4 pt/raychitPBRStandard.rchit.glsl -o SPIRV/pt/raychitPBRStandard.rchit.spv
%compiler% -V --target-env spirv1.4 pt/rayahitPrimary.rahit.glsl -o SPIRV/pt/rayahitPrimary.rahit.spv
%compiler% -V --target-env spirv1.4 pt/rayahitSecondary.rahit.glsl -o SPIRV/pt/rayahitSecondary.rahit.spv
%compiler% -V --target-env spirv1.4 pt/rayahitNEE.rahit.glsl -o SPIRV/pt/rayahitNEE.rahit.spv
%compiler% -V --target-env spirv1.4 pt/raymissPrimary.rmiss.glsl -o SPIRV/pt/raymissPrimary.rmiss.spv
%compiler% -V --target-env spirv1.4 pt/raymissSecondary.rmiss.glsl -o SPIRV/pt/raymissSecondary.rmiss.spv
%compiler% -V --target-env spirv1.4 pt/raymissNEE.rmiss.glsl -o SPIRV/pt/raymissNEE.rmiss.spv
