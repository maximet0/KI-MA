
cbuffer RootConstants : register(b0)
{
    uint elementCount;
    float learningRate;
    float beta1;
    float beta2;
    float epsilon;
    float decayFactor;
    float correction1;
    float correction2;
};

RWStructuredBuffer<float> param : register(u0);
StructuredBuffer<float> gradient : register(t0);
RWStructuredBuffer<float> m : register(u1);
RWStructuredBuffer<float> v : register(u2);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint index = id.x;

    if (index >= elementCount)
        return;
    
    float curM = beta1 * m[index] + (1.0f - beta1) * gradient[index];
    float curV = beta2 * v[index] + (1.0f - beta2) * gradient[index] * gradient[index];
    
    float mHat = curM / correction1;
    float vHat = curV / correction2;
    
    float denominator = sqrt(vHat) + epsilon;
    float update = learningRate * mHat / denominator;
    
    param[index] = param[index] * decayFactor - update;
    m[index] = curM;
    v[index] = curV;
}