Texture2D<float4> CurrentFrame : register(t0);
Texture2D<float4> VelocityBuffer : register(t1);
Texture2D<float4> HistoryBuffer : register(t2);
RWTexture2D<float4> ResultTexture : register(u0);
const float3 Resolution : register(b0);

#define USE_NEIGHBORHOOD_CLAMPING 1
#define USE_NEIGHBORHOOD_CLIPPING 1
#define USE_PLAYDEAD_BLENDING 1
#define USE_COLORSPACE_YCOCG 1
#define FEEDBACK_FACTOR_MIN 0.88
#define FEEDBACK_FACTOR_MAX 0.97
#define USE_LERP_BLENDING 1


/* From Inside's Temporal Reprojection Anti-Aliasing Talk. */
/* Slides: https://www.gdcvault.com/play/1022970/Temporal-Reprojection-Anti-Aliasing-in */
/* Video: https://www.gdcvault.com/play/1023254/Temporal-Reprojection-Anti-Aliasing-in */
float4 ClipAABB(float3 AABBMin, float3 AABBMax, float4 Current, float4 History)
{
    float3 PClip = 0.5 * (AABBMax + AABBMin);
    float3 EClip = 0.5 * (AABBMax - AABBMin);
    float4 VClip = History - float4(PClip, Current.w);
    float3 VUnit = VClip.xyz / EClip;
    float3 AUnit = abs(VUnit);
    float MaUnit = max(AUnit.x, max(AUnit.y, AUnit.z));
    if (MaUnit > 1.0)
        return float4(PClip, Current.w) + VClip / MaUnit;
    else
        return History;
}

/* Based of https://www.microsoft.com/en-us/research/wp-content/uploads/2016/06/Malvar_Sullivan_YCoCg-R_JVT-I014r3-2.pdf */
float4 RGBAToYCoCg(float4 RGBA)
{
#if USE_COLORSPACE_YCOCG
    return float4(mul(float3x3(
		0.25, 0.5, -0.25,
		0.5, 0.0, 0.5,
		0.25, -0.5, -0.25
		), RGBA.rgb), RGBA.a);
#else
	return RGBA;
#endif
}

/* Based of https://www.microsoft.com/en-us/research/wp-content/uploads/2016/06/Malvar_Sullivan_YCoCg-R_JVT-I014r3-2.pdf */
float4 YCoCgToRGBA(float4 YCoCg)
{
#if USE_COLORSPACE_YCOCG
    return float4(mul(float3x3(
		1, 1, 1,
		1, 0, -1,
		-1, 1, -1
		), YCoCg.xyz), YCoCg.w);
#else
	return YCoCg;
#endif
}

float4 GetCurrentFrame(uint2 uv)
{
    return CurrentFrame[uv];
}

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float4 result = 0;
    float GlobalBlendFactor = 0.8;
    float2 currUv = float2(DTid.xy) / (Resolution).xy;
    float2 velocity = VelocityBuffer[DTid.xy].xy;
    float2 prevUv = currUv + velocity; //(currUv - velocity) * (Resolution).xy;
    float currDepth = CurrentFrame[DTid.xy].w;
    float prevDepth = HistoryBuffer[DTid.xy].w;
    float4 currentColor = CurrentFrame[DTid.xy];
    float4 historyColor = HistoryBuffer[prevUv];
    currentColor = RGBAToYCoCg(currentColor);
    historyColor = RGBAToYCoCg(historyColor);

    float4 neighborBoxTL = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(-1, -1))));
    float4 neighborBoxTC = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+0, -1))));
    float4 neighborBoxTR = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+1, -1))));
    float4 neighborBoxCL = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(-1, +0))));
    float4 neighborBoxCR = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+1, +0))));
    float4 neighborBoxBL = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(-1, +1))));
    float4 neighborBoxBC = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+0, +1))));
    float4 neighborBoxBR = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+1, +1))));

    float4 neighborCrossTC = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+0, -1))));
    float4 neighborCrossCL = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(-1, +0))));
    float4 neighborCrossCR = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+1, +0))));
    float4 neighborCrossBC = RGBAToYCoCg(GetCurrentFrame(uint2(int2(DTid.xy) + int2(+0, +1))));

    float4 neighborBoxMin = min(currentColor, min(neighborBoxTL, min(neighborBoxTC, min(neighborBoxTR, min(neighborBoxCL, min(neighborBoxCR, min(neighborBoxBL, min(neighborBoxBC, neighborBoxBR))))))));
    float4 neighborBoxMax = max(currentColor, max(neighborBoxTL, max(neighborBoxTC, max(neighborBoxTR, max(neighborBoxCL, max(neighborBoxCR, max(neighborBoxBL, max(neighborBoxBC, neighborBoxBR))))))));

    float4 neighborPlusMin = min(currentColor, min(neighborCrossTC, min(neighborCrossCL, min(neighborCrossCR, neighborCrossBC))));
    float4 neighborPlusMax = max(currentColor, max(neighborCrossTC, max(neighborCrossCL, max(neighborCrossCR, neighborCrossBC))));

    float4 neighborMin = lerp(neighborBoxMin, neighborPlusMin, 0.5);
    float4 neighborMax = lerp(neighborBoxMax, neighborPlusMax, 0.5);
    
#if USE_NEIGHBORHOOD_CLAMPING
	/* Neighborhood Clamping */
	historyColor = clamp(historyColor, neighborMin, neighborMax);
#elif USE_NEIGHBORHOOD_CLIPPING
	/* Neighborhood Clipping (From UE4 TemporaAA talk and Inside Talk) */
    historyColor = ClipAABB(neighborMin.rgb, neighborMax.rgb, currentColor, historyColor);
#endif

#if USE_LERP_BLENDING
#if USE_PLAYDEAD_BLENDING
#if USE_COLORSPACE_YCOCG
    float lumCurrent = currentColor.x;
    float lumHistory = historyColor.x;
#else
	float lumCurrent = dot(float3(0.2126, 0.7152, 0.0722), currentColor.rgb);
	float lumHistory = dot(float3(0.2126, 0.7152, 0.0722), historyColor.rgb);
#endif
    float unbiasedDiff = abs(lumCurrent - lumHistory) / max(lumCurrent, max(lumHistory, 1.2));
    float unbiasedWeight = 1.0 - unbiasedDiff;
    float unbiasedWeightSqr = unbiasedWeight * unbiasedWeight;
    float blendFactor = lerp(FEEDBACK_FACTOR_MIN, 1 - GlobalBlendFactor, unbiasedWeightSqr);
#else
	float blendFactor = 1 - GlobalBlendFactor;
#endif
    result = YCoCgToRGBA(lerp(currentColor, historyColor, blendFactor));
#else
	float blendFactor = GlobalBlendFactor;
	result = YCoCgToRGBA(blendFactor * currentColor + (1 - blendFactor) * historyColor);
#endif
    result.w = currDepth;

    ResultTexture[DTid.xy] = (result);
}