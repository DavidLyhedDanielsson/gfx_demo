cbuffer Transform : register(b0) { matrix transform; }
cbuffer Transform : register(b1) { matrix viewProjection; }

struct Input {
    float2 position : POSITION;
    float2 uv : UV;
};

struct Output {
    float2 uv : UV;
    // Must be last or the UV slot will be mismatched in the pixel shader
    float4 finalPosition : SV_POSITION;
};

Output main(Input input) {
    Output output;
    output.uv = input.uv;

    output.finalPosition = mul(float4(input.position, 0.0f, 1.0f), mul(transform, viewProjection));

    return output;
}
