compiler=${VULKAN_SDK}/bin/glslangValidator

$compiler -V --target-env spirv1.4 standard.vert.glsl -o SPIRV/standard.vert.spv
$compiler -V --target-env spirv1.4 gbuffer.frag.glsl -o SPIRV/gbuffer.frag.spv
$compiler -V --target-env spirv1.4 lightCompositionIBL.frag.glsl -o SPIRV/lightCompositionIBL.frag.spv
$compiler -V --target-env spirv1.4 lightCompositionPoint.frag.glsl -o SPIRV/lightCompositionPoint.frag.spv
$compiler -V --target-env spirv1.4 lightCompositionDirectional.frag.glsl -o SPIRV/lightCompositionDirectional.frag.spv
$compiler -V --target-env spirv1.4 pbrForward.frag.glsl -o SPIRV/pbrForward.frag.spv
$compiler -V --target-env spirv1.4 lambertForward.frag.glsl -o SPIRV/lambertForward.frag.spv

$compiler -V overlay/transform3D.vert.glsl -o SPIRV/overlay/transform3D.vert.spv
$compiler -V overlay/transform3D.frag.glsl -o SPIRV/overlay/transform3D.frag.spv
$compiler -V overlay/AABB3.vert.glsl -o SPIRV/overlay/AABB3.vert.spv
$compiler -V overlay/AABB3.geom.glsl -o SPIRV/overlay/AABB3.geom.spv
$compiler -V overlay/AABB3.frag.glsl -o SPIRV/overlay/AABB3.frag.spv
$compiler -V overlay/outline.vert.glsl -o SPIRV/overlay/outline.vert.spv
$compiler -V overlay/outline.frag.glsl -o SPIRV/overlay/outline.frag.spv

$compiler -V skybox/skybox.vert.glsl -o SPIRV/skybox.vert.spv
$compiler -V skybox/skybox.frag.glsl -o SPIRV/skybox.frag.spv
$compiler -V skybox/skyboxFilterCube.vert.glsl -o SPIRV/skyboxFilterCube.vert.spv
$compiler -V skybox/skyboxCubemapWrite.frag.glsl -o SPIRV/skyboxCubemapWrite.frag.spv
$compiler -V skybox/skyboxCubemapIrradiance.frag.glsl -o SPIRV/skyboxCubemapIrradiance.frag.spv
$compiler -V skybox/skyboxCubemapPrefilteredMap.frag.glsl -o SPIRV/skyboxCubemapPrefilteredMap.frag.spv

$compiler -V quad.vert.glsl -o SPIRV/quad.vert.spv
$compiler -V genBRDFLUT.frag.glsl -o SPIRV/genBRDFLUT.frag.spv

$compiler -V post/highlight.frag.glsl -o SPIRV/highlight.frag.spv
$compiler -V post/pass.frag.glsl -o SPIRV/pass.frag.spv
$compiler -V post/output.frag.glsl -o SPIRV/output.frag.spv

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

