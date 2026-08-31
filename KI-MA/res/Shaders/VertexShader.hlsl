
struct VSInput
{
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    uint instanceID : SV_InstanceID;
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
    float2 textureScale;
    float zLayer;
    uint textureIndex;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
};

cbuffer StartInstance : register(b1)
{
    uint startInstance;
};

StructuredBuffer<Rectangle> rectInfo : register(t0);


VSOutput main(VSInput input)
{
    uint instanceID = input.instanceID + startInstance;
    VSOutput output;
    float2 transform = input.position * rectInfo[instanceID].size + rectInfo[instanceID].position;
    float4 pos = float4(transform, rectInfo[instanceID].zLayer, 1.0f);
    output.position = mul(viewProjectionMatrix, pos);
    output.uv = input.uv * rectInfo[instanceID].textureScale;
    output.textureID = rectInfo[instanceID].textureIndex;
    return output;
}