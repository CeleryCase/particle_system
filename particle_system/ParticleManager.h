//***************************************************************************************
// ParticleRender.h by X_Jun(MKXJun) (C) 2018-2022 All Rights Reserved.
// Licensed under the MIT License.
//
// 粒子渲染类
// Particle Render class.
//***************************************************************************************

#ifndef PARTICLE_MANAGER_H
#define PARTICLE_MANAGER_H

#include <utility>
#include "Effects.h"
#include "Camera.h"
#include "Texture2D.h"

class ParticleManager
{
public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    ParticleManager() = default;
    ~ParticleManager() = default;
    // 不允许拷贝，允许移动
    ParticleManager(const ParticleManager&) = delete;
    ParticleManager& operator=(const ParticleManager&) = delete;
    ParticleManager(ParticleManager&&) = default;
    ParticleManager& operator=(ParticleManager&&) = default;

    // 自从该系统被重置以来所经过的时间
    float GetAge() const;

    void SetEmitPos(const DirectX::XMFLOAT3& emitPos);
    void SetEmitDir(const DirectX::XMFLOAT3& emitDir);

    void SetEmitInterval(float t);
    void SetAliveTime(float t);
    void SetAcceleration(const DirectX::XMFLOAT3& accel);
    void SetParticleCount(uint32_t const defaultParticle, uint32_t const smokeParticle);

    void InitResource(ID3D11Device* device, uint32_t maxParticles);
    void SetTextureInput(ID3D11ShaderResourceView* textureInput);
    void SetTextureRandom(ID3D11ShaderResourceView* randomTexSRV);
    void SetTextureAsh(ID3D11ShaderResourceView* textureAsh);

    void Reset();
    void Update(float dt, float gameTime);
    void Draw(ID3D11DeviceContext* deviceContext, ParticleEffect& effect);
    void DrawWithSmoke(ID3D11DeviceContext* deviceContext, ParticleEffect& effect);

    std::pair<uint32_t, uint32_t> GetParticleCount(void);

    void SetBgColor(DirectX::XMFLOAT4 color);

    void SetDebugObjectName(const std::string& name);

    std::unique_ptr<Texture2D> pDefaultParticleTexture;                           // 默认粒子渲染结果缓冲区
    std::unique_ptr<Texture2D> pSmokeParticleTexture;                             // 烟雾粒子渲染结果缓冲区

    ID3D11RenderTargetView *pCurrBackBuffer = nullptr;
private:
    
    uint32_t m_MaxParticles = 0;
    int m_FirstRun = 1;

    float m_GameTime = 0.0f;
    float m_TimeStep = 0.0f;
    float m_Age = 0.0f;

    DirectX::XMFLOAT3 m_EmitPos = {};
    DirectX::XMFLOAT3 m_EmitDir = {};
    DirectX::XMFLOAT3 m_Accel = {};

    float m_EmitInterval = 0.0f;
    float m_AliveTime = 0.0f;

    uint32_t m_DefaultParticleCount = 0;
    uint32_t m_SmokeParticleCount = 0;

    DirectX::XMFLOAT4 m_BgColor = {0.0f, 0.0f, 0.0f, 1.0f};

    ComPtr<ID3D11Buffer> m_pInitVB;
    ComPtr<ID3D11Buffer> m_pDrawVB;
    ComPtr<ID3D11Buffer> m_pStreamOutVB;
    ComPtr<ID3D11Buffer> m_pFullScreenVB;
    ComPtr<ID3D11Buffer> m_pIndexBuffer;
    ComPtr<ID3D11Buffer> m_pStagingBuffer;
    ComPtr<ID3D11Query> pQuery;

    ComPtr<ID3D11ShaderResourceView> m_pTextureInputSRV;
    ComPtr<ID3D11ShaderResourceView> m_pTextureRanfomSRV;
    ComPtr<ID3D11ShaderResourceView> m_pTextureAshSRV;

};

#endif
