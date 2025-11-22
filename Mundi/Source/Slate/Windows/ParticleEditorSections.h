#pragma once

class SParticleEditorWindow;
class ParticleEditorState;

struct FParticleEditorSectionContext
{
    SParticleEditorWindow& Window;
    ParticleEditorState* ActiveState = nullptr;
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
    virtual void Draw(const FParticleEditorSectionContext& Context) override;
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
