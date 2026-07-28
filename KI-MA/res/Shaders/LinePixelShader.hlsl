
struct PSInput
{
    float4 position : SV_POSITION;
    uint color : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
    uint a = input.color & 0xFF;

    uint b = (input.color >> 8) & 0xFF;
    uint g = (input.color >> 16) & 0xFF;
    uint r = (input.color >> 24) & 0xFF;    
    return float4(r, g, b, a) / 255.0f;
}