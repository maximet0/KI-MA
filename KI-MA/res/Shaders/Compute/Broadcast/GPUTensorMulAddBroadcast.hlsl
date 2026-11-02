

cbuffer RootConstants : register(b0)
{
    uint4 totalSize;
    uint4 resultShape;
    uint4 stridesA;
    uint4 stridesB;
    uint4 stridesC;
};


StructuredBuffer<float> a : register(t0);
StructuredBuffer<float> b : register(t1);
StructuredBuffer<float> c : register(t2);
RWStructuredBuffer<float> result : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.x;
    if (index >= totalSize.x)
        return;
    
    uint remainingIndex = index;
    uint indexA = 0, indexB = 0, indexC = 0;
    
    for (uint d = 4; d > 0;)
    {
        d--;
        uint coord = remainingIndex % resultShape[d];
        remainingIndex /= resultShape[d];
        
        indexA += coord * stridesA[d];
        indexB += coord * stridesB[d];
        indexC += coord * stridesC[d];
    }
    
    result[index] = mad(a[indexA], b[indexB], c[indexC]);
}