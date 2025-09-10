#include "pch.h"
#include "Render/UI/Widget/Public/SceneIOWidget.h"

#include "Manager/Level/Public/LevelManager.h"

USceneIOWidget::USceneIOWidget()
	:UWidget("Scene IO Widget")
{
}

USceneIOWidget::~USceneIOWidget() = default;

void USceneIOWidget::Initialize()
{
}

void USceneIOWidget::Update()
{
}

void USceneIOWidget::RenderWidget()
{
	// Save Section
	ImGui::Text("Scene Save & Load");
	ImGui::Spacing();

	if (ImGui::Button("Save Scene", ImVec2(90, 20)))
	{
		path FilePath = OpenSaveFileDialog();
		if (!FilePath.empty())
		{
			SaveLevel(FilePath.string());
		}
	}

	// ImGui::SameLine();
	// if (ImGui::Button("Quick Save", ImVec2(120, 30)))
	// {
	// 	// 빠른 저장 - 기본 경로에 저장
	// 	SaveLevel("");
	// }

	// Load Section
	ImGui::SameLine();
	if (ImGui::Button("Load Scene", ImVec2(90, 20)))
	{
		path FilePath = OpenLoadFileDialog();
		if (!FilePath.empty())
		{
			LoadLevel(FilePath.string());
		}
	}

	ImGui::Spacing();

	// New Level Section
	ImGui::Text("새로운 Scene 생성");
	ImGui::Spacing();

	ImGui::InputText("Level Name", NewLevelNameBuffer, sizeof(NewLevelNameBuffer));
	if (ImGui::Button("Create New Scene", ImVec2(120, 25)))
	{
		CreateNewLevel();
	}

	// Status Message
	if (StatusMessageTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), StatusMessage.c_str());
		StatusMessageTimer -= DT;
	}
	// Reserve Space
	else
	{
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	}

	ImGui::Separator();
}

/**
 * @brief 지정된 경로에 Level을 Save하는 UI Window 기능
 * @param InFilePath 저장할 파일 경로
 */
void USceneIOWidget::SaveLevel(const FString& InFilePath)
{
	ULevelManager& LevelManager = ULevelManager::GetInstance();

	try
	{
		bool bSuccess;

		if (InFilePath.empty())
		{
			// Quick Save인 경우 기본 경로 사용
			bSuccess = LevelManager.SaveCurrentLevel("");
		}
		else
		{
			bSuccess = LevelManager.SaveCurrentLevel(InFilePath);
		}

		if (bSuccess)
		{
			StatusMessage = "Level Saved Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Level Saved Successfully");
		}
		else
		{
			StatusMessage = "Failed To Save Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Failed To Save Level");
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Save Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Save Error: %s", Exception.what());
	}
}

/**
 * @brief 지정된 경로에서 Level File을 Load 하는 UI Window 기능
 * @param InFilePath 불러올 파일 경로
 */
void USceneIOWidget::LoadLevel(const FString& InFilePath)
{
	try
	{
		// 파일명에서 확장자를 제외하고 레벨 이름 추출
		FString LevelName = InFilePath;

		size_t LastSlash = LevelName.find_last_of("\\/");
		if (LastSlash != std::wstring::npos)
		{
			LevelName = LevelName.substr(LastSlash + 1);
		}

		size_t LastDot = LevelName.find_last_of(".");
		if (LastDot != std::wstring::npos)
		{
			LevelName = LevelName.substr(0, LastDot);
		}

		ULevelManager& LevelManager = ULevelManager::GetInstance();
		bool bSuccess = LevelManager.LoadLevel(LevelName, InFilePath);

		if (bSuccess)
		{
			StatusMessage = "레벨을 성공적으로 로드했습니다";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
		}
		else
		{
			StatusMessage = "레벨을 로드하는 데에 실패했습니다";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
		}

		UE_LOG("SceneIO: %s", StatusMessage.c_str());
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Load Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Load Error: %s",Exception.what());
	}
}

/**
 * @brief New Blank Level을 생성하는 UI Window 기능
 */
void USceneIOWidget::CreateNewLevel()
{
	try
	{
		FString LevelName = FString(NewLevelNameBuffer, NewLevelNameBuffer + strlen(NewLevelNameBuffer));

		if (LevelName.empty())
		{
			StatusMessage = "Please Enter A Level Name!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			return;
		}

		ULevelManager& LevelManager = ULevelManager::GetInstance();
		bool bSuccess = LevelManager.CreateNewLevel(LevelName);

		if (bSuccess)
		{
			StatusMessage = "New Level Created Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: New Level Created: %s", FString(NewLevelNameBuffer).c_str());
		}
		else
		{
			StatusMessage = "Failed To Create New Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			UE_LOG("SceneIO: Failed To Create New Level");
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = std::string("Create Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		UE_LOG("SceneIO: Create Error: ");
	}
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path USceneIOWidget::OpenSaveFileDialog()
{
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {};

    // 기본 파일명 설정
    wcscpy_s(szFile, L"");

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();  // 현재 활성 윈도우를 부모로 설정
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Save Level File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"json";

    // Modal 다이얼로그 표시 - 이 함수가 리턴될 때까지 다른 입력 차단
    UE_LOG("SceneIO: Opening Save Dialog (Modal)...");

    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        UE_LOG("SceneIO: Save Dialog Closed - 파일 선택됨");
        return path(szFile);
    }

   UE_LOG("SceneIO: Save Dialog Closed - 취소됨");
    return L"";
}

/**
 * @brief Windows API를 사용하여 파일 열기 다이얼로그를 엽니다
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path USceneIOWidget::OpenLoadFileDialog()
{
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {};

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();  // 현재 활성 윈도우를 부모로 설정
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"JSON Files\0*.json\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Load Level File";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_HIDEREADONLY;

    UE_LOG("SceneIO: Opening Load Dialog (Modal)...");

    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        UE_LOG("SceneIO: Load Dialog Closed - 파일 선택됨");
        return path(szFile);
    }

    UE_LOG("SceneIO: Load Dialog Closed - 취소됨");
    return L"";
}
