#include "Effects.h"
#include <XUtil.h>
#include <RenderStates.h>
#include <EffectHelper.h>
#include <DXTrace.h>
#include <Vertex.h>
#include <TextureManager.h>
#include <filesystem>
#include "LightHelper.h"

using namespace DirectX;

# pragma warning(disable: 26812)

//
// ParticleEffect::Impl 需要先于BasicEffect的定义
//

class ParticleEffect::Impl
{
public:
    // 必须显式指定
    Impl() {}
    ~Impl() = default;

public:
    template<class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<EffectHelper> m_pEffectHelper;

    std::shared_ptr<IEffectPass> m_pCurrEffectPass;

    ComPtr<ID3D11InputLayout> m_pVertexParticleLayout;



    XMFLOAT4X4 m_View{}, m_Proj{};
};

//
// ParticleEffect
//

ParticleEffect::ParticleEffect()
{
    pImpl = std::make_unique<ParticleEffect::Impl>();
}

ParticleEffect::~ParticleEffect()
{
}

ParticleEffect::ParticleEffect(ParticleEffect&& moveFrom) noexcept
{
    this->pImpl.swap(moveFrom.pImpl);
}

ParticleEffect& ParticleEffect::operator=(ParticleEffect&& moveFrom) noexcept
{
    this->pImpl.swap(moveFrom.pImpl);
    return *this;
}

bool ParticleEffect::InitAll(ID3D11Device* device, std::wstring_view filename)
{
    namespace fs = std::filesystem;

    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    const D3D11_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"ACCEL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TYPE", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"EMIT_COUNT", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    const D3D11_SO_DECLARATION_ENTRY outputLayout[] = {
        {0, "POSITION", 0, 0, 3, 0},
        {0, "VELOCITY", 0, 0, 3, 0},
        {0, "ACCEL", 0, 0, 3, 0},
        {0, "SIZE", 0, 0, 2, 0},
        {0, "AGE", 0, 0, 1, 0},
        {0, "TYPE", 0, 0, 1, 0},
        {0, "EMIT_COUNT", 0, 0, 1, 0},
    };

    fs::path cacheDir = "Shaders\\Cache";
    bool overWrite = true;
    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(cacheDir.c_str(), overWrite);

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // ******************
    // 创建顶点着色器
    //
    
    fs::path path = filename;
    fs::path stem = fs::path(filename).stem();

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_SO_VS"), filename,
        device, "SO_VS", "vs_5_0", nullptr, blob.GetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(inputLayouts, ARRAYSIZE(inputLayouts),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexParticleLayout.GetAddressOf()));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_VS"), filename,
        device, "VS", "vs_5_0"));

    // ******************
    // 创建几何/流输出着色器
    //

    // 流输出几何着色器
    fs::path csoFilename = stem;
    csoFilename += "_SO_GS.cso";
    csoFilename = cacheDir / csoFilename;
    if (overWrite || FAILED(D3DReadFileToBlob(csoFilename.c_str(), blob.ReleaseAndGetAddressOf())))
    {
        HR(EffectHelper::CompileShaderFromFile(filename, "SO_GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
        D3DWriteBlobToFile(blob.Get(), csoFilename.c_str(), overWrite);
    }
    
    
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> pGS;
    uint32_t strides[] = { sizeof(VertexParticle) };
    HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(),
        outputLayout, ARRAYSIZE(outputLayout), strides, 1, D3D11_SO_NO_RASTERIZED_STREAM,
        nullptr, pGS.GetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddGeometryShaderWithStreamOutput((stem.string() + "_SO_GS"), device, pGS.Get(), blob.Get()));

    // 几何着色器
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_GS"), filename,
        device, "GS", "gs_5_0"));

    // ******************
    // 创建像素着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_PS"), filename,
        device, "PS", "ps_5_0"));

    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    std::string nameVS, nameGS, namePS;
    nameVS = stem.string() + "_VS";
    nameGS = stem.string() + "_GS";
    namePS = stem.string() + "_PS";
    passDesc.nameVS = nameVS;
    passDesc.nameGS = nameGS;
    passDesc.namePS = namePS;
    HR(pImpl->m_pEffectHelper->AddEffectPass("Render", device, &passDesc));

    nameVS = stem.string() + "_SO_VS";
    nameGS = stem.string() + "_SO_GS";
    passDesc.nameVS = nameVS;
    passDesc.nameGS = nameGS;
    passDesc.namePS = "";
    HR(pImpl->m_pEffectHelper->AddEffectPass("StreamOutput", device, &passDesc));
    // pImpl->m_pEffectHelper->GetEffectPass("StreamOutput")->SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);
    
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinear", RenderStates::SSLinearWrap.Get());
    // pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamShadow", RenderStates::SSShadowPCF.Get());

    return true;
}


