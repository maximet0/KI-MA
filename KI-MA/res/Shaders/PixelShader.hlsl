
struct Rectangle
{
    float2 position;
    float2 size;
    uint textureIndex;
};


struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint textureID : TEXCOORD1;
};

StructuredBuffer<Rectangle> rectInfo : register(t0);

Texture2D textures[2048] : register(t1);
SamplerState linearSampler : register(s0);

float4 main(PSInput input) : SV_TARGET
{
    return textures[input.textureID].Sample(linearSampler, input.uv);
}