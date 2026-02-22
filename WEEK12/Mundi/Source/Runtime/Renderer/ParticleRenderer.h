#pragma once

//#define MAX_PARTICLES 1000
//
//#define RELEASE_RESOURCE(MACRO) \
//    MACRO(InstanceBuffer) \
//    MACRO(InputLayout) \
//
//#define SAFE_RELEASE(Resource) \
//    SafeRelease(Resource);
//
//class D3D11RHI;
//class FSceneView;
//class UQuad;
//class UShader;
//class UParticleSystemComponent;
//
//class FParticleRenderer
//{
//public:
//    FParticleRenderer();
//    FParticleRenderer(D3D11RHI* InRHI);
//    ~FParticleRenderer();
//
//    void Initialize();
//    void Shutdown();
//    void RenderParticles(const FSceneView* View, const TArray<UParticleSystemComponent*>& ParticleComponents);
//
//private:
//   // void InitializeQuad();
//    //uint64 CreateSortKey(EBlendType Blend, float Distance, int Texture);
//
//private:
//    template<typename T>
//    inline void SafeRelease(T*& Resource);
//
//
//private:
//    D3D11RHI* RHI = nullptr;
//    UWorld* World = nullptr;
//
//    // Vertex buffer와 Index buffer는 UQuad에서 생성 및 소멸
//    ID3D11Buffer* VertexBuffer = nullptr;
//    ID3D11Buffer* IndexBuffer = nullptr;
//    ID3D11Buffer* InstanceBuffer = nullptr;
//    ID3D11InputLayout* InputLayout = nullptr;
//
//    UQuad* QuadMesh = nullptr;
//    UShader* Shader = nullptr;
//    TArray<FSpriteParticleInstance> ParticleInstanceData = {};
//    FShaderVariant* CurrentVariant = nullptr;
//};
//
//template <typename T>
//void FParticleRenderer::SafeRelease(T*& Resource)
//{
//    if (Resource)
//    {
//        Resource->Release();
//        Resource = nullptr;
//    }
//}