bool ParticleEffect::InitAllWithSmoke(ID3D11Device* device, std::wstring_view filename)
{
    namespace fs = std::filesystem;

    if (!device)
        return false;

    if (!RenderStates::IsInit())
        throw std::exception("RenderStates need to be initialized first!");

    pImpl->m_pEffectHelper = std::make_unique<EffectHelper>();

    const D3D11_INPUT_ELEMENT_DESC inputLayouts[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"ACCEL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"AGE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TYPE", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"EMIT_COUNT", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    const D3D11_SO_DECLARATION_ENTRY outputLayout[] = {
        {0, "POSITION", 0, 0, 3, 0},
        {0, "VELOCITY", 0, 0, 3, 0},
        {0, "ACCEL", 0, 0, 3, 0},
        {0, "SIZE", 0, 0, 2, 0},
        {0, "AGE", 0, 0, 1, 0},
        {0, "TYPE", 0, 0, 1, 0},
        {0, "EMIT_COUNT", 0, 0, 1, 0},
    };

    fs::path cacheDir = "Shaders\\Cache";
    bool overWrite = true;
    pImpl->m_pEffectHelper->SetBinaryCacheDirectory(cacheDir.c_str(), overWrite);

    Microsoft::WRL::ComPtr<ID3DBlob> blob;

    // ******************
    // 创建顶点着色器
    //
    
    fs::path path = filename;
    fs::path stem = fs::path(filename).stem();

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_SO_VS"), filename,
        device, "SO_VS", "vs_5_0", nullptr, blob.GetAddressOf()));
    // 创建顶点布局
    HR(device->CreateInputLayout(inputLayouts, ARRAYSIZE(inputLayouts),
        blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexParticleLayout.GetAddressOf()));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_VS"), filename,
        device, "VS", "vs_5_0"));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_BackBuffer_VS"), filename,
        device, "BackBuffer_VS", "vs_5_0"));

    // ******************
    // 创建几何/流输出着色器
    //

    // 流输出几何着色器
    fs::path csoFilename = stem;
    csoFilename += "_SO_GS.cso";
    csoFilename = cacheDir / csoFilename;
    if (overWrite || FAILED(D3DReadFileToBlob(csoFilename.c_str(), blob.ReleaseAndGetAddressOf())))
    {
        HR(EffectHelper::CompileShaderFromFile(filename, "SO_GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
        D3DWriteBlobToFile(blob.Get(), csoFilename.c_str(), overWrite);
    }
    
    
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> pGS;
    uint32_t strides[] = { sizeof(VertexParticle) };
    HR(device->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(),
        outputLayout, ARRAYSIZE(outputLayout), strides, 1, D3D11_SO_NO_RASTERIZED_STREAM,
        nullptr, pGS.GetAddressOf()));
    HR(pImpl->m_pEffectHelper->AddGeometryShaderWithStreamOutput((stem.string() + "_SO_GS"), device, pGS.Get(), blob.Get()));

    // 几何着色器
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_GS"), filename,
        device, "GS", "gs_5_0"));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_BackBuffer_GS"), filename,
        device, "BackBuffer_GS", "gs_5_0"));

    // ******************
    // 创建像素着色器
    //
    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_PS"), filename,
        device, "PS", "ps_5_0"));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_Smoke_PS"), filename,
        device, "Smoke_PS", "ps_5_0"));

    HR(pImpl->m_pEffectHelper->CreateShaderFromFile((stem.string() + "_BackBuffer_PS"), filename,
        device, "BackBuffer_PS", "ps_5_0"));
    // ******************
    // 创建通道
    //
    EffectPassDesc passDesc;
    std::string nameVS, nameGS, namePS;
    nameVS = stem.string() + "_VS";
    nameGS = stem.string() + "_GS";
    namePS = stem.string() + "_PS";
    passDesc.nameVS = nameVS;
    passDesc.nameGS = nameGS;
    passDesc.namePS = namePS;
    HR(pImpl->m_pEffectHelper->AddEffectPass("Render", device, &passDesc));

    namePS = stem.string() + "_Smoke_PS";
    passDesc.namePS = namePS;
    HR(pImpl->m_pEffectHelper->AddEffectPass("RenderSmoke", device, &passDesc));

    nameVS = stem.string() + "_BackBuffer_VS";
    nameGS = stem.string() + "_BackBuffer_GS";
    namePS = stem.string() + "_BackBuffer_PS";
    passDesc.nameVS = nameVS;
    passDesc.nameGS = "";
    passDesc.namePS = namePS;
    HR(pImpl->m_pEffectHelper->AddEffectPass("RenderToBackBuffer", device, &passDesc));

    nameVS = stem.string() + "_SO_VS";
    nameGS = stem.string() + "_SO_GS";
    passDesc.nameVS = nameVS;
    passDesc.nameGS = nameGS;
    passDesc.namePS = "";
    HR(pImpl->m_pEffectHelper->AddEffectPass("StreamOutput", device, &passDesc));
    // pImpl->m_pEffectHelper->GetEffectPass("StreamOutput")->SetDepthStencilState(RenderStates::DSSNoDepthTest.Get(), 0);
    
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinear", RenderStates::SSLinearWrap.Get());
    pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamLinearBoard", RenderStates::SSLinearBoard.Get());
    // pImpl->m_pEffectHelper->SetSamplerStateByName("g_SamShadow", RenderStates::SSShadowPCF.Get());

    return true;
}

