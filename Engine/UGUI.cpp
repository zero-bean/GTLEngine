#include "stdafx.h"
#include "UGUI.h"
#include "UTimeManager.h"
#include "UClass.h"

IMPLEMENT_UCLASS(UGUI, UEngineSubsystem)

// Forward declaration for external time manager access
extern UTimeManager* g_pTimeManager;

UGUI::UGUI()
    : bInitialized(false)
    , bShowDemoWindow(false)
{
}

UGUI::~UGUI()
{
    Shutdown();
}

bool UGUI::Initialize(HWND hWnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
    if (bInitialized)
        return true;

    if (!hWnd || !device || !deviceContext)
        return false;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable keyboard and gamepad controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // Alternative: ImGui::StyleColorsClassic();

    // Customize style for engine
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    // Setup Platform/Renderer backends
    if (!ImGui_ImplWin32_Init(hWnd))
    {
        ImGui::DestroyContext();
        return false;
    }

    if (!ImGui_ImplDX11_Init(device, deviceContext))
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    bInitialized = true;
    return true;
}

void UGUI::Shutdown()
{
    if (!bInitialized)
        return;

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    bInitialized = false;
}

void UGUI::BeginFrame()
{
    if (!bInitialized)
        return;

    // Start the Dear ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UGUI::EndFrame()
{
    if (!bInitialized)
        return;

    // Rendering
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UGUI::Render()
{
    if (!bInitialized)
        return;
}

bool UGUI::WantCaptureMouse() const
{
    if (!bInitialized)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

bool UGUI::WantCaptureKeyboard() const
{
    if (!bInitialized)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}