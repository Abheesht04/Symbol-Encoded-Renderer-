PS_INPUT VSMain(VS_INPUT input) {
    PS_INPUT output;

    // For now, we are just passing positions through.
    // Later, we will multiply by a 'Model-View-Projection' matrix here.
    output.pos = float4(input.pos, 1.0f);

    output.worldNormal = input.normal;
    output.uv = input.uv;

    return output;
}