#include "ParticleManager.h"
#include <Vertex.h>
#include <XUtil.h>
#include <DXTrace.h>

float ParticleManager::GetAge() const
{
    return m_Age;
}

void ParticleManager::SetEmitPos(const DirectX::XMFLOAT3& emitPos)
{
    m_EmitPos = emitPos;
}

void ParticleManager::SetEmitDir(const DirectX::XMFLOAT3& emitDir)
{
    m_EmitDir = emitDir;
}

void ParticleManager::SetEmitInterval(float t)
{
    m_EmitInterval = t;
}

void ParticleManager::SetAliveTime(float t)
{
    m_AliveTime = t;
}

void ParticleManager::SetAcceleration(const DirectX::XMFLOAT3& accel)
{
    m_Accel = accel;
}


void ParticleManager::SetParticleCount(uint32_t const defaultParticle, uint32_t const smokeParticle)
{
    m_DefaultParticleCount = defaultParticle;
    m_SmokeParticleCount = smokeParticle;
}

void ParticleManager::InitResource(ID3D11Device* device, uint32_t maxParticles)
{
    // 
    m_MaxParticles = maxParticles;

    // 创建缓冲区用于产生粒子系统
    // 初始粒子拥有类型0和存活时间0
    ParticleEffect::VertexParticle p{};
    p.age = 0.0f;
    p.type = 0;
    p.emitCount = 0;
    CD3D11_BUFFER_DESC bufferDesc(sizeof(ParticleEffect::VertexParticle),
        D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA initData{};
    initData.pSysMem = &p;
    HR(device->CreateBuffer(&bufferDesc, &initData, m_pInitVB.GetAddressOf()));

    ParticleEffect::VertexParticle fullScreenVertex[4] = {
        {DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f), 0.0f, 0, 0},
        {DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f), 0.0f, 0, 0},
        {DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f), 0.0f, 0, 0},
        {DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f), 0.0f, 0, 0},
    };
    bufferDesc.ByteWidth = sizeof(fullScreenVertex);
    initData.pSysMem = fullScreenVertex;
    HR(device->CreateBuffer(&bufferDesc, &initData, m_pFullScreenVB.GetAddressOf()));

    // 创建Ping-Pong的缓冲区用于流输出和绘制
    bufferDesc.BindFlags |= D3D11_BIND_STREAM_OUTPUT;
    bufferDesc.ByteWidth = sizeof(ParticleEffect::VertexParticle) * m_MaxParticles;
    HR(device->CreateBuffer(&bufferDesc, nullptr, m_pDrawVB.GetAddressOf()));

    HR(device->CreateBuffer(&bufferDesc, nullptr, m_pStreamOutVB.GetAddressOf()));

    DWORD indices[] = {
        0, 1, 2,
        2, 1, 3,
    };
    // 设置索引缓冲区描述
    D3D11_BUFFER_DESC ibd;
    ZeroMemory(&ibd, sizeof(ibd));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof indices;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    // 新建索引缓冲区
    initData.pSysMem = indices;
    HR(device->CreateBuffer(&ibd, &initData, m_pIndexBuffer.GetAddressOf()));

    ZeroMemory(&bufferDesc, sizeof(bufferDesc));
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.ByteWidth = sizeof(ParticleEffect::VertexParticle) * m_MaxParticles;
    bufferDesc.BindFlags = 0;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    HR(device->CreateBuffer(&bufferDesc, nullptr, &m_pStagingBuffer));

    // 使用查询对象获取顶点个数
    D3D11_QUERY_DESC queryDesc;
    queryDesc.Query = D3D11_QUERY_SO_STATISTICS;
    queryDesc.MiscFlags = 0;

    device->CreateQuery(&queryDesc, pQuery.GetAddressOf());

}

void ParticleManager::SetTextureInput(ID3D11ShaderResourceView* textureInput)
{
    m_pTextureInputSRV = textureInput;
}

void ParticleManager::SetTextureRandom(ID3D11ShaderResourceView* randomTexSRV)
{
    m_pTextureRanfomSRV = randomTexSRV;
}

