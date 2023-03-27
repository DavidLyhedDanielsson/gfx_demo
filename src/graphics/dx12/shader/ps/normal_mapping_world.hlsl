Texture2D diffuse : register(t0);
Texture2D normal : register(t1);

SamplerState samp : register(s0);

struct Input {
    float2 uv : UV;
    float3 normalWorld : NORMAL;
    float3 tangentWorld : TANGENT;
};

float4 main(Input input) : SV_Target {
    const static float3 lightPos = normalize(float3(0.0f, 0.0f, 1.0f));

    float3 bitangentWorld = cross(input.normalWorld, input.tangentWorld);
    float3x3 tbnMatrix = float3x3(input.tangentWorld, bitangentWorld, input.normalWorld);

    float3 texelNormal = normalize(2.0f * normal.Sample(samp, input.uv).rgb - 1.0f);
    float3 mappedNormal = mul(texelNormal, tbnMatrix);

    float3 textureColor = diffuse.Sample(samp, input.uv).rgb * max(dot(mappedNormal, lightPos), 0.0f);

    return float4(textureColor, 0.0f);
}