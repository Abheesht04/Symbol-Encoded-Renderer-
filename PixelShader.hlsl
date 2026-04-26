//float4 PSMain(PS_INPUT input) : SV_TARGET{
    // Basic Directional Light (like the Sun in Crimson Desert)
    //float3 lightDir = normalize(float3(0.5f, -1.0f, 0.5f));
    //float3 normal = normalize(input.worldNormal);

    // Simple Lambertian Reflection: Intensity = Normal . LightDirection
    // We use saturate() to clamp between 0 and 1, just like in C++ math libs
    //float diffuse = saturate(dot(normal, -lightDir));

    // Ambient light so the shadows aren't pitch black
    //float3 ambient = float3(0.1f, 0.1f, 0.1f);

    // Final Color (Let's start with a neutral gray for the model)
    //float3 baseColor = float3(0.7f, 0.7f, 0.7f);
    //float3 finalColor = baseColor * (diffuse + ambient);

    //return float4(finalColor, 1.0f);
//}