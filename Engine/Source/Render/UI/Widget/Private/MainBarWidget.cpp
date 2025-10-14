#include "pch.h"
#include "Render/UI/Widget/Public/MainBarWidget.h"
#include "Manager/UI/Public/UIManager.h"
#include "Render/UI/Window/Public/UIWindow.h"

#include <shobjidl.h>

#include "Level/Public/Level.h"
#include "Editor/Public/EditorEngine.h"
#include "Actor/Public/Actor.h"
#include "Component/Mesh/Public/StaticMeshComponent.h"
#include "Render/Renderer/Public/Renderer.h"


IMPLEMENT_CLASS(UMainBarWidget, UWidget)

UMainBarWidget::UMainBarWidget()
	: UWidget("MainBarWidget")
{
}

/**
 * @brief MainBarWidget 초기화 함수
 * UIManager 인스턴스를 여기서 가져온다
 */
void UMainBarWidget::Initialize()
{
	UIManager = &UUIManager::GetInstance();
	if (!UIManager)
	{
		UE_LOG("MainBarWidget: UIManager를 찾을 수 없습니다!");
		return;
	}

	UE_LOG("MainBarWidget: 메인 메뉴바 위젯이 초기화되었습니다");
}

void UMainBarWidget::Update()
{
	// 업데이트 정보 필요할 경우 추가할 것
}

void UMainBarWidget::RenderWidget()
{
	if (!bIsMenuBarVisible)
	{
		MenuBarHeight = 0.0f;
		return;
	}

	// 메인 메뉴바 시작
	if (ImGui::BeginMainMenuBar())
	{
		// 뷰포트 조정을 위해 메뉴바 높이 저장
		MenuBarHeight = ImGui::GetWindowSize().y;

		// 메뉴 Listing
		RenderFileMenu();
		RenderViewMenu();
		RenderShowFlagsMenu();
		RenderGraphicsMenu();
		RenderLODMenu();
		RenderOcclusionCullingMenu();
		RenderPIEMenu();
		RenderWindowsMenu();
		RenderHelpMenu();

		// 메인 메뉴바 종료
		ImGui::EndMainMenuBar();
	}
	else
	{
		MenuBarHeight = 0.0f;
	}
}

/**
 * @brief File 메뉴를 렌더링합니다
 */
