cbuffer Transform : register(b0) { matrix transform; }
cbuffer Transform : register(b1) { matrix viewProjection; }

struct Input {
    float3 position : POSITION;
    float2 uv : UV;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

struct Output {
    float2 uv : UV;
    float3 lightPosTangent : LIGHT_POS;
    // Must be last or the UV slot will be mismatched in the pixel shader
    float4 finalPosition : SV_POSITION;
};

Output main(Input input) {
    const static float3 lightPos = normalize(float3(0.0f, 0.0f, 1.0f));

    Output output;
    output.uv = input.uv;

    float3 normalWorld =    mul(float4(input.normal, 0.0f), transform).xyz;
    float3 tangentWorld =   mul(float4(input.tangent, 0.0f), transform).xyz;
    // Gram–Schmidt process to make sure the vector really is orthogonal, optional but correct step
    tangentWorld =          normalize(tangentWorld - dot(tangentWorld, normalWorld) * normalWorld);
    float3 bitangentWorld = cross(normalWorld, tangentWorld);
    float3x3 tbnMatrix = transpose(float3x3(tangentWorld, bitangentWorld, normalWorld));

    output.lightPosTangent = mul(lightPos, tbnMatrix);
    output.finalPosition = mul(float4(input.position, 1.0f), mul(transform, viewProjection));

    return output;
}
