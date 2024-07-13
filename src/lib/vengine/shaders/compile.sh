compiler=${VULKAN_SDK}/bin/glslangValidator

$compiler -V --target-env spirv1.4 standard.vert.glsl -o SPIRV/standard.vert.spv
$compiler -V --target-env spirv1.4 pbrForwardOpaque.frag.glsl -o SPIRV/pbrForwardOpaque.frag.spv
$compiler -V --target-env spirv1.4 pbrForwardTransparent.frag.glsl -o SPIRV/pbrForwardTransparent.frag.spv
$compiler -V --target-env spirv1.4 lambertForwardOpaque.frag.glsl -o SPIRV/lambertForwardOpaque.frag.spv
$compiler -V --target-env spirv1.4 lambertForwardTransparent.frag.glsl -o SPIRV/lambertForwardTransparent.frag.spv
$compiler -V ui/3duiTransform.vert.glsl -o SPIRV/3duiTransform.vert.spv
$compiler -V ui/3duiTransform.frag.glsl -o SPIRV/3duiTransform.frag.spv
$compiler -V ui/3duiAABB.vert.glsl -o SPIRV/3duiAABB.vert.spv
$compiler -V ui/3duiAABB.geom.glsl -o SPIRV/3duiAABB.geom.spv
$compiler -V ui/3duiAABB.frag.glsl -o SPIRV/3duiAABB.frag.spv

$compiler -V skybox/skybox.vert.glsl -o SPIRV/skybox.vert.spv
$compiler -V skybox/skybox.frag.glsl -o SPIRV/skybox.frag.spv
$compiler -V skybox/skyboxFilterCube.vert.glsl -o SPIRV/skyboxFilterCube.vert.spv
$compiler -V skybox/skyboxCubemapWrite.frag.glsl -o SPIRV/skyboxCubemapWrite.frag.spv
$compiler -V skybox/skyboxCubemapIrradiance.frag.glsl -o SPIRV/skyboxCubemapIrradiance.frag.spv
$compiler -V skybox/skyboxCubemapPrefilteredMap.frag.glsl -o SPIRV/skyboxCubemapPrefilteredMap.frag.spv

$compiler -V quad.vert.glsl -o SPIRV/quad.vert.spv
$compiler -V genBRDFLUT.frag.glsl -o SPIRV/genBRDFLUT.frag.spv

$compiler -V post/highlight.frag.glsl -o SPIRV/highlight.frag.spv

$compiler -V --target-env spirv1.4 pt/raygen.rgen.glsl -o SPIRV/pt/raygen.rgen.spv
$compiler -V --target-env spirv1.4 pt/raychitLambert.rchit.glsl -o SPIRV/pt/raychitLambert.rchit.spv
$compiler -V --target-env spirv1.4 pt/raychitPBRStandard.rchit.glsl -o SPIRV/pt/raychitPBRStandard.rchit.spv
$compiler -V --target-env spirv1.4 pt/raychitVolume.rchit.glsl -o SPIRV/pt/raychitVolume.rchit.spv
$compiler -V --target-env spirv1.4 pt/rayahitPrimary.rahit.glsl -o SPIRV/pt/rayahitPrimary.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayahitSecondary.rahit.glsl -o SPIRV/pt/rayahitSecondary.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayahitSecondaryVolume.rahit.glsl -o SPIRV/pt/rayahitSecondaryVolume.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayahitNEE.rahit.glsl -o SPIRV/pt/rayahitNEE.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayahitNEEVolume.rahit.glsl -o SPIRV/pt/rayahitNEEVolume.rahit.spv
$compiler -V --target-env spirv1.4 pt/raymissPrimary.rmiss.glsl -o SPIRV/pt/raymissPrimary.rmiss.spv
$compiler -V --target-env spirv1.4 pt/raymissSecondary.rmiss.glsl -o SPIRV/pt/raymissSecondary.rmiss.spv
$compiler -V --target-env spirv1.4 pt/raymissNEE.rmiss.glsl -o SPIRV/pt/raymissNEE.rmiss.spv

