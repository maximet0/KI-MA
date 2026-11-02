
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
    
    float x = a[index];
    float sigmoid = 1.0f / (1.0f + exp(-x));
    float deriv = sigmoid + x * sigmoid * (1.0f - sigmoid);
    
    grad[index] *= deriv;
}