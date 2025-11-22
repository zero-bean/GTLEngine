#pragma once

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

class FParticleEditorMenuBarSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;
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
    UTexture* IconColor = nullptr;
    UTexture* IconThumbnail = nullptr;
    UTexture* IconBound = nullptr;
    UTexture* IconAxis = nullptr;
    UTexture* IconLOD = nullptr;
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
};

class FParticleEditorDetailSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;
};

class FParticleEditorCurveSection : public FParticleEditorSection
{
public:
    virtual void Draw(const FParticleEditorSectionContext& Context) override;
};
