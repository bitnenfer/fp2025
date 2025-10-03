#include "ParticleConfig.h"

RWStructuredBuffer<float2> outputFrame : register(u0);
ConstantBuffer<AudioData> audioData : register(b0);

//------------------------------------------------------------------------------
// HLSL port of the provided Shadertoy shader
//------------------------------------------------------------------------------

static const float WARMUP_TIME = 2.0;
static const float loopSpeed = 0.1;
static const float loopTime = 5.0;
static const float impactTime = 1.0;
static const float impactFade = 0.3;
static const float fadeOutTime = 0.1;
static const float fadeInTime = 0.1;
static const float whiteTime = 0.3; // fade to white

// shims (GLSL -> HLSL)
float smoothstepf(float a, float b, float x)
{
    float t = saturate((x - a) / (b - a));
    return t * t * (3.0 - 2.0 * t);
}
float2 smoothstepf2(float a, float b, float2 x)
{
    float2 t = saturate((x - a) / (b - a));
    return t * t * (3.0 - 2.0 * t);
}
float fract(float x)
{
    return frac(x);
}
float2 fract(float2 v)
{
    return frac(v);
}
float3 fract(float3 v)
{
    return frac(v);
}
// (HLSL already has lerp, clamp, sin, cos, exp, pow, dot, etc.)

//------------------------------------------------------------------------------
// Random / hash utilities (Dave_Hoskins variants)
//------------------------------------------------------------------------------
static const float2 add = float2(1.0, 0.0);
static const float2 MOD2 = float2(0.16632, 0.17369);
static const float3 MOD3 = float3(0.16532, 0.17369, 0.15787);

float rand(float2 co)
{
    return fract(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// 1 out, 1 in
float hash11(float p)
{
    float2 p2 = fract(float2(p, p) * MOD2);
    p2 += dot(p2.yx, p2.xy + 19.19);
    return fract(p2.x * p2.y);
}

// 2 out, 1 in
float2 hash21(float p)
{
    float3 p3 = fract(float3(p, p, p) * MOD3);
    p3 += dot(p3.xyz, p3.yzx + 19.19);
    return fract(float2(p3.x * p3.y, p3.z * p3.x)) - 0.5;
}

// 2 out, 2 in
float2 hash22(float2 p)
{
    float3 p3 = fract(float3(p.x, p.y, p.x) * MOD3);
    p3 += dot(p3.zxy, p3.yxz + 19.19);
    return fract(float2(p3.x * p3.y, p3.z * p3.x));
}

// Noise
float2 Noise21(float x)
{
    float p = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(hash21(p), hash21(p + 1.0), f) - 0.5;
}

float Noise11(float x)
{
    float p = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(hash11(p), hash11(p + 1.0), f) - 0.5;
}

float2 Noise22(float2 x)
{
    float2 p = floor(x);
    float2 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);

    float2 res = lerp(lerp(hash22(p), hash22(p + add.xy), f.x),
                       lerp(hash22(p + add.yx), hash22(p + add.xx), f.x), f.y);
    return res - 0.5;
}

// FBM
float2 FBM21(float v)
{
    float2 r = 0.0.xx;
    float2 x = float2(v, v * 1.3 + 23.333);
    float a = 0.6;
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        r += Noise22(x * a) / a;
        a += a;
    }
    return r;
}

float2 FBM22(float2 x)
{
    float2 r = 0.0.xx;
    float a = 0.6;
    [unroll]
    for (int i = 0; i < 8; ++i)
    {
        r += Noise22(x * a) / a;
        a += a;
    }
    return r;
}

//------------------------------------------------------------------------------
// Musical helpers
//------------------------------------------------------------------------------
static const float PI = 3.1415;
static const float TWOPI = 6.2832;

float n2f(float note) // note number -> frequency (A=55 * 2^((n-3)/12))
{
    return 55.0 * pow(2.0, (note - 3.0) / 12.0);
}

float square(float time, float freq)
{
    return sin(time * TWOPI * freq) > 0.0 ? 1.0 : -1.0;
}

float sine(float time, float freq)
{
    return sin(time * TWOPI * freq);
}

float2 sineLoop(float time, float freq, float rhythm)
{
    float2 sig = 0.0.xx;
    float loop = frac(time * rhythm);

    float modFreq = freq + sine(time, 7.0);

    sig += float2(sine(time, freq) * exp(-3.0 * loop), 0.0);

    float panfreq = rhythm * 0.3;
    float panSin = sine(time, panfreq);
    float pan = (panSin + 1.0) * 0.5;
    sig.x *= pan;
    sig.y *= (1.0 - pan);

    return sig;
}

float2 chimeTrack(float time)
{
    float2 sig = 0.0.xx;
    sig += sineLoop(time, 1730.0, 0.44) * 0.2;
    sig += sineLoop(time, 880.0, 1.00);
    sig += sineLoop(time, 990.0, 0.30) * 0.4;
    sig += sineLoop(time, 330.0, 0.40);
    sig += sineLoop(time, 110.0, 0.10);
    sig += sineLoop(time, 60.0, 0.05);
    sig /= 6.0;
    return sig;
}

