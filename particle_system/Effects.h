//***************************************************************************************
// Effects.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 简易特效管理框架
// Simple effect management framework.
//***************************************************************************************

#ifndef EFFECTS_H
#define EFFECTS_H

#include <memory>
#include <LightHelper.h>
#include <RenderStates.h>

#include <GameObject.h>

#include <Buffer.h>
#include <IEffect.h>
#include <Material.h>
#include <MeshData.h>
#include <LightHelper.h>

class ParticleEffect : public IEffect
{
public:
    struct VertexParticle
    {
        DirectX::XMFLOAT3 initialPos;
        DirectX::XMFLOAT3 initialVel;
        DirectX::XMFLOAT3 accel;
        DirectX::XMFLOAT2 size;
        float age;
        uint32_t type;
        uint32_t emitCount;
    };

    struct InputData
    {
        ID3D11InputLayout* pInputLayout;
        D3D11_PRIMITIVE_TOPOLOGY topology;
        uint32_t stride;
        uint32_t offset;
    };

public:
    ParticleEffect();
    virtual ~ParticleEffect() override;

    ParticleEffect(ParticleEffect&& moveFrom) noexcept;
    ParticleEffect& operator=(ParticleEffect&& moveFrom) noexcept;

    bool InitAll(ID3D11Device* device, std::wstring_view filename);
    bool InitAllWithSmoke(ID3D11Device* device, std::wstring_view filename);

    // vertexCount为0时调用drawAuto
    void RenderToVertexBuffer(
        ID3D11DeviceContext* deviceContext,
        ID3D11Buffer* input,
        ID3D11Buffer* output,
        uint32_t vertexCount = 0);  
    // 绘制粒子系统
    InputData SetRenderDefault();
    InputData SetRenderSmoke();
    InputData SetRenderToBackBuffer();

    void XM_CALLCONV SetViewMatrix(DirectX::FXMMATRIX V);
    void XM_CALLCONV SetProjMatrix(DirectX::FXMMATRIX P);

    void SetEyePos(const DirectX::XMFLOAT3& eyePos);

    void SetGameTime(float t);
    void SetTimeStep(float step);

    void SetEmitDir(const DirectX::XMFLOAT3& dir);
    void SetEmitPos(const DirectX::XMFLOAT3& pos);

    void SetEmitInterval(float t);
    void SetAliveTime(float t);

    void SetAcceleration(const DirectX::XMFLOAT3& accel);

    void SetParticleCount(uint32_t const defaultParticle, uint32_t smokeParticle);

    void SetTextureInput(ID3D11ShaderResourceView* textureInput);
    void SetTextureRandom(ID3D11ShaderResourceView* textureRandom);
    void SetTextureAsh(ID3D11ShaderResourceView* textureAsh);
    void SetTextureDefaultParticle(ID3D11ShaderResourceView* textureDefaultParticle);
    void SetTextureSmokeParticle(ID3D11ShaderResourceView* textureSmokeParticle);

    void SetRasterizerState(ID3D11RasterizerState* rasterizerState);
    void SetBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask);
    void SetDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef);

    void SetSmokeRasterizerState(ID3D11RasterizerState* rasterizerState);
    void SetSmokeBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask);
    void SetSmokeDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef);

    // 
    // IEffect
    //

    // 应用常量缓冲区和纹理资源的变更
    void Apply(ID3D11DeviceContext* deviceContext) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#endif
