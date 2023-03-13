StructuredBuffer<float2> positions : register(t0);

float4 main(uint vertexId : SV_VERTEXID) : SV_POSITION {
  float2 position = positions[vertexId];
  return float4(position.x, position.y, 0.0f, 1.0f);
}
