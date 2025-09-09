#include "pch.h"
#include "Render/UI/Window/Public/LevelManangerWindow.h"

#include <commdlg.h>

#include "Manager/Level/Public/LevelManager.h"
#include "Manager/Time/Public/TimeManager.h"

ULevelManagerWindow::ULevelManagerWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Level IO";
	Config.DefaultSize = ImVec2(400, 300);
	Config.DefaultPosition = ImVec2(200, 200);
	Config.MinSize = ImVec2(300, 250);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	Config.DockDirection = EUIDockDirection::Center;

	Config.UpdateWindowFlags();
	SetConfig(Config);

	LevelManager = &ULevelManager::GetInstance();
	StatusMessage = "";
	StatusMessageTimer = 0.0f;
}

void ULevelManagerWindow::Initialize()
{
	// 초기화 로직이 필요하면 여기에 추가
	UE_LOG("[LevelManagerWindow] Initialized");
}

void ULevelManagerWindow::Render()
{
	bool bIsVisible = IsVisible();

	if (!bIsVisible)
	{
		return;
	}

	ImGui::Text("Level Save & Load Manager");
	ImGui::Separator();

	// Save Section
	ImGui::Text("Save Current Level");
	if (ImGui::Button("Save Level", ImVec2(120, 30)))
	{
		path FilePath = OpenSaveFileDialog();
		if (!FilePath.empty())
		{
			SaveLevel(FilePath.string());
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Quick Save", ImVec2(120, 30)))
	{
		// 빠른 저장 - 기본 경로에 저장
		SaveLevel("");
	}

	ImGui::Separator();

	// Load Section
	ImGui::Text("Load Level");
	if (ImGui::Button("Load Level", ImVec2(120, 30)))
	{
		path FilePath = OpenLoadFileDialog();
		if (!FilePath.empty())
		{
			LoadLevel(FilePath.string());
		}
	}

	ImGui::Separator();

	// New Level Section
	ImGui::Text("Create New Level");
	ImGui::InputText("Level Name", NewLevelNameBuffer, sizeof(NewLevelNameBuffer));

	if (ImGui::Button("Create New Level", ImVec2(150, 30)))
	{
		CreateNewLevel();
	}

	ImGui::Separator();

	// Status Message
	if (StatusMessageTimer > 0.0f)
	{
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), StatusMessage.c_str());
		StatusMessageTimer -= UTimeManager::GetInstance().GetDeltaTime();
	}

	// Help Text
	ImGui::Separator();
	ImGui::TextWrapped("Tip: Level Files Are Saved In JSON Format With .json Extension.");
}

/**
 * @brief Windows API를 활용한 파일 저장 Dialog Modal을 생성하는 UI Window 기능
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path ULevelManagerWindow::OpenSaveFileDialog()
{
    OPENFILENAMEW ofn;
    wchar_t szFile[260] = {};

    // 기본 파일명 설정
    wcscpy_s(szFile, L"NewLevel.json");

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
    cout << "[LevelManagerWindow] Opening Save Dialog (Modal)..." << "\n";

    if (GetSaveFileNameW(&ofn) == TRUE)
    {
        cout << "[LevelManagerWindow] Save Dialog Closed - File Selected" << "\n";
        return path(szFile);
    }

    cout << "[LevelManagerWindow] Save Dialog Closed - Cancelled" << "\n";
    return L"";
}

/**
 * @brief Windows API를 사용하여 파일 열기 다이얼로그를 엽니다
 * @return 선택된 파일 경로 (취소 시 빈 문자열)
 */
path ULevelManagerWindow::OpenLoadFileDialog()
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

    // Modal 다이얼로그 표시
    cout << "[LevelManagerWindow] Opening Load Dialog (Modal)..." << "\n";

    if (GetOpenFileNameW(&ofn) == TRUE)
    {
        cout << "[LevelManagerWindow] Load Dialog Closed - File Selected" << "\n";
        return path(szFile);
    }

    cout << "[LevelManagerWindow] Load Dialog Closed - Cancelled" << "\n";
    return L"";
}

/**
 * @brief 지정된 경로에 Level을 Save하는 UI Window 기능
 * @param InFilePath 저장할 파일 경로
 */
void ULevelManagerWindow::SaveLevel(const FString& InFilePath)
{
	try
	{
		bool bSuccess;

		if (InFilePath.empty())
		{
			// Quick Save인 경우 기본 경로 사용
			bSuccess = LevelManager->SaveCurrentLevel("");
		}
		else
		{
			bSuccess = LevelManager->SaveCurrentLevel(InFilePath);
		}

		if (bSuccess)
		{
			StatusMessage = "Level Saved Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Level Saved Successfully" << "\n";
		}
		else
		{
			StatusMessage = "Failed To Save Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Save Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Save Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Save Error: " << Exception.what() << "\n";
	}
}

/**
 * @brief 지정된 경로에서 Level File을 Load 하는 UI Window 기능
 * @param InFilePath 불러올 파일 경로
 */
void ULevelManagerWindow::LoadLevel(const FString& InFilePath)
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

		bool bSuccess = LevelManager->LoadLevel(LevelName, InFilePath);

		if (bSuccess)
		{
			StatusMessage = "Level Loaded Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Level Loaded Successfully" << "\n";
		}
		else
		{
			StatusMessage = "Failed To Load Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Load Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = FString("Load Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Load Error: " << Exception.what() << "\n";
	}
}

/**
 * @brief New Blank Level을 생성하는 UI Window 기능
 */
void ULevelManagerWindow::CreateNewLevel()
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

		bool bSuccess = LevelManager->CreateNewLevel(LevelName);

		if (bSuccess)
		{
			StatusMessage = "New Level Created Successfully!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] New Level Created: " << FString(NewLevelNameBuffer) << "\n";
		}
		else
		{
			StatusMessage = "Failed To Create New Level!";
			StatusMessageTimer = STATUS_MESSAGE_DURATION;
			cout << "[LevelManagerWindow] Failed To Create New Level" << "\n";
		}
	}
	catch (const exception& Exception)
	{
		StatusMessage = std::string("Create Error: ") + Exception.what();
		StatusMessageTimer = STATUS_MESSAGE_DURATION;
		cout << "[LevelManagerWindow] Create Error: " << Exception.what() << "\n";
	}
}
