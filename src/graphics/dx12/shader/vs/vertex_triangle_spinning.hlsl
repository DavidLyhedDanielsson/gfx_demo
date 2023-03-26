cbuffer Transform : register(b0) { matrix transform; }

float4 main(float2 position : POSITION) : SV_POSITION {
    float2 finalPosition = mul(float4(position, 0.0f, 0.0f), transpose(transform)).xy;
    return float4(finalPosition.xy, 0.0f, 1.0f);
}
