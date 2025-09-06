#include "pch.h"
#include "Manager/ImGui/Public/ImGuiManager.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "Manager/Input/Public/InputManager.h"
#include "Manager/Time/Public/TimeManager.h"

#include "Mesh/Public/CubeActor.h"

#include "Render/Public/Renderer.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

IMPLEMENT_SINGLETON(UImGuiManager)

UImGuiManager::UImGuiManager() = default;

UImGuiManager::~UImGuiManager() = default;

/**
 * @brief ImGui 초기화 함수
 */
void UImGuiManager::Init(HWND InWindowHandle)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(InWindowHandle);

	auto& Renderer = URenderer::GetInstance();
	ImGui_ImplDX11_Init(Renderer.GetDevice(), Renderer.GetDeviceContext());
}

/**
 * @brief ImGui 자원 해제 함수
 */
void UImGuiManager::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

/**
 * @brief ImGui Process
 */
void UImGuiManager::Render(AActor* SelectedActor)
{
	// Get New Frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Get Screen Info
	ImGuiIO& IO = ImGui::GetIO();
	float ScreenWidth = IO.DisplaySize.x;
	float ScreenHeight = IO.DisplaySize.y;

	// Panel Setting
	float RightPanelWidth = 300.0f;
	float CurrentY = 10.0f;
	float WindowPadding = 10.0f;
	float RightPanelX = ScreenWidth - RightPanelWidth - WindowPadding;


	// Show Demo
	// Frame Performance Info
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, 120.0f));
	ImGui::Begin("Frame Performance Info", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	auto& TimeManager = UTimeManager::GetInstance();
	float CurrentFPS = TimeManager.GetFPS();

	ImVec4 FPSColor;

	if (CurrentFPS >= 60.0f)
	{
		FPSColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // 녹색 (우수)
	}
	else if (CurrentFPS >= 30.0f)
	{
		FPSColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색 (보통)
	}
	else
	{
		FPSColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // 빨간색 (주의)
	}

	ImGui::TextColored(FPSColor, "FPS: %.1f", CurrentFPS);
	ImGui::Text("Delta Time: %.2f ms", DT * 1000.0f);
	ImGui::Text("Game Time: %.1f s", DT);

	ImGui::Separator();

	if (TimeManager.IsPaused())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Game Paused");
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Game Resumed");
	}

	ImGui::End();
	CurrentY += 130.0f;

	// Key Input Status
	float KeyInputHeight = ScreenHeight - CurrentY - WindowPadding;
	ImGui::SetNextWindowPos(ImVec2(RightPanelX, CurrentY));
	ImGui::SetNextWindowSize(ImVec2(RightPanelWidth, KeyInputHeight));
	ImGui::Begin("Key Input Status", nullptr,
	             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

	// KeyManager에서 현재 눌린 키들 가져오기
	auto& KeyManager = UInputManager::GetInstance();

	TArray<EKeyInput> PressedKeys = KeyManager.GetPressedKeys();

	ImGui::Text("Pressed Keys:");
	ImGui::Separator();

	if (PressedKeys.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Nothing Input)");
	}
	else
	{
		for (const EKeyInput& Key : PressedKeys)
		{
			const wchar_t* KeyString = UInputManager::KeyInputToString(Key);
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "- %ls", KeyString);
		}
	}

	ImGui::Separator();

	// 마우스 위치 표시
	FVector MousePosition = KeyManager.GetMousePosition();
	ImGui::Text("Mouse: (%.0f, %.0f)", MousePosition.X, MousePosition.Y);
	ImGui::Text("Total Keys: %d", static_cast<int>(PressedKeys.size()));

	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Controls:");
	ImGui::Text("ESC: Exit");
	ImGui::Text("Space: Shooter (Hold / Release)");
	ImGui::Text("A: Move Left Pad");
	ImGui::Text("D: Move Right Pad");

	ImGui::End();

	if (SelectedActor)
	{ //SceneComponent 포함하는지 확인 핋요
		FVector ActorLocation = SelectedActor->GetActorLocation();
		FVector ActorRotation = SelectedActor->GetActorRotation();
		FVector ActorScale3D = SelectedActor->GetActorScale3D();
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(300,100));
		ImGui::Text("Position");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("x##Position", &ActorLocation.X,0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("y##Position", &ActorLocation.Y, 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("z##Position", &ActorLocation.Z, 0.1f);
		ImGui::Text("Rotation");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("x##Rotation", &ActorRotation.X, 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("y##Rotation", &ActorRotation.Y, 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("z##Rotation", &ActorRotation.Z, 0.1f);
		ImGui::Text("Scale");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("x##Scale", &ActorScale3D.X, 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("y##Scale", &ActorScale3D.Y, 0.1f);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(70);
		ImGui::DragFloat("z##Scale", &ActorScale3D.Z, 0.1f);

		SelectedActor->SetActorLocation(ActorLocation);
		SelectedActor->SetActorRotation(ActorRotation);
		SelectedActor->SetActorScale3D(ActorScale3D);
	}

	// Render ImGui
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

/**
 * @brief WndProc Handler 래핑 함수
 * @return ImGui 자체 함수 반환
 */
LRESULT UImGuiManager::WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
