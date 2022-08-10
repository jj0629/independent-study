#include "Lighting.hlsli"

struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer ExternalData : register(b1)
{
    float3 cameraPos;
    float4x4 projection;
    float4x4 view;
};

Texture3D volumetricLight : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    // Grab the direction and distance between the pixel in question and the camera
    float3 dir = cameraPos - input.position.xyz;
    dir = normalize(dir);
    float distance = distance(cameraPos, input.position.xyz);
    
    // Do a ray march through the volumetric texture, adding light at each step
    float increment = 0.1;
    float3 totalLight = (0, 0, 0);
    for (int i = 0; i < distance; i += increment)
    {
        totalLight += volumetricLight.Sample(BasicSampler, cameraPos + (dir * increment));
    }
    
    return float4(totalLight, 1.0f);
}