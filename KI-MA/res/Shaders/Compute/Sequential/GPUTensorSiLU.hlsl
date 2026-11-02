
cbuffer RootConstants : register(b0)
{
    uint elementCount;
};

StructuredBuffer<float> a : register(t0);
RWStructuredBuffer<float> result : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= elementCount)
        return;
    
    result[index] = a[index] / (1.0f + exp(-a[index]));
}