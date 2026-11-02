
cbuffer RootConstants : register(b0)
{
    uint elemCount;
    uint batchSize;
};

StructuredBuffer<float> results : register(t0);
StructuredBuffer<float> targets : register(t1);
RWStructuredBuffer<float> gradient : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= elemCount) return;
    
    gradient[index] = (results[index] - targets[index]) / batchSize;
}