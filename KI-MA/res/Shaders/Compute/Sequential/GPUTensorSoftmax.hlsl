
cbuffer RootConstants : register(b0)
{
    uint batchSize;
    uint classes;
};

StructuredBuffer<float> input : register(t0);
RWStructuredBuffer<float> result : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= batchSize) return;
    
    uint offset = index * classes;
    
    float maxValue = -3.402823466e+38F;
    for (uint c = 0; c < classes; c++)
        maxValue = max(maxValue, input[offset + c]);
    
    float sum = 0.0f;
    for (uint c = 0; c < classes; c++)
        sum += exp(input[offset + c] - maxValue);
 
    
    float invSum = 1.0f / sum;
    for (uint c = 0; c < classes; c++)
        result[offset + c] = exp(input[offset + c] - maxValue) * invSum;
}