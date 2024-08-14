#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <CameraController.h>
#include <RenderStates.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <Collision.h>
#include <TextureManager.h>
#include "ParticleManager.h"

class GameApp : public D3DApp
{
public:
    GameApp(HINSTANCE hInstance, const std::wstring& windowName, int initWidth, int initHeight);
    ~GameApp();

    bool Init();
    void OnResize();
    void UpdateScene(float dt);
    void DrawScene();

private:
    bool InitResource();

private:

    enum class ParticleType {
        Fire = 0,
        Smoke,
        FireSmoke,
        Boom,
        Fountain,
    };

    TextureManager m_TextureManager;

    std::unique_ptr<Depth2D> m_pDepthTexture;                           // 深度缓冲区

    std::unique_ptr<Buffer> m_pInstancedBuffer;                         // 树的实例缓冲区
    ParticleManager m_Fire;                                             // 火焰粒子系统
    ParticleManager m_Boom;                                             // 爆炸粒子系统
    ParticleManager m_Fountain;                                         // 喷泉粒子系统
    ParticleManager m_Smoke;                                            // 烟雾粒子系统
    ParticleManager m_FireSmoke;                                        // 火焰烟雾粒子系统

    ParticleEffect m_FireEffect;                                        // 火焰特效
    ParticleEffect m_BoomEffect;                                        // 爆炸特效
    ParticleEffect m_FountainEffect;                                    // 喷泉特效
    ParticleEffect m_SmokeEffect;                                       // 烟雾特效
    ParticleEffect m_FireSmokeEffect;                                   // 火焰烟雾特效
    ParticleType m_CurrParticleType = ParticleType::Fire;                                    // 当前显示粒子
    ParticleManager *m_CurrParticle = &m_Fire;
    ParticleEffect *m_CurrEffect = &m_FireEffect;

    std::shared_ptr<FirstPersonCamera> m_pCamera;				        // 摄像机
    FirstPersonCameraController m_CameraController;                     // 摄像机控制器
};


#endif