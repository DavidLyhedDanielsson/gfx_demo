Texture2D texture : register(t0);
SamplerState samp : register(s0);

float4 main(float2 uv : UV) : SV_Target {
  float3 textureColor = texture.Sample(samp, uv).rgb;

  return float4(textureColor, 1.0f);
}