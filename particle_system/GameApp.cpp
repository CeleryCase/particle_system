#include "GameApp.h"
#include <XUtil.h>
#include <DXTrace.h>
#include <ScreenGrab11.h>
#define  _USE_MATH_DEFINES
#include <math.h>
using namespace DirectX;

#pragma warning(disable: 26812)

GameApp::GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight)
    : D3DApp(hInstance, windowName, initWidth, initHeight)
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
    if (!D3DApp::Init())
        return false;

    m_TextureManager.Init(m_pd3dDevice.Get());
    m_ModelManager.Init(m_pd3dDevice.Get());

    // 务必先初始化所有渲染状态，以供下面的特效使用
    RenderStates::InitAll(m_pd3dDevice.Get());

    if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_SkyboxEffect.InitAll(m_pd3dDevice.Get()))
        return false;

    if (!m_FireEffect.InitAll(m_pd3dDevice.Get(), L"../../particle_system/Shaders/Fire.hlsl"))
        return false;

    if (!m_RainEffect.InitAll(m_pd3dDevice.Get(), L"../../particle_system/Shaders/Rain.hlsl"))
        return false;

    if (!m_BoomEffect.InitAll(m_pd3dDevice.Get(), L"../../particle_system/Shaders/Boom.hlsl"))
        return false;

    if (!m_FountainEffect.InitAll(m_pd3dDevice.Get(), L"../../particle_system/Shaders/fountain.hlsl"))
        return false;

    if (!m_SmokeEffect.InitAll(m_pd3dDevice.Get(), L"../../particle_system/Shaders/smoke.hlsl"))
        return false;

    if (!InitResource())
        return false;

    return true;
}

void GameApp::OnResize()
{
    D3DApp::OnResize();

    m_pDepthTexture = std::make_unique<Depth2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight);
    m_pLitTexture = std::make_unique<Texture2D>(m_pd3dDevice.Get(), m_ClientWidth, m_ClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

    m_pDepthTexture->SetDebugObjectName("DepthTexture");
    m_pLitTexture->SetDebugObjectName("LitTexture");

    // 摄像机变更显示
    if (m_pCamera != nullptr)
    {
        m_pCamera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
        m_pCamera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
        m_BasicEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_SkyboxEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_FireEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_RainEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_BoomEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_FountainEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
        m_SmokeEffect.SetProjMatrix(m_pCamera->GetProjMatrixXM());
    }
}

