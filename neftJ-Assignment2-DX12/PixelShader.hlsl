#include "Lighting.hlsli"
#define MAX_LIGHTS 128
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION;
};

// Alignment matters!!!
cbuffer ExternalData : register(b0)
{
	float2 uvScale;
	float2 uvOffset;
	float3 cameraPosition;
    //float3 colorTint;
	int lightCount;
	Light lights[MAX_LIGHTS];
}

Texture2D AlbedoTexture : register(t0);
Texture2D NormalMap : register(t1);
Texture2D RoughnessMap : register(t2);
Texture2D MetalMap : register(t3);

SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
	// Clean up un-normalized normals
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
	
	// Scale and offset uv as necessary
    input.uv = input.uv * uvScale + uvOffset;

	// Normal mapping
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);

	// Surface color with gamma correction
    float4 surfaceColor = AlbedoTexture.Sample(BasicSampler, input.uv);
    surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);

	// Sample the other maps
    float roughness = RoughnessMap.Sample(BasicSampler, input.uv).r;
    float metal = MetalMap.Sample(BasicSampler, input.uv).r;
	
	// Specular color - Assuming albedo texture is actually holding specular color if metal == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match
    float3 specColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metal);

	// Keep a running total of light
    float3 totalLight = float3(0, 0, 0);

	// Loop and handle all lights
    for (int i = 0; i < lightCount; i++)
    {
		// Grab this light and normalize the direction (just in case)
        Light light = lights[i];
        light.Direction = normalize(light.Direction);

		// Run the correct lighting calculation based on the light's type
        switch (light.Type)
        {
            case LIGHT_TYPE_DIRECTIONAL:
                totalLight += DirLight(light, input.normal, input.worldPos, cameraPosition, roughness, surfaceColor.rgb);
                break;

            case LIGHT_TYPE_POINT:
                totalLight += PointLight(light, input.normal, input.worldPos, cameraPosition, roughness, surfaceColor.rgb);
                break;

            case LIGHT_TYPE_SPOT:
                totalLight += SpotLight(light, input.normal, input.worldPos, cameraPosition, roughness, surfaceColor.rgb);
                break;
        }
    }
    
    float3 test = AlbedoTexture.Sample(ClampSampler, float2(1, 1));

	// Gamma correct and return
    return float4(pow(totalLight, 1.0f / 2.2f), 1.0f * test.x);
}