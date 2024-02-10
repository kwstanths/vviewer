rayQueryEXT rayQuery;
rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsNoOpaqueEXT, 0xFF, fragPos_world, 0.001, rayquery_direction, rayquery_distance);
while(rayQueryProceedEXT(rayQuery))
{
}
bool occluded = true;
if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT ) {
    occluded = false;
}