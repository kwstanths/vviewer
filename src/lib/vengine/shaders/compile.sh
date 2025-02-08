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

$compiler -V post/pass.frag.glsl -o SPIRV/pass.frag.spv
$compiler -V post/output.frag.glsl -o SPIRV/output.frag.spv

$compiler -V --target-env spirv1.4 pt/raygen.rgen.glsl -o SPIRV/pt/raygen.rgen.spv
$compiler -V --target-env spirv1.4 pt/rayPrimaryLambert.rchit.glsl -o SPIRV/pt/rayPrimaryLambert.rchit.spv
$compiler -V --target-env spirv1.4 pt/rayPrimaryPBRStandard.rchit.glsl -o SPIRV/pt/rayPrimaryPBRStandard.rchit.spv
$compiler -V --target-env spirv1.4 pt/rayPrimary.rahit.glsl -o SPIRV/pt/rayPrimary.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayPrimary.rmiss.glsl -o SPIRV/pt/rayPrimary.rmiss.spv
$compiler -V --target-env spirv1.4 pt/raySecondary.rchit.glsl -o SPIRV/pt/raySecondary.rchit.spv
$compiler -V --target-env spirv1.4 pt/raySecondary.rahit.glsl -o SPIRV/pt/raySecondary.rahit.spv
$compiler -V --target-env spirv1.4 pt/raySecondary.rmiss.glsl -o SPIRV/pt/raySecondary.rmiss.spv
$compiler -V --target-env spirv1.4 pt/rayNEE.rchit.glsl -o SPIRV/pt/rayNEE.rchit.spv
$compiler -V --target-env spirv1.4 pt/rayNEE.rahit.glsl -o SPIRV/pt/rayNEE.rahit.spv
$compiler -V --target-env spirv1.4 pt/rayNEE.rmiss.glsl -o SPIRV/pt/rayNEE.rmiss.spv
