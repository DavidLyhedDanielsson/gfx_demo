Texture2D albedo : register(t0);
Texture2D ambient : register(t1);
Texture2D normal : register(t2);

SamplerState samp : register(s0);

struct Input {
    float2 uv : UV;
    float3 pixelPos : PIXEL_POS;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

const static float ambientFactor = 0.7f;

const static float3 lightPos = float3(0.0f, 0.0f, -1.75f);
const static float3 viewPos = float3(0.0f, 0.0f, -3.0f);

float4 main(Input input) : SV_Target {
    float3 bitangent = cross(input.normal, input.tangent);
    float3x3 tbnMatrix = float3x3(input.tangent, bitangent, input.normal);

    float3 normalTangent = normalize(2.0f * normal.Sample(samp, input.uv).rgb - 1.0f);
    float3 normalWorld = mul(normalTangent, tbnMatrix);

    // Pixel-to-light position since that matches normal map
    float3 lightDir = normalize(lightPos - input.pixelPos);
    float3 viewDir = normalize(viewPos - input.pixelPos);

    float ambientStrength = ambientFactor * ambient.Sample(samp, input.uv).r;
    float diffuseStrength = max(dot(normalWorld, lightDir), 0.0f);
    float3 reflected = reflect(-lightDir, normalWorld); // `reflect` wants an incident ray
    float specularStrength = pow(max(dot(viewDir, reflected), 0.0f), 32.0f) * 0.1f;

    float3 albedoColour = albedo.Sample(samp, input.uv).rgb;
    return float4(albedoColour * (ambientStrength + diffuseStrength) + specularStrength, 0.0f);
}