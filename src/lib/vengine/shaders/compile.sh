compiler=${VULKAN_SDK}/bin/glslangValidator

$compiler -V standard.vert.glsl -o SPIRV/standard.vert.spv
$compiler -V pbrBase.frag.glsl -o SPIRV/pbrBase.frag.spv
$compiler -V lambertBase.frag.glsl -o SPIRV/lambertBase.frag.spv
$compiler -V 3dui.vert.glsl -o SPIRV/3dui.vert.spv
$compiler -V 3dui.frag.glsl -o SPIRV/3dui.frag.spv

$compiler -V skybox/skybox.vert.glsl -o SPIRV/skybox.vert.spv
$compiler -V skybox/skybox.frag.glsl -o SPIRV/skybox.frag.spv
$compiler -V skybox/skyboxFilterCube.vert.glsl -o SPIRV/skyboxFilterCube.vert.spv
$compiler -V skybox/skyboxCubemapWrite.frag.glsl -o SPIRV/skyboxCubemapWrite.frag.spv
$compiler -V skybox/skyboxCubemapIrradiance.frag.glsl -o SPIRV/skyboxCubemapIrradiance.frag.spv
$compiler -V skybox/skyboxCubemapPrefilteredMap.frag.glsl -o SPIRV/skyboxCubemapPrefilteredMap.frag.spv

$compiler -V quad.vert.glsl -o SPIRV/quad.vert.spv
$compiler -V genBRDFLUT.frag.glsl -o SPIRV/genBRDFLUT.frag.spv

$compiler -V post/highlight.frag.glsl -o SPIRV/highlight.frag.spv

$compiler -V --target-env spirv1.4 rt/raygen.rgen.glsl -o SPIRV/rt/raygen.rgen.spv
$compiler -V --target-env spirv1.4 rt/raychitLambert.rchit.glsl -o SPIRV/rt/raychitLambert.rchit.spv
$compiler -V --target-env spirv1.4 rt/raychitPBRStandard.rchit.glsl -o SPIRV/rt/raychitPBRStandard.rchit.spv
$compiler -V --target-env spirv1.4 rt/raymiss.rmiss.glsl -o SPIRV/rt/raymiss.rmiss.spv
$compiler -V --target-env spirv1.4 rt/shadow.rmiss.glsl -o SPIRV/rt/shadow.rmiss.spv