void ParticleEffect::RenderToVertexBuffer(
    ID3D11DeviceContext* deviceContext,
    ID3D11Buffer* input,
    ID3D11Buffer* output,
    uint32_t vertexCount)
{
    pImpl->m_pEffectHelper->GetEffectPass("StreamOutput")->Apply(deviceContext);
    deviceContext->IASetInputLayout(pImpl->m_pVertexParticleLayout.Get());
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    
    UINT strides[1] = { sizeof(VertexParticle) };
    UINT offsets[1] = { 0 };
    deviceContext->IASetVertexBuffers(0, 1, &input, strides, offsets);
    
    // 经过流输出写入到顶点缓冲区
    deviceContext->SOSetTargets(1, &output, offsets);
    if (vertexCount)
    {
        deviceContext->Draw(vertexCount, 0);
    }
    else
    {
        deviceContext->DrawAuto();
    }
    
    // 解绑
    deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    deviceContext->SOSetTargets(0, nullptr, nullptr);
}

ParticleEffect::InputData ParticleEffect::SetRenderDefault()
{
    InputData res{};
    res.stride = sizeof(ParticleEffect::VertexParticle);
    res.pInputLayout = pImpl->m_pVertexParticleLayout.Get();
    res.topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("Render");
    return res;
}

ParticleEffect::InputData ParticleEffect::SetRenderSmoke()
{
    InputData res{};
    res.stride = sizeof(ParticleEffect::VertexParticle);
    res.pInputLayout = pImpl->m_pVertexParticleLayout.Get();
    res.topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("RenderSmoke");
    return res;
}

ParticleEffect::InputData ParticleEffect::SetRenderToBackBuffer()
{
    InputData res{};
    res.stride = sizeof(ParticleEffect::VertexParticle);
    res.pInputLayout = pImpl->m_pVertexParticleLayout.Get();
    res.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    pImpl->m_pCurrEffectPass = pImpl->m_pEffectHelper->GetEffectPass("RenderToBackBuffer");
    return res;
}

void XM_CALLCONV ParticleEffect::SetViewMatrix(DirectX::FXMMATRIX V)
{
    XMStoreFloat4x4(&pImpl->m_View, V);
}

void XM_CALLCONV ParticleEffect::SetProjMatrix(DirectX::FXMMATRIX P)
{
    XMStoreFloat4x4(&pImpl->m_Proj, P);
}

void ParticleEffect::SetEyePos(const DirectX::XMFLOAT3& eyePos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EyePosW")->SetFloatVector(3, (const FLOAT*)&eyePos);
}

void ParticleEffect::SetGameTime(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_GameTime")->SetFloat(t);
}

