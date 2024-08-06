#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <random>
#include <WinMin.h>
#include "d3dApp.h"
#include "Effects.h"
#include <CameraController.h>
#include <RenderStates.h>
#include <GameObject.h>
#include <Texture2D.h>
#include <Buffer.h>
#include <Collision.h>
#include <ModelManager.h>
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
    void CreateRandomTrees();

private:

    enum class ParticleType {
        Fire = 0,
        Rain,
        Boom,
        Fountain,
    };

    TextureManager m_TextureManager;
    ModelManager m_ModelManager;

    std::unique_ptr<Depth2D> m_pDepthTexture;                           // 深度缓冲区

    GameObject m_Trees;										            // 树
    GameObject m_Ground;										        // 地面                 
    std::unique_ptr<Buffer> m_pInstancedBuffer;                         // 树的实例缓冲区
    GameObject m_Skybox;                                                // 天空盒
    ParticleManager m_Rain;                                             // 雨水粒子系统
    ParticleManager m_Fire;                                             // 火焰粒子系统
    ParticleManager m_Boom;                                             // 爆炸粒子系统
    ParticleManager m_Fountain;                                         // 喷泉粒子系统

    BasicEffect m_BasicEffect;								            // 对象渲染特效管理
    SkyboxEffect m_SkyboxEffect;                                        // 天空盒特效
    ParticleEffect m_RainEffect;                                        // 雨水特效
    ParticleEffect m_FireEffect;                                        // 火焰特效
    ParticleEffect m_BoomEffect;                                        // 爆炸特效
    ParticleEffect m_FountainEffect;                                    // 喷泉特效
    ParticleType m_CurrParticleType = ParticleType::Fountain;                                    // 当前显示粒子

    std::unique_ptr<Texture2D> m_pLitTexture;                           // 中间场景缓冲区

    std::shared_ptr<FirstPersonCamera> m_pCamera;				        // 摄像机
    FirstPersonCameraController m_CameraController;                     // 摄像机控制器
};


#endif