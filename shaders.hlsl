cbuffer SceneBuffer : register(b0) { float4x4 mvp; };

struct VS_INPUT {
    float3 pos    : POSITION;
    float  symbol : TEXCOORD0;
    float3 normal : NORMAL;
    float4 color  : COLOR;
};

struct GS_OUT {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

VS_INPUT VSMain(VS_INPUT i) { return i; }

[maxvertexcount(36)]
void GSMain(point VS_INPUT input[1], inout TriangleStream<GS_OUT> triStream) {
    float3 p = input[0].pos;
    float4 baseCol = input[0].color;

    // Adjusted size for better visual distinction
    float s = 0.325f;
    float h = 0.65f;

    float3 v[8] = {
        p + float3(-s, 0, -s), p + float3(s, 0, -s), p + float3(s, h, -s), p + float3(-s, h, -s),
        p + float3(-s, 0,  s), p + float3(s, 0,  s), p + float3(s, h,  s), p + float3(-s, h,  s)
    };

    // Use uint for shader indices
    uint indices[36] = {
        0,3,2, 0,2,1, // Front
        5,6,7, 5,7,4, // Back
        4,7,3, 4,3,0, // Left
        1,2,6, 1,6,5, // Right
        3,7,6, 3,6,2, // Top
        4,0,1, 4,1,5  // Bottom
    };

    [unroll]
    for (int i = 0; i < 36; i += 3) {
        float3 v0 = v[indices[i]];
        float3 v1 = v[indices[i + 1]];
        float3 v2 = v[indices[i + 2]];

        // Calculate face normal for lighting
        float3 faceNorm = normalize(cross(v1 - v0, v2 - v0));
        float light = saturate(dot(faceNorm, normalize(float3(0.5, 1.0, -0.4)))) * 0.5 + 0.5;

        for (int j = 0; j < 3; j++) {
            GS_OUT o;
            o.pos = mul(float4(v[indices[i + j]], 1.0f), mvp);
            // Apply darker shading to sides than top
            float sideShade = (abs(faceNorm.y) > 0.5) ? 1.0 : 0.85;
            o.col = baseCol * light * sideShade;
            triStream.Append(o);
        }
        triStream.RestartStrip();
    }
}

float4 PSMain(GS_OUT i) : SV_TARGET{
    return i.col;
}