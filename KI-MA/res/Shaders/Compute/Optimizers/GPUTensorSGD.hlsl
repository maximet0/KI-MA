
cbuffer RootConstants : register(b0)
{
    uint elementCount;
    float learningRate;
};

RWStructuredBuffer<float> param : register(u0);
StructuredBuffer<float> gradient : register(t0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint index = id.x;

    if (index >= elementCount)
        return;
    
    param[index] -= learningRate * gradient[index];
}