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
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    // Must be last or the UV slot will be mismatched in the pixel shader
    float4 finalPosition : SV_POSITION;
};

Output main(Input input) {
    Output output;
    output.uv = input.uv;
    output.normal =    normalize(mul(float4(input.normal, 0.0f), transform).xyz);
    output.tangent =   normalize(mul(float4(input.tangent, 0.0f), transform).xyz);

    output.finalPosition = mul(float4(input.position, 1.0f), mul(transform, viewProjection));

    return output;
}