float tone(float freq, float time)
{
    return sin(6.2831 * freq * time);
}

float chord(float base, float time)
{
    float f = 0.0;
    f += tone(base, time) * (1.0 + 0.1 * sin(time * 5.2));
    f += tone(base, time * 1.26) * 0.6 * (1.0 + 0.3 * sin(time * 10.02));
    f += tone(base, time * 1.498) * 0.4 * (1.0 + 0.3 * sin(time * 23.4));
    f += tone(base, time * 2.0 * 1.498) * 0.2 * (1.0 + 0.3 * sin(time * 43.4));
    f += tone(base, time * 2.0 * 1.26) * 0.2 * (1.0 + 0.3 * sin(time * 10.4));
    return f / 2.0;
}

float2 bass(float time, float tt, float note)
{
    if (tt < 0.0)
        return 0.0.xx;

    float freqTime = 6.2831 * time * n2f(note);

    float s =
        (sin(freqTime + sin(freqTime) * 7.0 * exp(-2.0 * tt))
         + sin(freqTime * 2.0
              + cos(freqTime * 2.0) * 1.0 * sin(time * 3.14)
              + sin(freqTime * 8.0) * 0.25 * sin(1.0 + time * 3.14)) * exp(-2.0 * tt)
         + cos(freqTime * 4.0
              + cos(freqTime * 2.0) * 3.0 * sin(time * 3.14 + 0.3)) * exp(-2.0 * tt)
        )
        * exp(-1.0 * tt);

    return float2(s, s);
}

//------------------------------------------------------------------------------
// mainSound(s, time) — returns stereo sample
//------------------------------------------------------------------------------
float2 mainSound(int samp, float time)
{
    time = max(0.0, time - WARMUP_TIME);

    float tInput = time;
    float timeInLoop = loopTime - time * loopSpeed;
    float per = ((loopTime - timeInLoop) / loopTime);

    // fades
    float fadeIn = (loopTime - clamp(timeInLoop, loopTime - fadeInTime, loopTime)) / fadeInTime;
    float fadeOut = (loopTime - clamp(loopTime - timeInLoop, loopTime - fadeOutTime, loopTime)) / fadeOutTime;

    // (text/impact calculations kept for parity, though unused in final mix below)
    if (timeInLoop < impactTime + whiteTime)
    { /* noop */
    }
    if (timeInLoop < impactTime)
    {
        float textFade = pow(max(0.0, timeInLoop - (impactTime - impactFade)) / impactFade, 2.0);
        (void) textFade;
    }

    float peaceTone = 0.0;
    float cometTone = 0.0;
    float textTone = 0.0;
    float explosionTone = 0.0;
    float finalTone = 0.0;

    peaceTone = smoothstepf(0.0, 0.1, per);
    if (per > 0.6)
        peaceTone = smoothstepf(0.8, 0.6, per);

    if (per > 0.41)
    {
        cometTone = smoothstepf(0.41, 0.49, per);
        if (per > 0.49 && per < 0.75)
        {
            cometTone = 1.0 + pow((per - 0.49) / (0.75 - 0.49), 3.0);
        }
        if (per > 0.75)
        {
            cometTone = smoothstepf(0.84, 0.75, per) * 2.0;
        }
    }

    explosionTone = pow(cometTone, 5.0);
    textTone = smoothstepf(0.76, 0.90, per);
    finalTone = pow(smoothstepf(0.93, 0.96, per), 2.0) / max(1.0, exp((per - 0.96) * 80.0));

    float2 noiseT = float2(rand(float2(time, 20.51)), rand(float2(time * 4.0, 2.51)));
    float2 noiseFBM = FBM22(time * (Noise21(time * 0.4) + 900.0)) * abs(Noise21(time * 1.5));

    float2 comet = float2(chord(55.0, time), chord(55.0, time)) * cometTone;
    float2 peace = chimeTrack(time) * peaceTone;
    float2 text = chimeTrack(time * 20.0) * textTone;
    float2 finalS = float2(chord(110.0, time), chord(110.0, time)) * 3.0 * finalTone;
    float2 explosion = (noiseT + noiseFBM * 5.0) * 0.03 * explosionTone;

    float2 b = bass(time, 1.7, 15.0) + bass(time, 1.7, 10.0);
    b *= textTone * 3.0;

    float gain = min(fadeOut, fadeIn);
    float2 mixSig = b + (comet + peace + text + explosion + finalS) * 0.4;

    // NOTE: your original returned only a basic sine; keeping that for parity:
    // return float2(sine(time, 10.0), sine(time, 10.0));

    // If you actually want the composed signal, uncomment this:
    // return saturate(gain * mixSig); // or clamp to [-0.8, 0.8] as in comment
    float2 a = a = min(fadeOut, fadeIn) * (b + (comet + peace + text + explosion + finalS) * .4);
    return clamp(a, -.8, .8);
}


float2 makeSound(uint s, float t)
{
    return mainSound(s, t); //clamp(sine(t, 10.0), -0.8, 0.8);

}

[numthreads(256, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint i = DTid.x;
    float t = float(i) / float(audioData.sampleRate);
    float2 s = makeSound(i, t);
    outputFrame[i] = s;

}