void ParticleManager::SetTextureAsh(ID3D11ShaderResourceView* textureAsh) {
    m_pTextureAshSRV = textureAsh;
}

void ParticleManager::Reset()
{
    m_FirstRun = true;
    m_Age = 0.0f;
}

void ParticleManager::Update(float dt, float gameTime)
{
    m_GameTime = gameTime;
    m_TimeStep = dt;

    m_Age += dt;
}

void ParticleManager::Draw(ID3D11DeviceContext* deviceContext, ParticleEffect& effect)
{
    ID3D11RenderTargetView* pRTVs[]{pCurrBackBuffer};

    effect.SetGameTime(m_GameTime);
    effect.SetTimeStep(m_TimeStep);
    effect.SetEmitPos(m_EmitPos);
    effect.SetEmitDir(m_EmitDir);
    effect.SetAcceleration(m_Accel);
    effect.SetEmitInterval(m_EmitInterval);
    effect.SetAliveTime(m_AliveTime);
    effect.SetParticleCount(m_DefaultParticleCount, m_SmokeParticleCount);
    effect.SetTextureInput(m_pTextureInputSRV.Get());
    effect.SetTextureRandom(m_pTextureRanfomSRV.Get());
    effect.SetTextureAsh(m_pTextureAshSRV.Get());


    // ******************
    // 流输出
    //
    // 如果是第一次运行，使用初始顶点缓冲区
    // 否则，使用存有当前所有粒子的顶点缓冲区
    effect.RenderToVertexBuffer(deviceContext,
        m_FirstRun ? m_pInitVB.Get() : m_pDrawVB.Get(),
        m_pStreamOutVB.Get(),
        m_FirstRun);
    // 后续转为DrawAuto
    m_FirstRun = 0;


    // 进行顶点缓冲区的Ping-Pong交换
    m_pDrawVB.Swap(m_pStreamOutVB);

    // ******************
    // 使用流输出顶点绘制粒子
    //
    deviceContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<float*>(&m_BgColor));
    deviceContext->OMSetRenderTargets(1, pRTVs, nullptr);
    auto inputData = effect.SetRenderDefault();
    deviceContext->IASetPrimitiveTopology(inputData.topology);
    deviceContext->IASetInputLayout(inputData.pInputLayout);
    deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), &inputData.stride, &inputData.offset);
    effect.Apply(deviceContext);
    deviceContext->DrawAuto();
}

