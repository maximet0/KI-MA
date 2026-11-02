
cbuffer RootConstants : register(b0)
{
    uint rows;
    uint cols;
};

StructuredBuffer<float> a : register(t0);
RWStructuredBuffer<float> result : register(u0);

[numthreads(16, 16, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint col = id.x;
    uint row = id.y;
    
    if (row >= rows || col >= cols)
        return;
    
    result[row * cols + col] = a[col * rows + row];
}