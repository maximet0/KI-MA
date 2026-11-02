
cbuffer RootConstants : register(b0)
{
    uint elementCount;
};

StructuredBuffer<float> a : register(t0);
RWStructuredBuffer<float> grad : register(u0);


[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= elementCount) return;

    grad[index] = a[index] > 0.0f ? grad[index] : 0.0f;
}