void ParticleManager::DrawWithSmoke(ID3D11DeviceContext* deviceContext, ParticleEffect& effect)
{
    ID3D11RenderTargetView* pRTVs[]{ pSmokeParticleTexture->GetRenderTarget() };

    effect.SetGameTime(m_GameTime);
    effect.SetTimeStep(m_TimeStep);
    effect.SetEmitPos(m_EmitPos);
    effect.SetEmitDir(m_EmitDir);
    effect.SetAcceleration(m_Accel);
    effect.SetEmitInterval(m_EmitInterval);
    effect.SetAliveTime(m_AliveTime);
    effect.SetParticleCount(m_DefaultParticleCount, m_SmokeParticleCount);
    effect.SetTextureInput(m_pTextureInputSRV.Get());
    effect.SetTextureRandom(m_pTextureRanfomSRV.Get());
    effect.SetTextureAsh(m_pTextureAshSRV.Get());
    effect.SetTextureDefaultParticle(nullptr);
    effect.SetTextureSmokeParticle(nullptr);


    deviceContext->Begin(pQuery.Get());

    // ******************
    // 流输出
    //
    // 如果是第一次运行，使用初始顶点缓冲区
    // 否则，使用存有当前所有粒子的顶点缓冲区
    effect.RenderToVertexBuffer(deviceContext,
        m_FirstRun ? m_pInitVB.Get() : m_pDrawVB.Get(),
        m_pStreamOutVB.Get(),
        m_FirstRun);
    // 后续转为DrawAuto
    m_FirstRun = 0;

    deviceContext->End(pQuery.Get());

    D3D11_QUERY_DATA_SO_STATISTICS soStats;
    while (deviceContext->GetData(pQuery.Get(), &soStats, sizeof(soStats), 0) != S_OK) {
        ;
    }

    uint64_t numPrimitiveWritten = soStats.NumPrimitivesWritten;


    // 进行顶点缓冲区的Ping-Pong交换
    m_pDrawVB.Swap(m_pStreamOutVB);

    deviceContext->CopyResource(m_pStagingBuffer.Get(), m_pDrawVB.Get());

    // 获取粒子个数
    D3D11_MAPPED_SUBRESOURCE mappedResoure;
    deviceContext->Map(m_pStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedResoure);

    ParticleEffect::VertexParticle *pData = (ParticleEffect::VertexParticle*)mappedResoure.pData;

// FIXME:临时使用
#define PT_EMITTER 0
#define PT_PARTICLE 1
#define PT_SHELL 2
#define PT_SMOKE 3
    uint32_t defaultParticle = 0;
    uint32_t smokeParticle = 0;
    for (int i = 0; i < numPrimitiveWritten; ++i) {
        if (pData->type == PT_PARTICLE) {
            defaultParticle++;
        } else if (pData->type == PT_SMOKE) {
            smokeParticle++;
        }
        pData++;
    }
    
    deviceContext->Unmap(m_pStagingBuffer.Get(), 0);

    SetParticleCount(defaultParticle, smokeParticle);

    // ******************
    // 使用流输出顶点绘制粒子
    //

    deviceContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<float*>(&m_BgColor));
    deviceContext->OMSetRenderTargets(1, pRTVs, nullptr);
    auto inputData = effect.SetRenderSmoke();
    deviceContext->IASetPrimitiveTopology(inputData.topology);
    deviceContext->IASetInputLayout(inputData.pInputLayout);
    deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), &inputData.stride, &inputData.offset);
    effect.Apply(deviceContext);
    deviceContext->DrawAuto();


    pRTVs[0] = pDefaultParticleTexture->GetRenderTarget();
    deviceContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<float*>(&m_BgColor));
    deviceContext->OMSetRenderTargets(1, pRTVs, nullptr);
    inputData = effect.SetRenderDefault();
    deviceContext->IASetPrimitiveTopology(inputData.topology);
    deviceContext->IASetInputLayout(inputData.pInputLayout);
    deviceContext->IASetVertexBuffers(0, 1, m_pDrawVB.GetAddressOf(), &inputData.stride, &inputData.offset);
    effect.Apply(deviceContext);
    deviceContext->DrawAuto();

    effect.SetTextureDefaultParticle(pDefaultParticleTexture->GetShaderResource());
    effect.SetTextureSmokeParticle(pSmokeParticleTexture->GetShaderResource());
    pRTVs[0] = pCurrBackBuffer;
    deviceContext->ClearRenderTargetView(pRTVs[0], reinterpret_cast<float*>(&m_BgColor));
    deviceContext->OMSetRenderTargets(1, pRTVs, nullptr);
    inputData = effect.SetRenderToBackBuffer();
    deviceContext->IASetPrimitiveTopology(inputData.topology);
    deviceContext->IASetInputLayout(inputData.pInputLayout);
    deviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    deviceContext->IASetVertexBuffers(0, 1, m_pFullScreenVB.GetAddressOf(), &inputData.stride, &inputData.offset);
    effect.Apply(deviceContext);
    deviceContext->DrawIndexed(6, 0, 0);
}

std::pair<uint32_t, uint32_t> ParticleManager::GetParticleCount(void)
{
    return {m_DefaultParticleCount, m_SmokeParticleCount};
}


void ParticleManager::SetBgColor(DirectX::XMFLOAT4 color)
{
    m_BgColor = color;
}

void ParticleManager::SetDebugObjectName(const std::string& name)
{
#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
    ::SetDebugObjectName(m_pInitVB.Get(), name + ".InitVB");
    ::SetDebugObjectName(m_pStreamOutVB.Get(), name + ".StreamVB");
    ::SetDebugObjectName(m_pDrawVB.Get(), name + ".DrawVB");
#else
    UNREFERENCED_PARAMETER(name);
#endif
}
