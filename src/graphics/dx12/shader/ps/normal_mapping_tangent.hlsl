Texture2D diffuse : register(t0);
Texture2D normal : register(t1);

SamplerState samp : register(s0);

struct Input {
    float2 uv : UV;
    float3 lightPosTangent : LIGHT_POS;
};

float4 main(Input input) : SV_Target {
    float3 texelNormal = normalize(2.0f * normal.Sample(samp, input.uv).rgb - 1.0f);
    float3 finalColour = diffuse.Sample(samp, input.uv).rgb * max(dot(texelNormal, input.lightPosTangent), 0.0f);

    return float4(finalColour, 0.0f);
}