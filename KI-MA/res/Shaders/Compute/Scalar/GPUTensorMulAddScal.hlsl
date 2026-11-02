
cbuffer RootConstants : register(b0)
{
    uint elementCount;
};

StructuredBuffer<float> a : register(t0);
StructuredBuffer<float> b : register(t1);
StructuredBuffer<float> c : register(t2);
RWStructuredBuffer<float> result : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= elementCount) return;
    result[index] = mad(a[index], b[0], c[0]);
}