void UMainBarWidget::RenderFileMenu()
{
	if (ImGui::BeginMenu("파일"))
	{
		// 레벨 관련 메뉴
		if (ImGui::MenuItem("새 레벨", "Ctrl+N"))
		{
			CreateNewLevel();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("레벨 열기", "Ctrl+O"))
		{
			LoadLevel();
		}

		if (ImGui::MenuItem("레벨 저장", "Ctrl+S"))
		{
			SaveCurrentLevel();
		}

		ImGui::Separator();

		// 일반 파일 작업
		if (ImGui::MenuItem("일반 파일 열기"))
		{
			UE_LOG("MainBarWidget: 일반 파일 열기 메뉴 선택됨");
			// TODO(KHJ): 일반 파일 열기 로직 구현
		}

		ImGui::Separator();

		if (ImGui::MenuItem("종료", "Alt+F4"))
		{
			UE_LOG("MainBarWidget: 프로그램 종료 메뉴 선택됨");
			// TODO(KHJ): 프로그램 종료 로직 구현
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief Windows 토글 메뉴를 렌더링하는 함수
 * 등록된 MainMenu를 제외한 모든 UIWindow의 토글 옵션을 표시
 */
void UMainBarWidget::RenderWindowsMenu() const
{
	if (ImGui::BeginMenu("창"))
	{
		if (!UIManager)
		{
			ImGui::Text("UIManager를 사용할 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// 모든 등록된 UIWindow에 대해 토글 메뉴 항목 생성
		const auto& AllWindows = UIManager->GetAllUIWindows();

		if (AllWindows.empty())
		{
			ImGui::Text("등록된 창이 없습니다");
		}
		else
		{
			for (auto* Window : AllWindows)
			{
				if (!Window)
				{
					continue;
				}

				if (Window->GetWindowTitle() == "MainMenuBar")
				{
					continue;
				}

				if (ImGui::MenuItem(Window->GetWindowTitle().ToString().data(), nullptr, Window->IsVisible()))
				{
					Window->ToggleVisibility();

					UE_LOG("MainBarWidget: %s 창 토글됨 (현재 상태: %s)",
						Window->GetWindowTitle().ToString().data(),
						Window->IsVisible() ? "표시" : "숨김");
				}
			}

			ImGui::Separator();

			// 전체 창 제어 옵션
			if (ImGui::MenuItem("모든 창 표시"))
			{
				UIManager->ShowAllWindows();
				UE_LOG("MainBarWidget: 모든 창 표시됨");
			}

			if (ImGui::MenuItem("모든 창 숨김"))
			{
				for (auto* Window : UIManager->GetAllUIWindows())
				{
					if (!Window)
					{
						continue;
					}

					if (Window->GetWindowTitle() == "MainMenuBar")
					{
						continue;
					}

					Window->SetWindowState(EUIWindowState::Hidden);
				}

				UE_LOG("MainBarWidget: 모든 창 숨겨짐");
			}
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief View 메뉴를 렌더링하는 함수
 * ViewMode 선택 기능 (Lit, Unlit, Wireframe)
 */
void UMainBarWidget::RenderViewMenu()
{
	if (ImGui::BeginMenu("보기"))
	{
		// GEngine에서 Editor 가져오기
		UEditor* EditorInstance = GEngine->GetEditor();
		if (!EditorInstance)
		{
			ImGui::Text("에디터를 사용할 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		EViewModeIndex CurrentMode = EditorInstance->GetViewMode();

		// ViewMode 메뉴 아이템
		bool bIsLit = (CurrentMode == EViewModeIndex::VMI_Lit);
		bool bIsUnlit = (CurrentMode == EViewModeIndex::VMI_Unlit);
		bool bIsWireframe = (CurrentMode == EViewModeIndex::VMI_Wireframe);

		if (ImGui::MenuItem("조명 적용(Lit)", nullptr, bIsLit) && !bIsLit)
		{
			EditorInstance->SetViewMode(EViewModeIndex::VMI_Lit);
			UE_LOG("MainBarWidget: ViewMode를 Lit으로 변경");
		}

		if (ImGui::MenuItem("조명 비적용(Unlit)", nullptr, bIsUnlit) && !bIsUnlit)
		{
			EditorInstance->SetViewMode(EViewModeIndex::VMI_Unlit);
			UE_LOG("MainBarWidget: ViewMode를 Unlit으로 변경");
		}

		if (ImGui::MenuItem("와이어프레임(Wireframe)", nullptr, bIsWireframe) && !bIsWireframe)
		{
			EditorInstance->SetViewMode(EViewModeIndex::VMI_Wireframe);
			UE_LOG("MainBarWidget: ViewMode를 Wireframe으로 변경");
		}
		if (ImGui::MenuItem("Z버퍼 씬 뎁스(ZBffer Scene Depth)", nullptr, bIsWireframe) && !bIsWireframe)
		{
			EditorInstance->SetViewMode(EViewModeIndex::VMI_SceneDepth2D);
			UE_LOG("MainBarWidget: ViewMode를 Zbuffer SceneDepth로 변경");
		}
		if (ImGui::MenuItem("씬 뎁스(Scene Depth)", nullptr, bIsWireframe) && !bIsWireframe)
		{
			EditorInstance->SetViewMode(EViewModeIndex::VMI_SceneDepth);
			UE_LOG("MainBarWidget: ViewMode를 SceneDepth로 변경");
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief ShowFlags 메뉴를 렌더링하는 함수
 * Static Mesh, BillBoard 등의 플래그 상태 확인 및 토글
 */
void UMainBarWidget::RenderShowFlagsMenu()
{
	if (ImGui::BeginMenu("표시 옵션"))
	{
		// GEngine에서 현재 레벨 가져오기
		// GEngine is now a global pointer
		ULevel* CurrentLevel = GEngine->GetCurrentLevel();
		if (!CurrentLevel)
		{
			ImGui::Text("현재 레벨을 찾을 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// ShowFlags 가져오기
		uint64 ShowFlags = CurrentLevel->GetShowFlags();

		// Primitives 표시 옵션
		bool bShowPrimitives = (ShowFlags & EEngineShowFlags::SF_Primitives) != 0;
		if (ImGui::MenuItem("프리미티브 표시", nullptr, bShowPrimitives))
		{
			if (bShowPrimitives)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Primitives);
				UE_LOG("MainBarWidget: Primitives 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
			CurrentLevel->InitializeActorsInLevel();
		}

		// BillBoard Text 표시 옵션
		bool bShowBillboardText = (ShowFlags & EEngineShowFlags::SF_BillboardText) != 0;
		if (ImGui::MenuItem("빌보드 표시", nullptr, bShowBillboardText))
		{
			if (bShowBillboardText)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_BillboardText);
				UE_LOG("MainBarWidget: 빌보드 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
			CurrentLevel->InitializeActorsInLevel();
		}

		// Bounds 표시 옵션
		bool bShowBounds = (ShowFlags & EEngineShowFlags::SF_Bounds) != 0;
		if (ImGui::MenuItem("바운딩박스 표시", nullptr, bShowBounds))
		{
			if (bShowBounds)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Bounds);
				UE_LOG("MainBarWidget: 바운딩박스 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
			CurrentLevel->InitializeActorsInLevel();
		}

		// Decal 표시 옵션
		bool bShowDecals = (ShowFlags & EEngineShowFlags::SF_Decals) != 0;
		if (ImGui::MenuItem("데칼 표시", nullptr, bShowDecals))
		{
			if (bShowDecals)
			{
				ShowFlags &= ~static_cast<uint64>(EEngineShowFlags::SF_Decals);
				UE_LOG("MainBarWidget: 데칼 비표시");
			}
			else
			{
				ShowFlags |= static_cast<uint64>(EEngineShowFlags::SF_Decals);
				UE_LOG("MainBarWidget: 데칼 표시");
			}
			CurrentLevel->SetShowFlags(ShowFlags);
			CurrentLevel->InitializeActorsInLevel();
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief 그래픽 품질 메뉴를 렌더링하는 함수
 * LOD 레벨을 강제로 고정하는 그래픽 품질 설정
 */
void UMainBarWidget::RenderGraphicsMenu()
{
	if (ImGui::BeginMenu("그래픽"))
	{
		// GEngine에서 현재 레벨 가져오기
		// GEngine is now a global pointer
		ULevel* CurrentLevel = GEngine->GetCurrentLevel();
		if (!CurrentLevel)
		{
			ImGui::Text("현재 레벨을 찾을 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// 그래픽 품질 설정 (LOD 강제 고정)
		// 0: 울트라 (LOD0만 보임), 1: 높음 (자동 LOD), 2: 보통 (LOD1만 보임), 3: 낮음 (LOD2만 보임)
		static int graphicsQuality = 1; // 기본값: 높음 (자동 LOD)

		ImGui::Text("그래픽 품질:");

		if (ImGui::RadioButton("울트라 (최고 품질)", graphicsQuality == 0))
		{
			graphicsQuality = 0;
			CurrentLevel->SetGraphicsQuality(0);
		}

		if (ImGui::RadioButton("높음 (자동 LOD)", graphicsQuality == 1))
		{
			graphicsQuality = 1;
			CurrentLevel->SetGraphicsQuality(1);
		}

		if (ImGui::RadioButton("보통 (중간 품질)", graphicsQuality == 2))
		{
			graphicsQuality = 2;
			CurrentLevel->SetGraphicsQuality(2);
		}

		if (ImGui::RadioButton("낮음 (최저 품질)", graphicsQuality == 3))
		{
			graphicsQuality = 3;
			CurrentLevel->SetGraphicsQuality(3);
		}

		ImGui::Separator();

		// 현재 설정 표시
		switch(graphicsQuality)
		{
			case 0:
				ImGui::Text("현재 설정: 울트라 - LOD0만 표시 (원본 품질)");
				break;
			case 1:
				ImGui::Text("현재 설정: 높음 - 거리 기반 자동 LOD");
				break;
			case 2:
				ImGui::Text("현재 설정: 보통 - LOD1만 표시 (중간 품질)");
				break;
			case 3:
				ImGui::Text("현재 설정: 낮음 - LOD2만 표시 (최저 품질)");
				break;
		}

		ImGui::Separator();

		// FXAA toggle
		{
			URenderer& RendererInstance = URenderer::GetInstance();
			bool bFXAA = RendererInstance.GetFXAAEnabled();
			if (ImGui::Checkbox("FXAA (Fast Approximate AA)", &bFXAA))
			{
				RendererInstance.SetFXAAEnabled(bFXAA);
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(bFXAA ? "Disable screen-space AA (sharper UI)" : "Enable FXAA (reduce edges aliasing)");
			}
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief LOD 메뉴를 렌더링하는 함수
 * LOD 시스템의 활성화/비활성화 및 레벨 설정
 */
void UMainBarWidget::RenderLODMenu()
{
	if (ImGui::BeginMenu("LOD"))
	{
		// GEngine에서 현재 레벨 가져오기
		// GEngine is now a global pointer
		ULevel* CurrentLevel = GEngine->GetCurrentLevel();
		if (!CurrentLevel)
		{
			ImGui::Text("현재 레벨을 찾을 수 없습니다");
			ImGui::EndMenu();
			return;
		}

		// 전역 LOD 활성화/비활성화 토글
		static bool bGlobalLODEnabled = true;
		if (ImGui::MenuItem("LOD 활성화", nullptr, bGlobalLODEnabled))
		{
			bGlobalLODEnabled = !bGlobalLODEnabled;
			CurrentLevel->SetGlobalLODEnabled(bGlobalLODEnabled);
		}

		ImGui::Separator();

		// 최소 LOD 레벨 제한 설정
		static int minLODLevel = 0;
		ImGui::Text("최소 LOD 레벨 제한:");
		if (ImGui::RadioButton("LOD 0,1,2 허용", minLODLevel == 0))
		{
			minLODLevel = 0;
			CurrentLevel->SetMinLODLevel(0);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("LOD 1,2만 허용", minLODLevel == 1))
		{
			minLODLevel = 1;
			CurrentLevel->SetMinLODLevel(1);
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("LOD 2만 허용", minLODLevel == 2))
		{
			minLODLevel = 2;
			CurrentLevel->SetMinLODLevel(2);
		}

		ImGui::Separator();

		// LOD 거리 설정
		static float lodDistance1 = 20.0f;
		static float lodDistance2 = 40.0f;

		ImGui::Text("LOD 전환 거리:");
		if (ImGui::SliderFloat("LOD 1 거리", &lodDistance1, 10.0f, 200.0f, "%.1f"))
		{
			CurrentLevel->SetLODDistance1(lodDistance1);
		}
		if (ImGui::SliderFloat("LOD 2 거리", &lodDistance2, 20.0f, 400.0f, "%.1f"))
		{
			CurrentLevel->SetLODDistance2(lodDistance2);
		}

		// 제곱거리 정보 표시 (성능 최적화 정보)
		ImGui::Text("제곱거리 사용 (성능 최적화):");
		ImGui::Text("LOD 1: %.0f² = %.0f", lodDistance1, lodDistance1 * lodDistance1);
		ImGui::Text("LOD 2: %.0f² = %.0f", lodDistance2, lodDistance2 * lodDistance2);

		ImGui::EndMenu();
	}
}

/**
 * @brief Occlusion Culling 메뉴를 렌더링하는 함수
 * 오클루전 컬링 활성화/비활성화 토글
 */
void UMainBarWidget::RenderOcclusionCullingMenu()
{
	if (ImGui::BeginMenu("Occlusion Culling"))
	{
		URenderer& RendererInstance = URenderer::GetInstance();

		// 오클루전 컬링 활성화/비활성화 토글
		bool bOcclusionCullingEnabled = RendererInstance.GetOcclusionCullingEnabled();
		if (ImGui::MenuItem("Occlusion Culling 활성화", nullptr, bOcclusionCullingEnabled))
		{
			RendererInstance.SetOcclusionCullingEnabled(!bOcclusionCullingEnabled);
			UE_LOG("MainBarWidget: Occlusion Culling %s", !bOcclusionCullingEnabled ? "활성화" : "비활성화");
		}

		ImGui::Separator();

		// 현재 상태 표시
		ImGui::Text("현재 상태: %s", bOcclusionCullingEnabled ? "활성화됨" : "비활성화됨");

		ImGui::EndMenu();
	}
}

/**
 * @brief PIE 메뉴 렌더링 함수
 * Play In Editor 시작/종료 기능 제공
 */
void UMainBarWidget::RenderPIEMenu()
{
	if (!GEngine)
	{
		if (ImGui::BeginMenu("PIE"))
		{
			ImGui::Text("엔진을 사용할 수 없습니다");
			ImGui::EndMenu();
		}
		return;
	}

	bool bIsPIEActive = GEngine->IsPIEActive();

	// PIE 상태에 따라 메뉴 이름 동적 변경
	const char* MenuName = bIsPIEActive ? "PIE (플레이 중)" : "PIE";

	if (ImGui::BeginMenu(MenuName))
	{
		if (!bIsPIEActive)
		{
			// PIE 비활성 상태 - Play 버튼 표시
			if (ImGui::MenuItem("Play In Editor", "F5"))
			{
				GEngine->StartPIE();
				UE_LOG("MainBarWidget: PIE 시작");
			}
		}
		else
		{
			// PIE 활성 상태 - Stop 버튼 표시
			if (ImGui::MenuItem("Stop", "F5"))
			{
				GEngine->EndPIE();
				UE_LOG("MainBarWidget: PIE 종료");
			}
		}

		ImGui::Separator();

		// 현재 상태 표시
		ImGui::Text("상태: %s", bIsPIEActive ? "실행 중" : "중지됨");

		ImGui::EndMenu();
	}
}

/**
 * @brief Help 메뉴에 대한 렌더링 함수
 */
void UMainBarWidget::RenderHelpMenu()
{
	if (ImGui::BeginMenu("도움말"))
	{
		if (ImGui::MenuItem("정보", "F1"))
		{
			UE_LOG("MainBarWidget: 정보 메뉴 선택됨");
			// TODO(KHJ): 정보 다이얼로그 표시
		}

		ImGui::EndMenu();
	}
}

/**
 * @brief 레벨 저장 기능
 */
void UMainBarWidget::SaveCurrentLevel()
{
	path FilePath = OpenSaveFileDialog();
	if (!FilePath.empty())
	{
		// GEngine is now a global pointer

		try
		{
			bool bSuccess = GEngine->SaveCurrentLevel(FilePath.string());

			if (bSuccess)
			{
				UE_LOG("MainMenuBar: SceneIO: Level Saved Successfully");
			}
			else
			{
				UE_LOG("MainMenuBar: SceneIO: Failed To Save Level");
			}
		}
		catch (const exception& Exception)
		{
			FString StatusMessage = FString("Save Error: ") + Exception.what();
			UE_LOG("MainMenuBar: SceneIO: Save Error: %s", Exception.what());
		}
	}
}

/**
 * @brief 레벨 로드 기능
 */
void UMainBarWidget::LoadLevel()
{
	path FilePath = OpenLoadFileDialog();

	if (!FilePath.empty())
	{
		try
		{
			bool bSuccess = GEngine->LoadLevel(FilePath.string());

			if (bSuccess)
			{
				UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 성공했습니다");
			}
			else
			{
				UE_LOG("MainMenuBar: SceneIO: 레벨 로드에 실패했습니다");
			}
		}
		catch (const exception& Exception)
		{
			FString StatusMessage = FString("Load Error: ") + Exception.what();
			UE_LOG("SceneIO: Load Error: %s", Exception.what());
		}
	}
}

/**
 * @brief 새로운 레벨 생성 기능
 */
void UMainBarWidget::CreateNewLevel()
{
	// GEngine is now a global pointer
	if (GEngine->CreateNewLevel())
	{
		UE_LOG("MainBarWidget: 새로운 레벨이 성공적으로 생성되었습니다");
	}
	else
	{
		UE_LOG("MainBarWidget: 새로운 레벨 생성에 실패했습니다");
	}
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * PWSTR: WideStringPointer 클래스
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenSaveFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		IFileSaveDialog* FileSaveDialogPtr = nullptr;

		// 2. FileSaveDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
			IID_IFileSaveDialog, reinterpret_cast<void**>(&FileSaveDialogPtr));

		if (SUCCEEDED(ResultHandle))
		{
			// 3. 대화상자 옵션 설정
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.scene)", L"*.scene"},
				{L"All Files (*.*)", L"*.*"}
			};
			FileSaveDialogPtr->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);

			// 기본 필터를 "Scene Files" 로 설정
			FileSaveDialogPtr->SetFileTypeIndex(1);

			// 기본 확장자 설정
			FileSaveDialogPtr->SetDefaultExtension(L"json");

			// 대화상자 제목 설정
			FileSaveDialogPtr->SetTitle(L"Save Level File");

			// Set Flag
			DWORD DoubleWordFlags;
			FileSaveDialogPtr->GetOptions(&DoubleWordFlags);
			FileSaveDialogPtr->SetOptions(DoubleWordFlags | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);

			// Show Modal
			// 현재 활성 창을 부모로 가짐
			UE_LOG("SceneIO: Save Dialog Modal Opening...");
			ResultHandle = FileSaveDialogPtr->Show(GetActiveWindow());

			// 결과 처리
			// 사용자가 '저장' 을 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileSaveDialogPtr->GetResult(&ShellItemResult);
				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR FilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &FilePath);

					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(FilePath);
						CoTaskMemFree(FilePath);
					}

					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소'를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Save Dialog Modal Closed - 취소됨");
			}

			// Release FileSaveDialog
			FileSaveDialogPtr->Release();
		}

		// COM 라이브러리 해제
		CoUninitialize();
	}

	return ResultPath;
}

/**
 * @brief Windows API를 활용한 파일 로드 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path UMainBarWidget::OpenLoadFileDialog()
{
	path ResultPath = L"";

	// COM 라이브러리 초기화
	HRESULT ResultHandle = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	if (SUCCEEDED(ResultHandle))
	{
		IFileOpenDialog* FileOpenDialog = nullptr;

		// FileOpenDialog 인스턴스 생성
		ResultHandle = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
			IID_IFileOpenDialog, reinterpret_cast<void**>(&FileOpenDialog));

		if (SUCCEEDED(ResultHandle))
		{
			// 파일 타입 필터 설정
			COMDLG_FILTERSPEC SpecificationRange[] = {
				{L"Scene Files (*.scene)", L"*.scene"},
				{L"All Files (*.*)", L"*.*"}
			};

			FileOpenDialog->SetFileTypes(ARRAYSIZE(SpecificationRange), SpecificationRange);

			// 기본 필터를 "Scene Files" 로 설정
			FileOpenDialog->SetFileTypeIndex(1);

			// 대화상자 제목 설정
			FileOpenDialog->SetTitle(L"Load Level File");

			// Flag Setting
			DWORD DoubleWordFlags;
			FileOpenDialog->GetOptions(&DoubleWordFlags);
			FileOpenDialog->SetOptions(DoubleWordFlags | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

			// Open Modal
			UE_LOG("SceneIO: Load Dialog Modal Opening...");
			ResultHandle = FileOpenDialog->Show(GetActiveWindow()); // 현재 활성 창을 부모로

			// 결과 처리
			// 사용자가 '열기' 를 눌렀을 경우
			if (SUCCEEDED(ResultHandle))
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 파일 선택됨");
				IShellItem* ShellItemResult;
				ResultHandle = FileOpenDialog->GetResult(&ShellItemResult);

				if (SUCCEEDED(ResultHandle))
				{
					// Get File Path from IShellItem
					PWSTR ReturnFilePath = nullptr;
					ResultHandle = ShellItemResult->GetDisplayName(SIGDN_FILESYSPATH, &ReturnFilePath);

					if (SUCCEEDED(ResultHandle))
					{
						ResultPath = path(ReturnFilePath);
						CoTaskMemFree(ReturnFilePath);
					}

					ShellItemResult->Release();
				}
			}
			// 사용자가 '취소' 를 눌렀거나 오류 발생
			else
			{
				UE_LOG("SceneIO: Load Dialog Modal Closed - 취소됨");
			}

			FileOpenDialog->Release();
		}

		// COM 라이브러리 해제
		CoUninitialize();
	}

	return ResultPath;
}

