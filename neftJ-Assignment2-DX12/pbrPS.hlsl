#include "Lighting.hlsli"

// How many lights could we handle?
#define MAX_LIGHTS 128

// Data that only changes once per frame
cbuffer perFrame : register(b1)
{
	// An array of light data
    Light lights[MAX_LIGHTS];

	// The amount of lights THIS FRAME
    int lightCount;

	// Needed for specular (reflection) calculation
    float3 cameraPosition;
	
	// Needed for IBL calculations
    //int SpecIBLTotalMipLevels;
};

// Data that can change per material
cbuffer perMaterial : register(b2)
{
	// Surface color
    float3 colorTint;
    float padding;

	// UV adjustments
    float2 uvScale;
    float2 uvOffset;
};

// Defines the input to this pixel shader
// - Should match the output of our corresponding vertex shader
struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION; // The world position of this PIXEL
};

struct PS_Output
{
    float4 colorNoAmbient : SV_Target0;
    float4 ambientColor : SV_Target1;
    float4 normals : SV_Target2;
    float4 depths : SV_Target3;
};

// Texture-related variables
Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalMap : register(t3);

// IBL (indirect PBR) textures
//Texture2D BrdfLookUpMap : register(t4); // New!
//TextureCube IrradianceIBLMap : register(t5); // New!
//TextureCube SpecularIBLMap : register(t6); // New!

// Samplers
SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1); // New!

// Entry point for this pixel shader
PS_Output main(VertexToPixel input)
{
	// Always re-normalize interpolated direction vectors
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);

	// Apply the uv adjustments
    input.uv = input.uv * uvScale + uvOffset;

	// Sample various textures
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float metal = MetalMap.Sample(BasicSampler, input.uv).r;

	// Gamma correct the texture back to linear space and apply the color tint
    float4 surfaceColor = Albedo.Sample(BasicSampler, input.uv);
    surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);

	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
    float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Total color for this pixel
    float3 totalColor = float3(0, 0, 0);

	// Loop through all lights this frame
    for (int i = 0; i < lightCount; i++)
    {
        Light currentLight = lights[i];
        currentLight.Direction = normalize(lights[i].Direction);
        
		// Which kind of light?
        switch (currentLight.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalColor += DirLightPBR(currentLight, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
                break;

            case LIGHT_TYPE_POINT:
                totalColor += PointLightPBR(currentLight, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
                break;

            case LIGHT_TYPE_SPOT:
                totalColor += SpotLightPBR(currentLight, input.normal, input.worldPos, cameraPosition, roughness, metal, surfaceColor.rgb, specColor);
                break;
        }
    }
	
	// Calculate requisite reflection vectors
    float3 viewToCam = normalize(cameraPosition - input.worldPos);
    float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
    float NdotV = saturate(dot(input.normal, viewToCam));

	// Indirect lighting
    //float3 indirectDiffuse = IndirectDiffuse(IrradianceIBLMap, BasicSampler, input.normal);
    //float3 indirectSpecular = IndirectSpecular(
	//SpecularIBLMap, SpecIBLTotalMipLevels,
	//BrdfLookUpMap, ClampSampler, // MUST use the clamp sampler here!
	//viewRefl, NdotV,
	//roughness, specColor);

	// Balance indirect diff/spec
    //float3 balancedDiff = DiffuseEnergyConserve(indirectDiffuse, indirectSpecular, metal);
    //float3 fullIndirect = indirectSpecular + balancedDiff * surfaceColor.rgb;

	// Add the indirect to the direct
    //totalColor += fullIndirect;
	
	// MRT Output
    //PS_Output output;
    //output.colorNoAmbient = float4(totalColor + indirectSpecular, 1);
    //output.ambientColor = float4(balancedDiff, 1);
    //output.normals = float4(input.normal * 0.5f + 0.5f, 1);
    //output.depths = input.screenPosition.z / input.screenPosition.w;
    PS_Output output;
    output.ambientColor = float4(1, 1, 1, 1);
    output.colorNoAmbient = float4(pow(totalColor, 1.0f / 2.2f), 1.0f);
    output.normals = float4(input.normal * 0.5f + 0.5f, 1);
    output.depths = input.screenPosition.z / input.screenPosition.w;
    
    return output;
}