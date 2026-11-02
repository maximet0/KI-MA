
cbuffer RootConstants : register(b0)
{
    uint batchSize;
    uint neuronCount;

};

StructuredBuffer<float> gradient : register(t0);
RWStructuredBuffer<float> biasGradient : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint neuron = id.x;
    if (neuron >= neuronCount)
        return;
        
    float sum = 0.0f;
    for (uint batch = 0; batch < batchSize; batch++)
        sum += gradient[batch * neuronCount + neuron];
    
    biasGradient[neuron] = sum;
}