
struct VSInput
{
    float2 position : POSITION;
    uint color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    uint color : TEXCOORD0;
};

cbuffer CameraBuffer : register(b0)
{
    float4x4 viewProjectionMatrix;
};

VSOutput main(VSInput input)
{
    float4 pos = float4(input.position, 0.0f, 1.0f);
    
    VSOutput output;
    output.position = mul(viewProjectionMatrix, pos);
    output.color = input.color;
    return output;
}