void ParticleEffect::SetTimeStep(float step)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_TimeStep")->SetFloat(step);
}

void ParticleEffect::SetEmitDir(const DirectX::XMFLOAT3& dir)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitDirW")->SetFloatVector(3, (const FLOAT*)&dir);
}

void ParticleEffect::SetEmitPos(const DirectX::XMFLOAT3& pos)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitPosW")->SetFloatVector(3, (const FLOAT*)&pos);
}

void ParticleEffect::SetEmitInterval(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_EmitInterval")->SetFloat(t);
}

void ParticleEffect::SetAliveTime(float t)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AliveTime")->SetFloat(t);
}

void ParticleEffect::SetAcceleration(const DirectX::XMFLOAT3& accel)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_AccelW")->SetFloatVector(3, reinterpret_cast<const float*>(&accel)); 
}

void ParticleEffect::SetParticleCount(uint32_t const defaultParticle, uint32_t smokeParticle)
{
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_DefaultParticleCount")->SetUInt(defaultParticle); 
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_SmokeParticleCount")->SetUInt(smokeParticle); 
}

void ParticleEffect::SetTextureInput(ID3D11ShaderResourceView* textureInput)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureInput", textureInput);
}

void ParticleEffect::SetTextureRandom(ID3D11ShaderResourceView* textureRandom)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureRandom", textureRandom);
}

void ParticleEffect::SetTextureAsh(ID3D11ShaderResourceView* textureAsh)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureAsh", textureAsh);
}

void ParticleEffect::SetTextureDefaultParticle(ID3D11ShaderResourceView* textureDefaultParticle)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureDefaultParticle", textureDefaultParticle);
}

void ParticleEffect::SetTextureSmokeParticle(ID3D11ShaderResourceView* textureSmokeParticle)
{
    pImpl->m_pEffectHelper->SetShaderResourceByName("g_TextureSmokeParticle", textureSmokeParticle);
}

void ParticleEffect::SetRasterizerState(ID3D11RasterizerState* rasterizerState)
{
    pImpl->m_pEffectHelper->GetEffectPass("Render")->SetRasterizerState(rasterizerState);
}

void ParticleEffect::SetBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask)
{
    pImpl->m_pEffectHelper->GetEffectPass("Render")->SetBlendState(blendState, blendFactor, sampleMask);
}

void ParticleEffect::SetDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef)
{
    pImpl->m_pEffectHelper->GetEffectPass("Render")->SetDepthStencilState(depthStencilState, stencilRef);
}

void ParticleEffect::SetSmokeRasterizerState(ID3D11RasterizerState* rasterizerState)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderSmoke")->SetRasterizerState(rasterizerState);
}

void ParticleEffect::SetSmokeBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderSmoke")->SetBlendState(blendState, blendFactor, sampleMask);
}

void ParticleEffect::SetSmokeDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderSmoke")->SetDepthStencilState(depthStencilState, stencilRef);
}

void ParticleEffect::SetBackBufferRasterizerState(ID3D11RasterizerState* rasterizerState)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderToBackBuffer")->SetRasterizerState(rasterizerState);
}

void ParticleEffect::SetBackBufferBlendState(ID3D11BlendState* blendState, const float blendFactor[4], uint32_t sampleMask)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderToBackBuffer")->SetBlendState(blendState, blendFactor, sampleMask);
}

void ParticleEffect::SetBackBufferDepthStencilState(ID3D11DepthStencilState* depthStencilState, UINT stencilRef)
{
    pImpl->m_pEffectHelper->GetEffectPass("RenderToBackBuffer")->SetDepthStencilState(depthStencilState, stencilRef);
}

void ParticleEffect::Apply(ID3D11DeviceContext* deviceContext)
{
    XMMATRIX V = XMLoadFloat4x4(&pImpl->m_View);
    XMMATRIX P = XMLoadFloat4x4(&pImpl->m_Proj);

    XMMATRIX VP = V * P;
    VP = XMMatrixTranspose(VP);
    pImpl->m_pEffectHelper->GetConstantBufferVariable("g_ViewProj")->SetFloatMatrix(4, 4, (FLOAT*)&VP);

    if (pImpl->m_pCurrEffectPass)
        pImpl->m_pCurrEffectPass->Apply(deviceContext);
}
