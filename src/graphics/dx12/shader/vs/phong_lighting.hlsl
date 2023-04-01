cbuffer Transform : register(b0) { matrix transform; }
cbuffer Transform : register(b1) { matrix viewProjection; }

struct Input {
    float3 position : POSITION;
    float2 uv : UV;
    float3 normal : NORMAL;
};

struct Output {
    float2 uv : UV;
    float3 pixelPos : PIXEL_POS;
    float3 normal : NORMAL;
    // Must be last or the UV slot will be mismatched in the pixel shader
    float4 finalPosition : SV_POSITION;
};

Output main(Input input) {
    Output output;
    output.uv = input.uv;
    output.normal = mul(float4(input.normal, 0.0f), transform).xyz;

    float4 vertexPosition = mul(float4(input.position, 1.0f), transform);
    output.pixelPos = vertexPosition.xyz;
    output.finalPosition = mul(vertexPosition, viewProjection);

    return output;
}