void GameApp::UpdateScene(float dt)
{

    auto cam1st = std::dynamic_pointer_cast<FirstPersonCamera>(m_pCamera);

    ImGuiIO& io = ImGui::GetIO();
    // ******************
    // 第一人称摄像机的操作
    //
    float d1 = 0.0f, d2 = 0.0f;
    if (ImGui::IsKeyDown(ImGuiKey_W))
        d1 += dt;
    if (ImGui::IsKeyDown(ImGuiKey_S))
        d1 -= dt;
    if (ImGui::IsKeyDown(ImGuiKey_A))
        d2 -= dt;
    if (ImGui::IsKeyDown(ImGuiKey_D))
        d2 += dt;

    cam1st->Walk(d1 * 6.0f);
    cam1st->Strafe(d2 * 6.0f);

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
    {
        cam1st->Pitch(io.MouseDelta.y * 0.01f);
        cam1st->RotateY(io.MouseDelta.x * 0.01f);
    }

    if (ImGui::Begin("Particle System"))
    {
        if (ImGui::Button("Reset Particle"))
        {
            m_Fire.Reset();
            m_Rain.Reset();
            m_Boom.Reset();
            m_Fountain.Reset();
            m_Smoke.Reset();
        }        

        static int curr_particle_item = 4;
        static ParticleManager *curr_particle = &m_Smoke;
        static float alive_time = 3.0f;
        static float emit_interval = 0.0015f;
        static DirectX::XMFLOAT3 accel(0.0f, 7.8f, 0.0f);
        const char* particle_strs[] = {
            "Flare",
            "Rain",
            "Boom",
            "Fountain",
            "Smoke"
        };
        if (ImGui::Combo("Particle Type", &curr_particle_item, particle_strs, ARRAYSIZE(particle_strs)))
        {
            m_CurrParticleType = static_cast<ParticleType>(curr_particle_item);
            switch (m_CurrParticleType) {
                case ParticleType::Fire: curr_particle = &m_Fire; break;
                case ParticleType::Rain: curr_particle = &m_Rain; break;
                case ParticleType::Boom: curr_particle = &m_Boom; break;
                case ParticleType::Fountain: curr_particle = &m_Fountain; break;
                case ParticleType::Smoke: curr_particle = &m_Smoke; break;
                default: curr_particle = &m_Fountain; break;
            }
            // curr_particle->SetAliveTime(fountainAliveTime);
            // curr_particle->SetEmitInterval(fountainEmitInterval);
        }                                                                            
        if (ImGui::SliderFloat("Emit Interval", &emit_interval, 0.0f, 1.0f, "%.4f"))
        {
            curr_particle->SetEmitInterval(emit_interval);
        }

        if (ImGui::SliderFloat("Alive Time", &alive_time, 0.0f, 10.0f, "%.1f"))
        {
            curr_particle->SetAliveTime(alive_time);
        }
        if (ImGui::SliderFloat("Acceleration: x", &accel.x, -20.0f, 20.0f, "%.1f"))
        {
            curr_particle->SetAcceleration(accel);
        }
        if (ImGui::SliderFloat("Acceleration: y", &accel.y, -20.0f, 20.0f, "%.1f"))
        {
            curr_particle->SetAcceleration(accel);
        }
        if (ImGui::SliderFloat("Acceleration: z", &accel.z, -20.0f, 20.0f, "%.1f"))
        {
            curr_particle->SetAcceleration(accel);
        }
    }
    ImGui::End();
    ImGui::Render();

    // 将位置限制在[-80.0f, 80.0f]的区域内
    // 不允许穿地
    XMFLOAT3 adjustedPos;
    XMStoreFloat3(&adjustedPos, XMVectorClamp(cam1st->GetPositionXM(), XMVectorReplicate(-80.0f), XMVectorReplicate(80.0f)));
    cam1st->SetPosition(adjustedPos);

    m_SkyboxEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BasicEffect.SetEyePos(m_pCamera->GetPosition());

    // ******************
    // 粒子系统
    //
    m_Fire.Update(dt, m_Timer.TotalTime());
    m_Rain.Update(dt, m_Timer.TotalTime());
    m_Boom.Update(dt, m_Timer.TotalTime());
    m_Fountain.Update(dt, m_Timer.TotalTime());
    m_Smoke.Update(dt, m_Timer.TotalTime());

    m_FireEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_FireEffect.SetEyePos(m_pCamera->GetPosition());

    m_BoomEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_BoomEffect.SetEyePos(m_pCamera->GetPosition());

    m_FountainEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_FountainEffect.SetEyePos(m_pCamera->GetPosition());

    m_SmokeEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_SmokeEffect.SetEyePos(m_pCamera->GetPosition());

    m_RainEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_RainEffect.SetEyePos(m_pCamera->GetPosition());

    static XMFLOAT3 lastCameraPos = m_pCamera->GetPosition();
    XMFLOAT3 cameraPos = m_pCamera->GetPosition();

    XMVECTOR cameraPosVec = XMLoadFloat3(&cameraPos);
    XMVECTOR lastCameraPosVec = XMLoadFloat3(&lastCameraPos);
    XMFLOAT3 emitPos;
    XMStoreFloat3(&emitPos, cameraPosVec + 3.0f * (cameraPosVec - lastCameraPosVec));
    m_RainEffect.SetViewMatrix(m_pCamera->GetViewMatrixXM());
    m_RainEffect.SetEyePos(m_pCamera->GetPosition());
    m_Rain.SetEmitPos(emitPos);
    lastCameraPos = m_pCamera->GetPosition();
}

