#ifndef BOOM_HLSL
#define BOOM_HLSL

#include "Particle.hlsl"

static const float2 g_TexCoord[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };

// 绘制输出
struct VertexOut
{
    float3 posW : POSITION;
    float2 sizeW : SIZE;
    float4 color : COLOR;
    uint type : TYPE;
};

struct GeoOut
{
    float4 posH : SV_position;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
    uint type : TYPE;
};

VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.age;

    [flatten]
    if (vIn.type == PT_SHELL) {
        t = min(vIn.age, g_EmitInterval);
    }
    
    // 恒定加速度等式
    vOut.posW = 0.5f * t * t * vIn.accelW * g_AccelW + t * vIn.initialVelW + vIn.initialPosW;
    
    // 颜色随着时间褪去
    float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0 * 2.0f);
    vOut.color = float4(1.0f, 1.0f, 1.0f, opacity);
    
    vOut.sizeW = vIn.sizeW;
    vOut.type = vIn.type;
    
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
        float halfWidth = 0.5f * gIn[0].sizeW.x;
        float halfHeight = 0.5f * gIn[0].sizeW.y;
        
        float4 v[4];
        v[0] = float4(gIn[0].posW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(gIn[0].posW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(gIn[0].posW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(gIn[0].posW - halfWidth * right + halfHeight * up, 1.0f);
    
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
            gOut.color = gIn[0].color;
            gOut.type = gIn[0].type;
            output.Append(gOut);
        }
    }
}

float4 PS(GeoOut pIn) : SV_Target
{
    if (pIn.type == PT_SHELL)
    {
        return g_TextureAsh.Sample(g_SamLinear, pIn.tex) * pIn.color;
        // return (0.0f, 0.0f, 0.0f, 0.0f);
    }
    else
    {
        return g_TextureInput.Sample(g_SamLinear, pIn.tex) * pIn.color;
    }
}

VertexParticle SO_VS(VertexParticle vIn)
{
    return vIn;
}

[maxvertexcount(32)]
void SO_GS(point VertexParticle gIn[1], inout PointStream<VertexParticle> output)
{
    gIn[0].age += g_TimeStep;
    
    if (gIn[0].type == PT_EMITTER)
    {
        // if (gIn[0].age > g_EmitInterval * 0.01) 
        {
            for (int i = 0; i < 8; ++i) {
                float3 vRandom = RandVec3((float)i / 16) * 50;
                VertexParticle p;
                p.initialPosW = gIn[0].initialPosW;
                p.initialVelW = float3(0.0f, 0.0f, 0.0f);
                p.accelW = vRandom;
                // p.initialVelW = vRandom;
                // p.accelW = float3(0.0f, 0.0f, 0.0f);
                p.sizeW = float2(2.5f, 2.5f);
                // p.age = 0.0f;
                p.age = RandVec3(0.0f).x * g_EmitInterval / 1;
                p.type = PT_SHELL;
                p.emitCount = 0;
            
                output.Append(p);
                gIn[0].emitCount++;
            }

            if (gIn[0].emitCount < 32) {
                output.Append(gIn[0]);
            }
        } 
        // else 
        // {
        //     output.Append(gIn[0]);
        // }

    }
    else if (gIn[0].type == PT_SHELL) 
    {
        if (gIn[0].age > g_EmitInterval) 
        // if (1)
        {
            for (int i = 0; i < 16; ++i) {
                float3 vRandom = RandVec3((float)i / 16) * 25;
                // vRandom.xy *= 0.5;
                float t = g_EmitInterval;
                float3 posW = 0.5f * t * t * gIn[0].accelW * g_AccelW + t * gIn[0].initialVelW + gIn[0].initialPosW;
                VertexParticle p;
                // p.initialPosW = gIn[0].initialPosW;
                p.initialPosW = posW;
                p.initialVelW = float3(0.0f, 0.0f, 0.0f);
                p.accelW = vRandom;
                // p.initialVelW = vRandom;
                // p.accelW = float3(0.0f, 0.0f, 0.0f);
                p.sizeW = float2(2.5f, 2.5f);
                p.age = 0.0f;
                p.type = PT_PARTICLE;
                p.emitCount = 0;
            
                output.Append(p);

                gIn[0].emitCount++;
            }

            if (gIn[0].emitCount <= 128) {
                output.Append(gIn[0]);
            }
        }
        else
        {
            output.Append(gIn[0]);
        }
        
        // if (gIn[0].age <= g_AliveTime)
        //     output.Append(gIn[0]);
    }
    else
    {
        // 用于限制粒子数目产生的特定条件，对于不同的粒子系统限制也有所变化
        if (gIn[0].age <= g_AliveTime)
            output.Append(gIn[0]);
    }

}

#endif

