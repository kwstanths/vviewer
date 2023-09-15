#include "common.glsl"
#include "../sampling.glsl"

struct PBRStandard {
    vec3 albedo;
    float metallic;
    float roughness;
};

vec3 evalDisneyDiffuse(const in float NdotL, const in float NdotV, const in float LdotH, const in PBRStandard pbr) {
    
    float FL = schlickWeight(NdotL); 
    float FV = schlickWeight(NdotV);
    
    float Fd90 = 0.5 + 2. * LdotH * LdotH * pbr.roughness;
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);
    
    return (1.0 / PI) * Fd * pbr.albedo;
}

void sampleDisneyDiffuse(out vec3 wi, out float pdf, const in vec2 u) {
	wi = cosineSampleHemisphere(u, pdf);
}

vec3 evalDisneyMicrofacetIsotropic(float NdotL, float NdotV, float NdotH, float LdotH, const in PBRStandard pbr) {
    
    float Cdlum = 0.3 * pbr.albedo.r + 0.6 * pbr.albedo.g + 0.1 * pbr.albedo.b; // luminance approx.

    vec3 Ctint = Cdlum > 0. ? pbr.albedo / Cdlum : vec3(1.); // normalize lum. to isolate hue+sat
    
    float specular = 0.5;
    float specularTint = 0.0;
    vec3 Cspec0 = mix(specular * 0.08 * mix(vec3(1.0), Ctint, specularTint), pbr.albedo, pbr.metallic);
    
    float a = max(0.0001, pbr.roughness * pbr.roughness);
    float Ds = GTR2(NdotH, a);
    float FH = schlickWeight(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
    float Gs;
    Gs  = smithG_GGX(NdotL, a);
    Gs *= smithG_GGX(NdotV, a);
    
    return Gs * Fs * Ds;
}

float pdfDisneyMicrofacetIsotropic(const in vec3 wi, const in vec3 wo, const in PBRStandard pbr) {
    
    if (!(wo.y > 0)) return 0.;
    if (!(wi.y > 0)) return 0.;
    
    vec3 wh = normalize(wo + wi);
    
    float NdotH = max(wh.y, 0.0);
    float alpha2 = pbr.roughness * pbr.roughness;
    alpha2 *= alpha2;
    
    float cos2Theta = NdotH * NdotH;
    float denom = cos2Theta * ( alpha2 - 1.0) + 10.;
    if( denom == 0.0 ) return 0.0;

    float pdfDistribution = alpha2 * NdotH / (PI * denom * denom);
    return pdfDistribution/(4. * dot(wo, wh));
}

void sampleDisneyMicrofacetIsotropic(out vec3 wi, const in vec3 wo, out float pdf, const in vec2 u, const in PBRStandard pbr) {
    float cosTheta = 0.0;
    float phi = (2.0 * PI) * u[1];
    float alpha = pbr.roughness * pbr.roughness;
    float tanTheta2 = alpha * alpha * u[0] / (1.0 - u[0]);
    cosTheta = 1.0 / sqrt(1.0 + tanTheta2);
    
    float sinTheta = sqrt(max(EPSILON, 1. - cosTheta * cosTheta));
    vec3 wh = vec3(sinTheta * cos(phi), cosTheta, sinTheta * sin(phi));
    
    if(!(wh.y > 0)) {
        wh *= -1.;
    }

    wi = reflect(-wo, wh);
    pdf = pdfDisneyMicrofacetIsotropic(wi, wo, pbr);
}

float getDiffuseSamplingRatio(PBRStandard pbr)
{
    /* Add at least 10% sampling to each lobe */
    float diffuseSamplingRatio = max(1.0 - pbr.metallic, 0.1);
    float glossySamplingRatio = max(1.0 - pbr.roughness, 0.1);

    return diffuseSamplingRatio / (diffuseSamplingRatio + glossySamplingRatio);
}

vec3 evalPBRStandard(const in PBRStandard pbr, const in vec3 wi, const in vec3 wo, vec3 H)
{
    float NdotL = wi.y;
    float NdotV = wo.y;
    if (NdotL < 0.0 || NdotV < 0.0) return vec3(0.0);
    
    float NdotH = H.y;
    float LdotH = dot(wi, H);
    
    vec3 diffuse = evalDisneyDiffuse(NdotL, NdotV, LdotH, pbr);
    vec3 glossy = evalDisneyMicrofacetIsotropic(NdotL, NdotV, NdotH, LdotH, pbr);
    
    return (diffuse * (1.0 - pbr.metallic) + glossy) * NdotL;
}

float pdfPBRStandard(const in vec3 wi, const in vec3 wo, const in PBRStandard pbr)
{
    float cosTheta = wi.y;

    if (cosTheta < 0) {
        return 0.0;
    }

    float pdfDiffuse = cosineSampleHemispherePdf(cosTheta);
    float pdfMicrofacet = pdfDisneyMicrofacetIsotropic(wi, wo, pbr);

    float diffuseSamplingRatio = getDiffuseSamplingRatio(pbr);

    return pdfDiffuse * diffuseSamplingRatio + pdfMicrofacet * (1.0 - diffuseSamplingRatio);
}

vec3 samplePBRStandard(out vec3 wi, const in vec3 wo, out float pdf, const in PBRStandard pbr, vec2 randoms2D, float randoms1D) {
    
    pdf = 0.0;
	wi = vec3(0.0);
    
    vec2 u = randoms2D;
    float rnd = randoms1D;

    float diffuseSamplingRatio = getDiffuseSamplingRatio(pbr);

	if( rnd <= diffuseSamplingRatio ) {
       sampleDisneyDiffuse(wi, pdf, u);
    }
    else {
       sampleDisneyMicrofacetIsotropic(wi, wo, pdf, u, pbr);
    }

    vec3 F = evalPBRStandard(pbr, wi, wo, normalize(wi + wo));
    pdf = pdfPBRStandard(wi, wo, pbr);

    if (pdf < EPSILON)
    {
        return vec3(0.0);
    }

	return F;
}
