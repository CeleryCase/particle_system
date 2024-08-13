#ifndef FIRE_SMOKE_HLSL
#define FIRE_SMOKE_HLSL

#include "Particle.hlsl"
static const float2 g_TexCoord[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };
// 绘制输出
struct VertexOut
{
    float3 posW : POSITION;
    float2 sizeW : SIZE;
    float4 color : COLOR;
    uint type : TYPE;
    float age : AGE;
};

struct GeoOut
{
    float4 posH : SV_position;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
    uint type : TYPE;
};

struct VertexOutSV
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD;
};

VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.age;
    
    float opacity;
    // 恒定加速度等式
    if (vIn.type == PT_PARTICLE) {
        vOut.posW = 0.5f * t * t * g_AccelW + t * vIn.initialVelW + vIn.initialPosW;
        opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0f);
    } else if (vIn.type == PT_SMOKE) {
        vOut.posW = 0.5f * t * t * g_AccelW / 5 + t * vIn.initialVelW + vIn.initialPosW;
        // opacity = 1.0f - smoothstep(0.0f, 2.0f, t / 1.0f);
        opacity = max(0.6f - smoothstep(0.0f, 20.0f, t), 0.0f);
    }
    
    // 颜色随着时间褪去
    vOut.color = float4(1.0f, 1.0f, 1.0f, opacity);
    
    vOut.sizeW = vIn.sizeW;
    vOut.type = vIn.type;
    vOut.age = vIn.age;
    
    return vOut;
}

VertexOutSV BackBuffer_VS(VertexParticle vIn)
{
    VertexOutSV vOut;
    
    vOut.position = float4(vIn.initialPosW, 1.0f);
    // vOut.position = mul(vOut.position, g_ViewProj);
    vOut.tex = vIn.sizeW;
    
    return vOut;
}