void GameApp::DrawScene()
{
    // 创建后备缓冲区的渲染目标视图
    if (m_FrameCount < m_BackBufferCount)
    {
        ComPtr<ID3D11Texture2D> pBackBuffer;
        m_pSwapChain->GetBuffer(0, IID_PPV_ARGS(pBackBuffer.GetAddressOf()));
        CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
        m_pd3dDevice->CreateRenderTargetView(pBackBuffer.Get(), &rtvDesc, m_pRenderTargetViews[m_FrameCount].ReleaseAndGetAddressOf());
    }

    // ******************
    // 正常绘制场景
    //
    
    float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dImmediateContext->ClearRenderTargetView(m_pLitTexture->GetRenderTarget(), black);
    m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthTexture->GetDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    ID3D11RenderTargetView* pRTVs[]{ m_pLitTexture->GetRenderTarget() };
    // ID3D11RenderTargetView* pRTVs[]{ nullptr};

    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    D3D11_VIEWPORT vp = m_pCamera->GetViewPort();
    m_pd3dImmediateContext->RSSetViewports(1, &vp);

    // m_BasicEffect.DrawInstanced(m_pd3dImmediateContext.Get(), *m_pInstancedBuffer, m_Trees, 144);
    // // 绘制地面
    // m_BasicEffect.SetRenderDefault();
    // m_Ground.Draw(m_pd3dImmediateContext.Get(), m_BasicEffect);

    // ******************
    // 绘制天空盒
    //
    pRTVs[0] = GetBackBufferRTV();
    // m_pd3dImmediateContext->RSSetViewports(1, &vp);
    // m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, nullptr);
    // m_SkyboxEffect.SetRenderDefault();
    // m_SkyboxEffect.SetDepthTexture(m_pDepthTexture->GetShaderResource());
    // m_SkyboxEffect.SetLitTexture(m_pLitTexture->GetShaderResource());
    // m_Skybox.Draw(m_pd3dImmediateContext.Get(), m_SkyboxEffect);
    // m_SkyboxEffect.SetDepthTexture(nullptr);
    // m_SkyboxEffect.SetLitTexture(nullptr);
    // m_SkyboxEffect.Apply(m_pd3dImmediateContext.Get());

    // ******************
    // 粒子系统留在最后绘制便于混合
    //
    // 只显示粒子效果
    m_pd3dImmediateContext->ClearRenderTargetView(GetBackBufferRTV(), black);

    m_pd3dImmediateContext->OMSetRenderTargets(1, pRTVs, m_pDepthTexture->GetDepthStencil());
    // m_Fire.Draw(m_pd3dImmediateContext.Get(), m_FireEffect);
    // m_Rain.Draw(m_pd3dImmediateContext.Get(), m_RainEffect);
    // m_Boom.Draw(m_pd3dImmediateContext.Get(), m_BoomEffect);
    // m_Fountain.Draw(m_pd3dImmediateContext.Get(), m_FountainEffect);

    switch (m_CurrParticleType) {
        case ParticleType::Fire: m_Fire.Draw(m_pd3dImmediateContext.Get(), m_FireEffect); break;
        case ParticleType::Rain: m_Rain.Draw(m_pd3dImmediateContext.Get(), m_RainEffect); break;
        case ParticleType::Boom: m_Boom.Draw(m_pd3dImmediateContext.Get(), m_BoomEffect); break;
        case ParticleType::Fountain: m_Fountain.Draw(m_pd3dImmediateContext.Get(), m_FountainEffect); break;
        case ParticleType::Smoke: m_Smoke.Draw(m_pd3dImmediateContext.Get(), m_SmokeEffect); break;
        default: m_Fire.Draw(m_pd3dImmediateContext.Get(), m_FireEffect); break;
    }


    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HR(m_pSwapChain->Present(0, m_IsDxgiFlipModel ? DXGI_PRESENT_ALLOW_TEARING : 0));
}

