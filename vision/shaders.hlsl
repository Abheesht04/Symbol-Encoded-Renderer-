cbuffer Camera : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput o;

    o.pos = mul(mvp, float4(input.pos, 1.0));
    o.col = input.col;

    return o;
}
