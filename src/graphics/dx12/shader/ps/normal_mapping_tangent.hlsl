Texture2D albedo : register(t0);
Texture2D normal : register(t1);

SamplerState samp : register(s0);

struct Input {
    float2 uv : UV;
    float3 pixelPosTangent : PIXEL_POS;
    float3 lightPosTangent : LIGHT_POS;
    float3 viewPosTangent : VIEW_POS;
};

const static float ambientStrength = 0.7f;

float4 main(Input input) : SV_Target {
    float3 normalTangent = normalize(2.0f * normal.Sample(samp, input.uv).rgb - 1.0f);

    // Pixel-to-light position since that matches normal map
    float3 lightDir = normalize(input.lightPosTangent - input.pixelPosTangent);
    float3 viewDir = normalize(input.viewPosTangent - input.pixelPosTangent);

    float diffuseStrength = max(dot(normalTangent, lightDir), 0.0f);
    float3 reflected = reflect(-lightDir, normalTangent); // `reflect` wants an incident ray
    float specularStrength = pow(max(dot(viewDir, reflected), 0.0f), 32.0f) * 0.1f;

    float3 albedoColour = albedo.Sample(samp, input.uv).rgb;
    return float4(albedoColour * (ambientStrength + diffuseStrength) + specularStrength, 0.0f);
}