const static float2 vertices[3] = {
    float2(0.0f, 0.5f),
    float2(-0.5f, -0.5f),
    float2(0.5f, -0.5f),
};

float4 main(uint vertexId : SV_VERTEXID) : SV_POSITION {
  float2 position = vertices[vertexId];
  return float4(position.x, position.y, 0.0f, 1.0f);
}