[maxvertexcount(4)]
void GS(point VertexOut gIn[1], inout TriangleStream<GeoOut> output)
{
    // 不要绘制用于产生粒子的顶点
    if (gIn[0].type != PT_EMITTER)
    {
        //
        // 计算该粒子的世界矩阵让公告板朝向摄像机
        //
        float3 look = normalize(g_EyePosW.xyz - gIn[0].posW);
        float3 right = normalize(cross(float3(0.0f, 1.0f, 0.0f), look));
        float3 up = cross(look, right);
        
        //
        // 计算出处于世界空间的四边形
        //
        float halfWidth;
        float halfHeight;
        if (gIn[0].type == PT_PARTICLE) {
            halfWidth = 0.5f * gIn[0].sizeW.x - gIn[0].age * 0.2f;
            halfHeight = 0.5f * gIn[0].sizeW.y - gIn[0].age * 0.2f;
        } else {
            // 圆形
            halfWidth = 0.5f * gIn[0].age / 2 + 1.0f;
            halfHeight = 0.5f * gIn[0].age / 2 + 1.0f;
            // 火焰形
            // halfWidth = gIn[0].age + 1.0f;
            // halfHeight = gIn[0].age + 1.0f;
        }
        
        float4 v[4];
        v[0] = float4(gIn[0].posW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(gIn[0].posW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(gIn[0].posW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(gIn[0].posW - halfWidth * right + halfHeight * up, 1.0f);
    
        // 旋转矩阵
        float cosAngle = cos(gIn[0].age / 1);
        float sinAngle = sin(gIn[0].age / 1);

        //
        // 将四边形顶点从世界空间变换到齐次裁减空间
        //
        GeoOut gOut;
        [unroll]
        for (int i = 0; i < 4; ++i)
        {
            gOut.posH = mul(v[i], g_ViewProj);
            // gOut.tex = float2((float) (i % 2), 1.0f - (i / 2));
            gOut.tex = g_TexCoord[i];

            gOut.tex -= float2(0.5f, 0.5f);

            float2 rotatedTexcoord;
            rotatedTexcoord.x = cosAngle * gOut.tex.x - sinAngle * gOut.tex.y;
            rotatedTexcoord.y = sinAngle * gOut.tex.x + cosAngle * gOut.tex.y;
            gOut.tex = rotatedTexcoord;

            gOut.tex += float2(0.5f, 0.5f);

            gOut.color = gIn[0].color;
            gOut.type = gIn[0].type;
            output.Append(gOut);
        }
    }
}

[maxvertexcount(4)]
void BackBuffer_GS(triangle VertexOut gIn[3], inout PointStream<GeoOut> output)
{
        GeoOut gOut;
        [unroll]
        for (int i = 0; i < 3; ++i)
        {
            gOut.posH = float4(gIn[i].posW, 1.0f);
            gOut.tex = gIn[i].sizeW;

            gOut.color = gIn[i].color;
            gOut.type = gIn[i].type;
            output.Append(gOut);
        }
}

float4 PS(GeoOut pIn) : SV_Target
{
    if (pIn.type != PT_PARTICLE) {
        discard;
    }
    return g_TextureInput.Sample(g_SamLinearBoard, pIn.tex) * pIn.color;
}

float4 Smoke_PS(GeoOut pIn) : SV_Target
{
    if (pIn.type != PT_SMOKE) {
        discard;
    }
    // return g_TextureAsh.Sample(g_SamLinear, pIn.tex) * pIn.color;
    float4 dst_color = g_TextureAsh.Sample(g_SamLinearBoard, pIn.tex) * pIn.color;
    if (dst_color.r <= 0.05f || dst_color.g <= 0.05f || dst_color.b <= 0.05f) {
        discard;
    } 
    return dst_color;
}

float4 BackBuffer_PS(VertexOutSV pIn) : SV_Target
{
    float4 defaultParticleColor = g_TextureDefaultParticle.Sample(g_SamLinear, pIn.tex);
    float4 smokeParticleColor = g_TextureSmokeParticle.Sample(g_SamLinear, pIn.tex);

    if (defaultParticleColor.r >= 0.1f && defaultParticleColor.g >= 0.1f && defaultParticleColor.b >= 0.1f) {
        return defaultParticleColor;
    } else {
        return defaultParticleColor + smokeParticleColor * smokeParticleColor.a;
    }
    // return defaultParticleColor + smokeParticleColor * smokeParticleColor.a;
    // return defaultParticleColor;
}

VertexParticle SO_VS(VertexParticle vIn)
{
    return vIn;
}

[maxvertexcount(2)]
void SO_GS(point VertexParticle gIn[1], inout PointStream<VertexParticle> output, uint primitiveID : SV_PrimitiveID)
{
    gIn[0].age += g_TimeStep;
    
    if (gIn[0].type == PT_EMITTER)
    {
        // 是否到时间发射新的粒子
        if (gIn[0].age > g_EmitInterval)
        {
            float3 vRandom = RandUnitVec3(0.0f);
            vRandom.x *= 0.5f;
            vRandom.z *= 0.5f;
            
            VertexParticle p;
            p.initialPosW = g_EmitPosW.xyz;
            p.initialVelW = 4.0f * vRandom;
            p.accelW = float3(0.0f, 0.0f, 0.0f);
            p.sizeW = float2(3.0f, 3.0f);
            p.age = 0.0f;
            p.type = PT_PARTICLE;
            p.emitCount = 0;
            
            output.Append(p);
            
            // 重置时间准备下一次发射
            gIn[0].age = 0.0f;
        }
        
        // 总是保留发射器
        output.Append(gIn[0]);
    }
    else if (gIn[0].type == PT_PARTICLE)
    {
        // 用于限制粒子数目产生的特定条件，对于不同的粒子系统限制也有所变化
        if (gIn[0].age <= g_AliveTime) {
            if (gIn[0].age >= 0.8 * g_AliveTime && gIn[0].emitCount == 0 && primitiveID % 30 == 0 && g_SmokeParticleCount <= 100) {
                gIn[0].emitCount = 1;
                float t = gIn[0].age;
                float3 vRandom = RandUnitVec3(0.0f);
                vRandom.x *= 0.5f;
                vRandom.z *= 0.5f;
                // 恒定加速度等式
                VertexParticle p;
                p.initialPosW = 0.5f * t * t * g_AccelW + t * gIn[0].initialVelW + gIn[0].initialPosW;
                p.initialVelW = vRandom;
                p.accelW = float3(0.0f, 0.0f, 0.0f);
                p.sizeW = float2(3.0f, 3.0f);
                p.age = g_AliveTime * (vRandom.x + 1.0f) * 1;
                p.type = PT_SMOKE;
                p.emitCount = 0;
                output.Append(p);
            }
            output.Append(gIn[0]);
        }
    }
    else
    {
        if (gIn[0].age <= g_AliveTime * 3.0) {
            output.Append(gIn[0]);
        }
    }
}

#endif
