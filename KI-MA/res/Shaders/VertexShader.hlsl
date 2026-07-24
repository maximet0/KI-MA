
struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    uint instanceID : SV_InstanceID;
    uint startinstanceLocation : SV_StartInstanceLocation;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    uint textureID : TEXCOORD1;
};

struct Rectangle
{
    float2 position;
    float2 size;
    uint textureIndex;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
};

StructuredBuffer<Rectangle> rectInfo : register(t0);


VSOutput main(VSInput input)
{
    uint instanceID = input.instanceID + input.startinstanceLocation;
    VSOutput output;
    float2 transform = input.position * rectInfo[instanceID].size + rectInfo[instanceID].position;
    float4 pos = float4(transform, 0.0f, 1.0f);
    output.position = mul(viewProjectionMatrix, pos);
    output.uv = input.uv;
    output.textureID = rectInfo[instanceID].textureIndex;
    return output;
}