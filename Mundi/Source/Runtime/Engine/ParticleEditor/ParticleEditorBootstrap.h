#pragma once

class ParticleEditorState;
class UWorld;
struct ID3D11Device;

// Bootstrap helpers to construct/destroy per-tab particle editor state.
class ParticleEditorBootstrap
{
public:
    static ParticleEditorState* CreateEditorState(const char* Name, UWorld* InWorld, ID3D11Device* InDevice);
    static void DestroyEditorState(ParticleEditorState*& State);
};
