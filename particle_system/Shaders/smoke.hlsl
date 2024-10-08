#ifndef FIRE_HLSL
#define FIRE_HLSL

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
};


VertexOut VS(VertexParticle vIn)
{
    VertexOut vOut;
    
    float t = vIn.age;
    
    // 恒定加速度等式
    vOut.posW = 0.5f * t * t * g_AccelW * vIn.accelW + t * vIn.initialVelW + vIn.initialPosW;
    
    // 颜色随着时间褪去
    float opacity = 1.0f - smoothstep(0.0f, 5.0f, t);
    opacity = 0.5f;
    vOut.color = float4(0.5f, 0.5f, 0.5f, opacity);
    vOut.sizeW = vIn.sizeW;
    vOut.type = vIn.type;
    vOut.age = vIn.age;
    
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
        // float halfWidth = 0.5f * gIn[0].sizeW.x;
        // float halfHeight = 0.5f * gIn[0].sizeW.y;
        float halfWidth = 0.5f * gIn[0].age / 2 + 0.1f;
        float halfHeight = 0.5f * gIn[0].age / 2 + 0.1f;
        
        float4 v[4];
        v[0] = float4(gIn[0].posW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(gIn[0].posW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(gIn[0].posW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(gIn[0].posW - halfWidth * right + halfHeight * up, 1.0f);
    

        // 旋转矩阵
        float cosAngle = cos(gIn[0].age / 3);
        float sinAngle = sin(gIn[0].age / 3);

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
            output.Append(gOut);
        }
    }
}

float4 PS(GeoOut pIn) : SV_Target
{
    return g_TextureInput.Sample(g_SamLinear, pIn.tex) * pIn.color;
    // float4 dst_color = g_TextureInput.Sample(g_SamLinear, pIn.tex) * pIn.color;
    // if (dst_color.r <= 0.11f || dst_color.g <= 0.11f || dst_color.b <= 0.11f) {
    //     // discard;
    //     dst_color.rgb = float3(1.0f, 1.0f, 1.0f);
    // } 
    // return dst_color;
    // return pIn.color;
}

VertexParticle SO_VS(VertexParticle vIn)
{
    return vIn;
}

[maxvertexcount(2)]
void SO_GS(point VertexParticle gIn[1], inout PointStream<VertexParticle> output)
{
    gIn[0].age += g_TimeStep;
    
    if (gIn[0].type == PT_EMITTER)
    {
        // 是否到时间发射新的粒子
        if (gIn[0].age > g_EmitInterval)
        {
            float3 vRandom = RandUnitVec3(0.0f);
            // vRandom.x *= 0.5f;
            // vRandom.z *= 0.5f;
            
            VertexParticle p;
            p.initialPosW = g_EmitPosW.xyz;
            p.initialVelW = float3(0.0f, 0.0f, 0.0f);
            p.accelW = vRandom;
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
    else
    {
        // 用于限制粒子数目产生的特定条件，对于不同的粒子系统限制也有所变化
        if (gIn[0].age <= g_AliveTime)
            output.Append(gIn[0]);
    }
}

#endif
