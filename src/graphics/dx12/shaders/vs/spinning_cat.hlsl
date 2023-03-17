cbuffer Transform : register(b0) { matrix transform; }

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

  float2 finalPosition =
      mul(float4(input.position, 0.0f, 0.0f), transpose(transform)).xy;
  output.finalPosition = float4(finalPosition.xy, 0.0f, 1.0f);

  return output;
}
