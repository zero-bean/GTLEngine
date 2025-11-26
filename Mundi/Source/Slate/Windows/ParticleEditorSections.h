#pragma once

class SParticleEditorWindow;
class ParticleEditorState;
class UTexture;
struct ImVec2;

enum class EEmitterType
{
    Sprite,
    Mesh
};
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
    void CreateNewEmitter(ParticleEditorState* State, EEmitterType EmitterType);
    void DeleteSelectedEmitter(ParticleEditorState* State);
};

class FParticleEditorDetailSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;

private:
    void DrawTypeDataMeshModuleProperties(class UParticleModuleTypeDataMesh* Module, const FParticleEditorSectionContext& Context);
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
};
