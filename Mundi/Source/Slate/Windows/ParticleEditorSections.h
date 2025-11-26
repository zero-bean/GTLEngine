#pragma once
#include <map>
#include <string>

class SParticleEditorWindow;
class ParticleEditorState;
class UTexture;
struct ImVec2;

struct FParticleEditorSectionContext
{
    SParticleEditorWindow& Window;
    ParticleEditorState* ActiveState = nullptr;
    bool* bShowColorPicker = nullptr;
};

class FParticleEditorSection
{
public:
    virtual ~FParticleEditorSection() = default;
    virtual void Draw(const FParticleEditorSectionContext& Context) = 0;
};

class FParticleEditorToolBarSection : public FParticleEditorSection
{
public:
    FParticleEditorToolBarSection();
    virtual void Draw(const FParticleEditorSectionContext& Context) override;

private:
    void EnsureIconsLoaded();
    bool DrawIconButton(const char* Id, UTexture* Icon, const char* Label, const char* Tooltip, float ButtonWidth, ImVec2* OutRectMin = nullptr, ImVec2* OutRectMax = nullptr);

    UTexture* IconSave = nullptr;
    UTexture* IconLoad = nullptr;
    UTexture* IconResetSimul = nullptr;
    UTexture* IconResetLevel = nullptr;
    UTexture* IconPlay = nullptr;
    UTexture* IconPause = nullptr;
    UTexture* IconNextFrame = nullptr;
    UTexture* IconColor = nullptr;
    UTexture* IconAxis = nullptr;
    UTexture* IconGrid = nullptr;
    float IconSize = 40.0f;
};

class FParticleEditorViewportSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;
};

class FParticleEditorEmitterSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;

private:
    void CreateNewEmitter(ParticleEditorState* State);
    void DeleteSelectedEmitter(ParticleEditorState* State);
};

class FParticleEditorDetailSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;

private:
    void DrawTypeDataMeshModuleProperties(class UParticleModuleTypeDataMesh* Module, const FParticleEditorSectionContext& Context);
    void DrawTypeDataBeamModuleProperties(class UParticleModuleTypeDataBeam* Module, const FParticleEditorSectionContext& Context);
    void DrawRequiredModuleProperties(class UParticleModuleRequired* Module, const FParticleEditorSectionContext& Context);
    void DrawSpawnModuleProperties(class UParticleModuleSpawn* Module);
    void DrawModuleProperties(class UParticleModule* Module, int32 ModuleIndex);
    void DrawDistributionFloat(const char* Label, struct FRawDistributionFloat& Distribution,
        float Min = 0.0f, float Max = 100.0f);
    void DrawDistributionVector(const char* Label, struct FRawDistributionVector& Distribution,
        float Min = 0.0f, float Max = 100.0f, bool bIsColor = false);

    void AddSpawnModule(ParticleEditorState* State, class UParticleLODLevel* LODLevel);
    void DeleteTypeModule(ParticleEditorState* State, class UParticleLODLevel* LODLevel);
    void DeleteSpawnModule(ParticleEditorState* State, class UParticleLODLevel* LODLevel);
    void AddModule(ParticleEditorState* State, class UParticleLODLevel* LODLevel, const char* ModuleClassName);
    void DeleteModule(ParticleEditorState* State, class UParticleLODLevel* LODLevel, int32 ModuleIndex);
};

class FParticleEditorCurveSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;

private:
    struct FCurveViewRange
    {
        float MinValue = 0.0f;
        float MaxValue = 1.0f;
        float DefaultMin = 0.0f;
        float DefaultMax = 1.0f;
    };

    void DrawCurveEditor(struct FCurve* Curve, const char* PropertyName, float MinValue = 0.0f, float MaxValue = 1.0f);

    // 범용 Distribution 커브 에디터 (Vector 3축)
    void DrawVectorDistributionCurveEditor(
        struct FRawDistributionVector* Distribution,
        const char* ModuleName,
        void* ModulePtr,
        float MinValue,
        float MaxValue,
        const char* AxisNames[3] = nullptr  // 기본: X, Y, Z
    );

    // 범용 Distribution 커브 에디터 (Float 단일)
    void DrawFloatDistributionCurveEditor(
        struct FRawDistributionFloat* Distribution,
        const char* ModuleName,
        void* ModulePtr,
        float MinValue,
        float MaxValue
    );

    // 드래그 상태 (PropertyName별로 관리)
    std::map<std::string, int32> DraggingKeyIndexMap;
    std::map<std::string, bool> IsDraggingMap;

    // 현재 선택된 축 (PropertyName별로 관리)
    std::map<std::string, int32> SelectedAxisIndexMap;

    // 커브별 사용자 정의 그리드 범위
    std::map<std::string, FCurveViewRange> CurveRangeMap;
};
