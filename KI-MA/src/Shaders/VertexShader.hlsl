
struct VSInput
{
    float2 position : POSITION;
};

struct VSOutput
{
    float4 position : SV_POSITION;
};

struct Rectangle
{
    float2 position;
    float2 size;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
};

RWStructuredBuffer<Rectangle> rectInfo : register(u0);


VSOutput main(VSInput input)
{
    VSOutput output;
    float2 transform = input.position * rectInfo[0].size + rectInfo[0].position;
    float4 pos = float4(transform, 0.0f, 1.0f);
    output.position = mul(viewProjectionMatrix, pos);
    return output;
}