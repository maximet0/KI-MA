
cbuffer RootConstants : register(b0)
{
    uint M;
    uint N;
    uint K;
};

StructuredBuffer<float> a : register(t0);
StructuredBuffer<float> b : register(t1);
RWStructuredBuffer<float> result : register(u0);

#define THREADS_X 16
#define THREADS_Y 16
#define OUT_ROWS_PER_THREAD 4
#define OUT_COLS_PER_THREAD 4

#define OUT_TILE_ROWS (THREADS_Y * OUT_ROWS_PER_THREAD) // 64
#define OUT_TILE_COLS (THREADS_X * OUT_COLS_PER_THREAD) // 64
#define RED_TILE_SIZE 16
#define THREAD_COUNT (THREADS_X * THREADS_Y)

groupshared float tileA[OUT_TILE_ROWS][RED_TILE_SIZE];
groupshared float tileB[RED_TILE_SIZE][OUT_TILE_COLS];

[numthreads(16, 16, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID)
{
    uint threadIndex = threadID.y * THREADS_X + threadID.x;
    uint groupRow = groupID.y * OUT_TILE_ROWS;
    uint groupCol = groupID.x * OUT_TILE_COLS;

    float sums[OUT_ROWS_PER_THREAD][OUT_COLS_PER_THREAD] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    for (uint tile = 0; tile < (K + RED_TILE_SIZE - 1) / RED_TILE_SIZE; ++tile) {

        [unroll]
        for (uint i = 0; i < OUT_ROWS_PER_THREAD; i++)
        {
            uint index = threadIndex + i * THREAD_COUNT;
            uint localRow = index / RED_TILE_SIZE;
            uint localCol = index % RED_TILE_SIZE;
            uint globalRow = groupID.y * OUT_TILE_ROWS + localRow;
            uint globalCol = tile * RED_TILE_SIZE + localCol;
            
            tileA[localRow][localCol] = (globalRow < M && globalCol < K) ? a[globalRow * K + globalCol] : 0.0f;
        }
        
        [unroll]
        for (uint i = 0; i < OUT_COLS_PER_THREAD; i++)
        {
            uint index = threadIndex + i * THREAD_COUNT;
            uint localRow = index / OUT_TILE_COLS;
            uint localCol = index % OUT_TILE_COLS;
            
            uint globalRow = tile * RED_TILE_SIZE + localRow;
            uint globalCol = groupCol + localCol;
        
            tileB[localRow][localCol] = (globalRow < K && globalCol < N) ? b[globalRow * N + globalCol] : 0.0f;
        }
        
        GroupMemoryBarrierWithGroupSync();
        
        [unroll]
        for (uint k = 0; k < RED_TILE_SIZE; k++)
        {
            float valA[OUT_ROWS_PER_THREAD];
            float valB[OUT_COLS_PER_THREAD];
            
            [unroll]
            for (uint i = 0; i < OUT_ROWS_PER_THREAD; i++)
                valA[i] = tileA[threadID.y + i * THREADS_Y][k];
            
            [unroll]
            for (uint i = 0; i < OUT_COLS_PER_THREAD; i++)
                valB[i] = tileB[k][threadID.x + i * THREADS_X];
            
            [unroll]
            for (uint i = 0; i < OUT_ROWS_PER_THREAD; i++)
            {
                [unroll]
                for (uint j = 0; j < OUT_COLS_PER_THREAD; j++)
                    sums[i][j] = mad(valA[i], valB[j], sums[i][j]);
            }

        }
           
        
        GroupMemoryBarrierWithGroupSync();
        
    }
    
    [unroll]
    for (uint i = 0; i < OUT_ROWS_PER_THREAD; ++i)
    {
        [unroll]
        for (uint j = 0; j < OUT_COLS_PER_THREAD; ++j)
        {
            uint row = groupRow + threadID.y + i * THREADS_Y;
            uint col = groupCol + threadID.x + j * THREADS_X;
            if (row < M && col < N)
                result[row * N + col] = sums[i][j];
        }
    }
        
}