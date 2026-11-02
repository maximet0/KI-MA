
cbuffer RootConstants : register(b0)
{
    uint M;
    uint N;
    uint K;
};

StructuredBuffer<float> a : register(t0);
StructuredBuffer<float> b : register(t1);
RWStructuredBuffer<float> result : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint index = id.x;
  
    if (index >= M * N)
        return;
    
    uint row = index / N;
    uint col = index % N;
    
    float sum = 0.0f;
    
    for (uint k = 0; k < K; ++k)
        sum += a[row * K + k] * b[k * N + col];
  
    
    result[row * N + col] = sum;
}