bool GameApp::InitResource()
{
    // ******************
    // 初始化摄像机
    //

    auto camera = std::make_shared<FirstPersonCamera>();
    m_pCamera = camera;

    camera->SetViewPort(0.0f, 0.0f, (float)m_ClientWidth, (float)m_ClientHeight);
    camera->SetFrustum(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f);
    camera->LookTo(XMFLOAT3(0.0f, 0.0f, -10.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));

    // ******************
    // 初始化特效
    //

    m_BasicEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BasicEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_FireEffect.SetBlendState(RenderStates::BSAlphaWeightedAdditive.Get(), nullptr, 0xFFFFFFFF);
    m_FireEffect.SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
    m_FireEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_FireEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_BoomEffect.SetBlendState(RenderStates::BSAlphaWeightedAdditive.Get(), nullptr, 0xFFFFFFFF);
    m_BoomEffect.SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
    m_BoomEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_BoomEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_FountainEffect.SetBlendState(RenderStates::BSAlphaWeightedAdditive.Get(), nullptr, 0xFFFFFFFF);
    m_FountainEffect.SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
    m_FountainEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_FountainEffect.SetProjMatrix(camera->GetProjMatrixXM());

    float blend_factor[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    m_SmokeEffect.SetBlendState(RenderStates::BSAlphaWeightedSub.Get(), blend_factor, 0xFFFFFFFF);
    m_SmokeEffect.SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
    m_SmokeEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_SmokeEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_RainEffect.SetDepthStencilState(RenderStates::DSSNoDepthWrite.Get(), 0);
    m_RainEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_RainEffect.SetProjMatrix(camera->GetProjMatrixXM());

    m_SkyboxEffect.SetViewMatrix(camera->GetViewMatrixXM());
    m_SkyboxEffect.SetProjMatrix(camera->GetProjMatrixXM());



    // ******************
    // 初始化游戏对象
    //

    // 创建随机的树
    CreateRandomTrees();

    // 初始化地面
    Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\ground_35.obj");
    pModel->SetDebugObjectName("Ground");
    m_Ground.SetModel(pModel);

    // 天空盒
    {
        Model* pModel = m_ModelManager.CreateFromGeometry("Skybox", Geometry::CreateBox());
        pModel->SetDebugObjectName("Skybox");
        m_Skybox.SetModel(pModel);
        m_TextureManager.CreateFromFile("..\\Texture\\grasscube1024.dds", false, true);
        pModel->materials[0].Set<std::string>("$Skybox", "..\\Texture\\grasscube1024.dds");
    }

    // ******************
    // 初始化粒子系统
    //
    m_TextureManager.CreateFromFile("..\\Texture\\flare0.dds", false, true);
    m_TextureManager.CreateFromFile("..\\Texture\\flare_mul.dds", true, true);
    m_TextureManager.CreateFromFile("..\\Texture\\raindrop.dds", false, true);
    m_TextureManager.CreateFromFile("..\\Texture\\raindrop0.dds", false, true);
    m_TextureManager.CreateFromFile("..\\Texture\\ash0.dds", false, true);
    m_TextureManager.CreateFromFile("..\\Texture\\boom.dds", false, true);
    m_TextureManager.CreateFromFile("..\\Texture\\smoke_01.dds", false, true);
    // ID3D11Texture2D *resource = nullptr;
    // D3D11_TEXTURE2D_DESC desc;
    // m_TextureManager.GetTexture("..\\Texture\\flare_mul.dds")->GetResource((ID3D11Resource **)&resource);
    // SaveDDSTextureToFile(m_pd3dImmediateContext.Get(), resource,  L"..\\Texture\\flare_mul_mipmap.dds");
    // resource->GetDesc(&desc);
    // desc.Width = 64;
    // desc.Height = 64;
    // desc.MipLevels = 1;
    // ID3D11Texture2D *outTex = nullptr;
    // m_pd3dDevice->CreateTexture2D(&desc, nullptr, &outTex);


    // 创建随机数据
    std::mt19937 randEngine;
    randEngine.seed(std::random_device()());
    std::uniform_real_distribution<float> randF(-1.0f, 1.0f);
    std::uniform_real_distribution<float> randUnitF(0.0f, 1.0f);
    std::vector<float> randomValues(4096);
    
    // 生成1D随机纹理
    CD3D11_TEXTURE1D_DESC texDesc(DXGI_FORMAT_R32G32B32A32_FLOAT, 1024, 1, 1);
    D3D11_SUBRESOURCE_DATA initData{ randomValues.data(), 1024 * GetFormatSize(DXGI_FORMAT_R32G32B32A32_FLOAT) };
    ComPtr<ID3D11Texture1D> pRandomTex;
    ComPtr<ID3D11ShaderResourceView> pRandomTexSRV;

    std::generate(randomValues.begin(), randomValues.end(), [&]() { return randF(randEngine); });
    HR(m_pd3dDevice->CreateTexture1D(&texDesc, &initData, pRandomTex.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateShaderResourceView(pRandomTex.Get(), nullptr, pRandomTexSRV.ReleaseAndGetAddressOf()));
    m_TextureManager.AddTexture("FireRandomTex", pRandomTexSRV.Get());

    m_Fire.InitResource(m_pd3dDevice.Get(), 10000);
    m_Fire.SetTextureInput(m_TextureManager.GetTexture("..\\Texture\\flare0.dds"));
    m_Fire.SetTextureRandom(m_TextureManager.GetTexture("FireRandomTex"));
    m_Fire.SetTextureAsh(m_TextureManager.GetTexture("..\\Texture\\ash0.dds"));
    m_Fire.SetEmitPos(XMFLOAT3(0.0f, -1.0f, 0.0f));
    m_Fire.SetEmitDir(XMFLOAT3(0.0f, 1.0f, 0.0f));
    m_Fire.SetAcceleration(XMFLOAT3(0.0f, 7.8f, 0.0f));
    m_Fire.SetEmitInterval(0.005f);
    m_Fire.SetAliveTime(1.0f);
    m_Fire.SetDebugObjectName("Fire");

    std::generate(randomValues.begin(), randomValues.end(), [&]() { return randF(randEngine); });
    HR(m_pd3dDevice->CreateTexture1D(&texDesc, &initData, pRandomTex.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateShaderResourceView(pRandomTex.Get(), nullptr, pRandomTexSRV.ReleaseAndGetAddressOf()));
    m_TextureManager.AddTexture("BoomRandomTex", pRandomTexSRV.Get());

    auto RandomClip = [&] (float min, float max) {
        return min + randUnitF(randEngine) * (max - min);
    };
    auto RandomDirectionInCone = [&](float coneAngle) -> std::tuple<float, float, float> {
        // 生成一个随机半径 r
        float r = RandomClip(0.0f, 1.0f); 

        // 生成一个随机角度 phi
        float phi = RandomClip(0.0f, static_cast<float>(2.0f * M_PI));

        // 生成一个随机角度 theta
        float theta = acos(RandomClip(cos(coneAngle), 1.0f));

        // 将球面坐标转换为笛卡尔坐标
        float x = r * sin(theta) * cos(phi);
        float z = r * sin(theta) * sin(phi);
        float y = r * cos(theta);

        return std::tuple{x, y, z};
    };

    for (int i = 0; i < randomValues.size(); i += 4) {
        auto tmp = RandomDirectionInCone(static_cast<float>(M_PI_2 / 2));
        randomValues[i] = std::get<0>(tmp);
        randomValues[i + 1] = std::get<1>(tmp);
        randomValues[i + 2] = std::get<2>(tmp);
        randomValues[i + 3] = 1.0f;
    }
    // std::generate(randomValues.begin(), randomValues.end(), [&]() { return randF(randEngine); });
    HR(m_pd3dDevice->CreateTexture1D(&texDesc, &initData, pRandomTex.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateShaderResourceView(pRandomTex.Get(), nullptr, pRandomTexSRV.ReleaseAndGetAddressOf()));
    m_TextureManager.AddTexture("FountainRandomTex", pRandomTexSRV.Get());

    m_Boom.InitResource(m_pd3dDevice.Get(), 200000);
    m_Boom.SetTextureInput(m_TextureManager.GetTexture("..\\Texture\\boom.dds"));
    m_Boom.SetTextureRandom(m_TextureManager.GetTexture("BoomRandomTex"));
    m_Boom.SetTextureAsh(m_TextureManager.GetTexture("..\\Texture\\ash0.dds"));
    m_Boom.SetEmitPos(XMFLOAT3(0.0f, -1.0f, 0.0f));
    m_Boom.SetEmitDir(XMFLOAT3(0.0f, 1.0f, 0.0f));
    m_Boom.SetAcceleration(XMFLOAT3(1.0f, 1.0f, 1.0f));
    // m_Boom.SetEmitInterval(0.005f);
    m_Boom.SetEmitInterval(0.25f);
    m_Boom.SetAliveTime(2.5f);
    m_Boom.SetDebugObjectName("Boom");


    m_Fountain.InitResource(m_pd3dDevice.Get(), 10000);
    m_Fountain.SetTextureInput(m_TextureManager.GetTexture("..\\Texture\\raindrop0.dds"));
    m_Fountain.SetTextureRandom(m_TextureManager.GetTexture("FountainRandomTex"));
    m_Fountain.SetEmitPos(XMFLOAT3(0.0f, 0.0f, 0.0f));
    m_Fountain.SetEmitDir(XMFLOAT3(0.0f, 1.0f, 0.0f));
    m_Fountain.SetAcceleration(XMFLOAT3(0.0f, -9.8f, 0.0f));
    m_Fountain.SetEmitInterval(0.0015f);
    m_Fountain.SetAliveTime(3.0f);
    m_Fountain.SetDebugObjectName("Fountain");
    
    std::generate(randomValues.begin(), randomValues.end(), [&]() { return randF(randEngine); });
    HR(m_pd3dDevice->CreateTexture1D(&texDesc, &initData, pRandomTex.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateShaderResourceView(pRandomTex.Get(), nullptr, pRandomTexSRV.ReleaseAndGetAddressOf()));
    m_TextureManager.AddTexture("SmokeRandomTex", pRandomTexSRV.Get());

    m_Smoke.InitResource(m_pd3dDevice.Get(), 10000);
    m_Smoke.SetTextureInput(m_TextureManager.GetTexture("..\\Texture\\smoke_01.dds"));
    m_Smoke.SetTextureRandom(m_TextureManager.GetTexture("SmokeRandomTex"));
    m_Smoke.SetEmitPos(XMFLOAT3(0.0f, -1.0f, 0.0f));
    m_Smoke.SetEmitDir(XMFLOAT3(0.0f, 1.0f, 0.0f));
    m_Smoke.SetAcceleration(XMFLOAT3(0.0f, 7.8f, 0.0f));
    m_Smoke.SetEmitInterval(0.005f);
    m_Smoke.SetAliveTime(1.0f);
    m_Smoke.SetDebugObjectName("Smoke");

    std::generate(randomValues.begin(), randomValues.end(), [&]() { return randF(randEngine); });
    HR(m_pd3dDevice->CreateTexture1D(&texDesc, &initData, pRandomTex.ReleaseAndGetAddressOf()));
    HR(m_pd3dDevice->CreateShaderResourceView(pRandomTex.Get(), nullptr, pRandomTexSRV.ReleaseAndGetAddressOf()));
    m_TextureManager.AddTexture("RainRandomTex", pRandomTexSRV.Get());

    m_Rain.InitResource(m_pd3dDevice.Get(), 10000);
    m_Rain.SetTextureInput(m_TextureManager.GetTexture("..\\Texture\\raindrop.dds"));
    m_Rain.SetTextureRandom(m_TextureManager.GetTexture("RainRandomTex"));
    m_Rain.SetEmitDir(XMFLOAT3(0.0f, -1.0f, 0.0f));
    m_Rain.SetAcceleration(XMFLOAT3(-1.0f, -9.8f, 0.0f));
    m_Rain.SetEmitInterval(0.0015f);
    m_Rain.SetAliveTime(3.0f);
    m_Rain.SetDebugObjectName("Rain");

    
    // ******************
    // 初始化光照
    //
    // 方向光(默认)
    DirectionalLight dirLight[4];
    dirLight[0].ambient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);
    dirLight[0].diffuse = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
    dirLight[0].specular = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
    dirLight[0].direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
    dirLight[1] = dirLight[0];
    dirLight[1].direction = XMFLOAT3(0.577f, -0.577f, 0.577f);
    dirLight[2] = dirLight[0];
    dirLight[2].direction = XMFLOAT3(0.577f, -0.577f, -0.577f);
    dirLight[3] = dirLight[0];
    dirLight[3].direction = XMFLOAT3(-0.577f, -0.577f, -0.577f);
    for (int i = 0; i < 4; ++i)
        m_BasicEffect.SetDirLight(i, dirLight[i]);
        
    return true;
}

void GameApp::CreateRandomTrees()
{
    // 初始化树
    Model* pModel = m_ModelManager.CreateFromFile("..\\Model\\tree.obj");
    pModel->SetDebugObjectName("Trees");
    m_Trees.SetModel(pModel);
    XMMATRIX S = XMMatrixScaling(0.015f, 0.015f, 0.015f);

    BoundingBox treeBox = m_Trees.GetModel()->boundingbox;

    // 让树木底部紧贴地面位于y = -2的平面
    treeBox.Transform(treeBox, S);
    float Ty = -(treeBox.Center.y - treeBox.Extents.y + 2.0f);
    // 随机生成144颗随机朝向的树
    std::vector<BasicEffect::InstancedData> treeData(144);
    m_pInstancedBuffer = std::make_unique<Buffer>(m_pd3dDevice.Get(),
        CD3D11_BUFFER_DESC(sizeof(BasicEffect::InstancedData) * 144, D3D11_BIND_VERTEX_BUFFER,
            D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE));
    m_pInstancedBuffer->SetDebugObjectName("InstancedBuffer");

    std::mt19937 rng;
    rng.seed(std::random_device()());
    std::uniform_real_distribution<float> radiusNormDist(0.0f, 30.0f);
    std::uniform_real_distribution<float> normDist;
    float theta = 0.0f;
    int pos = 0;
    Transform transform;
    transform.SetScale(0.015f, 0.015f, 0.015f);
    for (int i = 0; i < 16; ++i)
    {
        // 取5-95的半径放置随机的树
        for (int j = 0; j < 3; ++j)
        {
            // 距离越远，树木越多
            for (int k = 0; k < 2 * j + 1; ++k, ++pos)
            {
                float radius = (float)(radiusNormDist(rng) + 30 * j + 5);
                float randomRad = normDist(rng) * XM_2PI / 16;
                transform.SetRotation(0.0f, normDist(rng) * XM_2PI, 0.0f);
                transform.SetPosition(radius * cosf(theta + randomRad), Ty, radius * sinf(theta + randomRad));

                XMStoreFloat4x4(&treeData[pos].world,
                    XMMatrixTranspose(transform.GetLocalToWorldMatrixXM()));
                XMStoreFloat4x4(&treeData[pos].worldInvTranspose,
                    XMMatrixTranspose(XMath::InverseTranspose(transform.GetLocalToWorldMatrixXM())));
            }
        }
        theta += XM_2PI / 16;
    }

    memcpy_s(m_pInstancedBuffer->MapDiscard(m_pd3dImmediateContext.Get()), m_pInstancedBuffer->GetByteWidth(),
        treeData.data(), treeData.size() * sizeof(BasicEffect::InstancedData));
    m_pInstancedBuffer->Unmap(m_pd3dImmediateContext.Get());
}
