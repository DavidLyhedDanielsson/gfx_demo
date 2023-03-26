float4 main(float2 position : POSITION) : SV_POSITION {
    return float4(position.x, position.y, 0.0f, 1.0f);
}
