Texture2D theTextureY : register(t0);
Texture2D theTextureU : register(t1);
Texture2D theTextureV : register(t2);
SamplerState theSampler : register(s0);

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 color : COLOR0;
};

// Derived from https://msdn.microsoft.com/en-us/library/windows/desktop/dd206750(v=vs.85).aspx
// Section: Converting 8-bit YUV to RGB888
static const float3x3 YUVtoRGBCoeffMatrix =
{
	1.164383f,  1.164383f, 1.164383f,
	0.000000f, -0.391762f, 2.017232f,
	1.596027f, -0.812968f, 0.000000f
};

float3 ConvertYUVtoRGB(float3 yuv)
{
	// Derived from https://msdn.microsoft.com/en-us/library/windows/desktop/dd206750(v=vs.85).aspx
	// Section: Converting 8-bit YUV to RGB888

	// These values are calculated from (16 / 255) and (128 / 255)
	yuv -= float3(0.062745f, 0.501960f, 0.501960f);
	yuv = mul(yuv, YUVtoRGBCoeffMatrix);

	return saturate(yuv);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 yuv;
	yuv.x = theTextureY.Sample(theSampler, input.tex).r;
	yuv.y = theTextureU.Sample(theSampler, input.tex).r;
	yuv.z = theTextureV.Sample(theSampler, input.tex).r;

	return float4(ConvertYUVtoRGB(yuv), 1.f);
}