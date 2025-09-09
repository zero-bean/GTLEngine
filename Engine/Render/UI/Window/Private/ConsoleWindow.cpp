#include "pch.h"
#include "Render/UI/Window/Public/ConsoleWindow.h"
#include <sstream>
#include <iostream>
#include <cstdio>

IMPLEMENT_SINGLETON(UConsoleWindow)

UConsoleWindow::~UConsoleWindow()
{
	CleanupSystemRedirect();
	ClearLog();
}

UConsoleWindow::UConsoleWindow(const FUIWindowConfig& InConfig)
	: UUIWindow(InConfig)
{
	// 콘솔 윈도우 기본 설정
	FUIWindowConfig Config = InConfig;
	Config.WindowTitle = "Game Console";
	Config.DefaultSize = ImVec2(520, 600);
	Config.DefaultPosition = ImVec2(100, 100);
	Config.MinSize = ImVec2(400, 300);
	Config.bResizable = true;
	Config.bMovable = true;
	Config.bCollapsible = true;
	SetConfig(Config);

	// 초기화
	ClearLog();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPosition = -1;
	bIsAutoScroll = true;
	bIsScrollToBottom = false;

	// Stream Redirection 초기화
	ConsoleOutputBuffer = nullptr;
	ConsoleErrorBuffer = nullptr;
	OriginalConsoleOutput = nullptr;
	OriginalConsoleError = nullptr;

	SetName(Config.WindowTitle);
}

// ConsoleStreamBuffer implementation
int ConsoleStreamBuffer::overflow(int InCharacter)
{
	if (InCharacter != EOF)
	{
		Buffer += static_cast<char>(InCharacter);
		if (InCharacter == '\n')
		{
			if (Console)
			{
				Console->AddSystemLog(Buffer.c_str(), bIsError);
			}
			Buffer.clear();
		}
	}
	return InCharacter;
}

streamsize ConsoleStreamBuffer::xsputn(const char* InString, streamsize InCount)
{
	for (std::streamsize i = 0; i < InCount; ++i)
	{
		overflow(InString[i]);
	}
	return InCount;
}

void UConsoleWindow::Initialize()
{
	AddLog("Game Console Initialized - Logging System Ready");

	// Initialize system output redirection
	InitializeSystemRedirect();
}

void UConsoleWindow::Render()
{
	// 제어 버튼들
	if (ImGui::Button("Clear"))
	{
		ClearLog();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Auto-Scroll", &bIsAutoScroll);

	ImGui::Separator();

	// 로그 출력 영역 미리 예약
	const float ReservedHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("LogOutput", ImVec2(0, -ReservedHeight), ImGuiChildFlags_NavFlattened,
	                      ImGuiWindowFlags_HorizontalScrollbar))
	{
		// 로그 리스트 출력
		for (const auto& Log : LogItems)
		{
			// 색상 지정
			ImVec4 Color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // 기본 하양
			bool HasColor = false;

			if (Log.find("[error]") != FString::npos || Log.find("[ERROR]") != FString::npos)
			{
				Color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
				HasColor = true;
			}
			else if (Log.find("[warning]") != FString::npos || Log.find("[WARNING]") != FString::npos)
			{
				Color = ImVec4(1.0f, 1.0f, 0.4f, 1.0f); // Yellow
				HasColor = true;
			}
			else if (Log.find("[UE_LOG]") != FString::npos)
			{
				Color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green
				HasColor = true;
			}
			else if (Log.find("[Terminal]") != FString::npos)
			{
				Color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); // Light Blue
				HasColor = true;
			}
			else if (Log.find("[Terminal Error]") != FString::npos)
			{
				Color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red
				HasColor = true;
			}
			else if (Log.find("# ") == 0)
			{
				Color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); // Orange
				HasColor = true;
			}

			if (HasColor)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			}

			ImGui::TextUnformatted(Log.c_str());

			if (HasColor)
			{
				ImGui::PopStyleColor();
			}
		}

		// Auto Scroll
		if (bIsScrollToBottom || (bIsAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
		{
			ImGui::SetScrollHereY(1.0f);
		}
		bIsScrollToBottom = false;
	}

	ImGui::EndChild();
	ImGui::Separator();

	// Input Command
	bool ReclaimFocus = false;
	ImGuiInputTextFlags InputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll;
	if (ImGui::InputText("Input", InputBuf, sizeof(InputBuf), InputFlags))
	{
		if (strlen(InputBuf) > 0)
		{
			ProcessCommand(InputBuf);
			strcpy_s(InputBuf, sizeof(InputBuf), "");
			ReclaimFocus = true;
		}
	}

	// 입력 포커스 설정
	ImGui::SetItemDefaultFocus();
	if (ReclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1);
	}
}

void UConsoleWindow::Update()
{
	// 필요한 경우 여기에 업데이트 로직 추가할 것
	// ImGui 위주라서 필요하지 않을 것으로 보이긴 함...
}

void UConsoleWindow::Cleanup()
{
	CleanupSystemRedirect();
	ClearLog();
}


void UConsoleWindow::ClearLog()
{
	LogItems.clear();
}

void UConsoleWindow::AddLog(const char* fmt, ...)
{
	char buf[1024];

	// Variable Argument List 접근
	va_list args;
	va_start(args, fmt);

	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;

	va_end(args);

	LogItems.push_back(FString(buf));

	// Auto Scroll
	bIsScrollToBottom = true;
}

void UConsoleWindow::ProcessCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
		return;

	// 명령어를 로그에 표시
	// AddLog("# %s", InCommand);

	// 명령 히스토리에 추가
	CommandHistory.push_back(FString(InCommand));
	HistoryPosition = -1;

	// UE_LOG( 구문 처리 먼저 확인
	if (strncmp(InCommand, "UE_LOG(", 7) == 0)
	{
		// UE_LOG("내용", 인수...) 형태 파싱
		FString Input = InCommand;

		// UE_LOG( 찾기
		size_t StartPosition = Input.find("UE_LOG(");
		if (StartPosition != FString::npos)
		{
			StartPosition += 7; // "UE_LOG(" 길이
			size_t EndPosition = Input.rfind(')');

			if (EndPosition != FString::npos && EndPosition > StartPosition)
			{
				FString Arguments = Input.substr(StartPosition, EndPosition - StartPosition);

				// 첫 번째 인수는 문자열 (" 로 시작)
				if (!Arguments.empty() && Arguments[0] == '"')
				{
					size_t FirstQuoteEnd = Arguments.find('"', 1);
					if (FirstQuoteEnd != FString::npos)
					{
						FString FormatString = Arguments.substr(1, FirstQuoteEnd - 1);

						// 뒤쪽에 있는 인수들 처리
						FString RemainingArguments;
						if (FirstQuoteEnd + 1 < Arguments.length())
						{
							RemainingArguments = Arguments.substr(FirstQuoteEnd + 1);
							// 앞의 콤마와 공백 제거
							while (!RemainingArguments.empty() &&
								(RemainingArguments[0] == ',' || RemainingArguments[0] == ' '))
								RemainingArguments = RemainingArguments.substr(1);
						}

						// printf 스타일 처리
						if (!RemainingArguments.empty())
						{
							if (FormatString.find("%d") != FString::npos)
							{
								try
								{
									int Value = std::stoi(RemainingArguments);
									char Buffer[512];
									(void)snprintf(Buffer, sizeof(Buffer), FormatString.c_str(), Value);
									// Prevent Recursive
									LogItems.push_back(u8"[UE_LOG] " + FString(Buffer));
									bIsScrollToBottom = true;
								}
								catch (...)
								{
									LogItems.push_back(u8"[UE_LOG] Failed To Parse Integer: " + RemainingArguments);
									bIsScrollToBottom = true;
								}
							}
							else if (FormatString.find("%s") != FString::npos)
							{
								char Buffer[512];
								(void)snprintf(Buffer, sizeof(Buffer), FormatString.c_str(),
								               RemainingArguments.c_str());
								LogItems.push_back(u8"[UE_LOG] " + FString(Buffer));
								bIsScrollToBottom = true;
							}
							else
							{
								LogItems.push_back(u8"[UE_LOG] " + FormatString + " " + RemainingArguments);
								bIsScrollToBottom = true;
							}
						}
						else
						{
							// 인수 없는 경우
							LogItems.push_back(u8"[UE_LOG] " + FormatString);
							bIsScrollToBottom = true;
						}
					}
					else
					{
						LogItems.emplace_back(u8"[Error] Invalid UE_LOG Format - Missing Closing Quote");
						bIsScrollToBottom = true;
					}
				}
				else
				{
					LogItems.emplace_back(u8"[Error] Invalid UE_LOG Format - First Argument Must Be A String");
					bIsScrollToBottom = true;
				}
			}
			else
			{
				LogItems.emplace_back(u8"[Error] Invalid UE_LOG Format - Missing Closing Parenthesis");
				bIsScrollToBottom = true;
			}
		}
	}
	// 대소문자 구분없이 비교하기 위해 소문자로 변환
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "clear")
	{
		ClearLog();
	}
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "help")
	{
		AddLog("Available Commands:");
		AddLog("  CLEAR - Clear The Console");
		AddLog("  HELP - Show This Help");
		AddLog("  UE_LOG(\"String With Format\", Args...) - Log With Printf Formatting");
		AddLog("    Example: UE_LOG(\"Hello World %%d\", 2025)");
		AddLog("    Example: UE_LOG(\"User: %%s\", \"John\")");
		AddLog("");
		AddLog("Terminal Commands:");
		AddLog("  dir, ls - List directory contents");
		AddLog("  cd [path] - Change directory");
		AddLog("  echo [text] - Display text");
		AddLog("  type [file] - Display file contents");
		AddLog("  ping [host] - Network ping");
		AddLog("  Any Windows command will be executed directly");
	}
	else if (strncmp(InCommand, "ue_log", 6) == 0)
	{
		FString Input = InCommand;
		size_t UELogPosition = Input.find("ue_log");
		if (UELogPosition != FString::npos)
		{
			// "ue_log " 다음 부분 추출
			FString Arguments = Input.substr(UELogPosition + 7); // "ue_log " 길이는 7

			// 앞에 있는 공백 제거
			while (!Arguments.empty() && Arguments[0] == ' ')
				Arguments = Arguments.substr(1);

			if (!Arguments.empty())
			{
				// printf 방식으로 처리하기 위해 간단한 파싱
				// 첫 번째 공백까지를 format string으로 처리
				size_t FirstSpace = Arguments.find(' ');

				if (FirstSpace != FString::npos)
				{
					FString FormatString = Arguments.substr(0, FirstSpace);
					FString Remaining = Arguments.substr(FirstSpace + 1);

					// 간단한 %d 처리만 지원
					if (FormatString.find("%d") != FString::npos)
					{
						try
						{
							int Value = std::stoi(Remaining);
							char Buffer[512];
							(void)snprintf(Buffer, sizeof(Buffer), FormatString.c_str(), Value);
							AddLog("[UE_LOG] %s", Buffer);
						}
						catch (...)
						{
							AddLog("[UE_LOG] %s (failed to parse: %s)", FormatString.c_str(), Remaining.c_str());
						}
					}
					else if (FormatString.find("%s") != FString::npos)
					{
						char Buffer[512];
						(void)snprintf(Buffer, sizeof(Buffer), FormatString.c_str(), Remaining.c_str());
						AddLog("[UE_LOG] %s", Buffer);
					}
					else
					{
						AddLog("[UE_LOG] %s %s", FormatString.c_str(), Remaining.c_str());
					}
				}
				else
				{
					// 인수가 없는 경우
					AddLog("[UE_LOG] %s", Arguments.c_str());
				}
			}
			else
			{
				AddLog("Usage: ue_log \"format string\" [args...]");
			}
		}
	}
	else
	{
		// 실제 터미널 명령어 실행
		ExecuteTerminalCommand(InCommand);
	}

	// 스크롤 하단으로 이동
	bIsScrollToBottom = true;
}

/**
 * @brief 실제 터미널 명령어를 실행하고 결과를 콘솔에 표시하는 함수
 * @param InCommand 실행할 터미널 명령어
 */
void UConsoleWindow::ExecuteTerminalCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	AddLog("[Terminal] > %s", InCommand);

	try
	{
		FString FullCommand = "powershell /c " + FString(InCommand);

		// Prepare Pipe for Create Process
		HANDLE PipeReadHandle, PipeWriteHandle;

		SECURITY_ATTRIBUTES SecurityAttribute = {};
		SecurityAttribute.nLength = sizeof(SECURITY_ATTRIBUTES);
		SecurityAttribute.bInheritHandle = TRUE;
		SecurityAttribute.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(&PipeReadHandle, &PipeWriteHandle, &SecurityAttribute, 0))
		{
			AddLog("[Terminal Error] Failed To Create Pipe For Command Execution.");
			return;
		}

		// Set STARTUPINFO
		STARTUPINFOA StartUpInfo = {};
		StartUpInfo.cb = sizeof(STARTUPINFOA);
		StartUpInfo.dwFlags |= STARTF_USESTDHANDLES;
		StartUpInfo.hStdInput = nullptr;

		// stderr & stdout을 동일 파이프에서 처리
		StartUpInfo.hStdError = PipeWriteHandle;
		StartUpInfo.hStdOutput = PipeWriteHandle;

		PROCESS_INFORMATION ProcessInformation = {};

		// lpCommandLine Param Setting
		TArray<char> CommandLine(FullCommand.begin(), FullCommand.end());
		CommandLine.push_back('\0');

		// Create Process
		if (!CreateProcessA(nullptr, CommandLine.data(), nullptr,
		                    nullptr, TRUE, CREATE_NO_WINDOW,
		                    nullptr, nullptr, &StartUpInfo, &ProcessInformation))
		{
			AddLog("[Terminal Error] Failed To Execute Command: %s", InCommand);
			CloseHandle(PipeReadHandle);
			CloseHandle(PipeWriteHandle);
			return;
		}

		// 부모 프로세스에서는 쓰기 핸들이 불필요하므로 바로 닫도록 처리
		CloseHandle(PipeWriteHandle);

		// Read Execution Process
		char Buffer[256];
		DWORD ReadDoubleWord;
		FString Output;
		bool bHasOutput = false;

		while (ReadFile(PipeReadHandle, Buffer, sizeof(Buffer) - 1, &ReadDoubleWord, nullptr) && ReadDoubleWord != 0)
		{
			Buffer[ReadDoubleWord] = '\0'; // Null 문자로 String 끝 세팅
			Output += Buffer;
			bHasOutput = true;
		}

		// Process 종료 대기
		WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

		// 종료 코드 확인
		DWORD ExitCode;
		GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);

		// Release Handle
		CloseHandle(PipeReadHandle);
		CloseHandle(ProcessInformation.hProcess);
		CloseHandle(ProcessInformation.hThread);

		// Print Result
		if (bHasOutput)
		{
			FString Utf8Output = ConvertCP949ToUTF8(Output.c_str());

			std::istringstream Stream(Utf8Output);
			FString Line;
			while (std::getline(Stream, Line))
			{
				// istringstream이 \r을 남길 수 있으므로 제거
				if (!Line.empty() && Line.back() == '\r')
				{
					Line.pop_back();
				}
				if (!Line.empty())
				{
					AddLog("%s", Line.c_str());
				}
			}
		}
		else if (ExitCode == 0)
		{
			// 출력은 없지만 성공적으로 실행된 경우
			AddLog("[Terminal] Command executed successfully (no output)");
		}

		// 에러 코드가 있는 경우 표시
		if (ExitCode != 0)
		{
			AddLog("[Terminal Error] Command failed with exit code: %d", ExitCode);
		}
	}
	catch (const exception& Exception)
	{
		AddLog("[Terminal Error] Exception occurred: %s", Exception.what());
	}
	catch (...)
	{
		AddLog("[Terminal Error] Unknown Error Occurred While Executing Command");
	}
}

// System output redirection implementation
void UConsoleWindow::InitializeSystemRedirect()
{
	try
	{
		// Save Original Stream Buffers
		OriginalConsoleOutput = cout.rdbuf();
		OriginalConsoleError = cerr.rdbuf();

		// Create Custom Stream Buffers
		ConsoleOutputBuffer = new ConsoleStreamBuffer(this, false);
		ConsoleErrorBuffer = new ConsoleStreamBuffer(this, true);

		// Redirect ConsoleOutput and ConsoleError To Our Custom Buffers
		cout.rdbuf(ConsoleOutputBuffer);
		cerr.rdbuf(ConsoleErrorBuffer);

		AddLog("[System] Console Output Redirection Initialized");
	}
	catch (...)
	{
		AddLog("[error] Failed To Initialize Console Output Redirection");
	}
}

void UConsoleWindow::CleanupSystemRedirect()
{
	try
	{
		// Restore Original Stream Buffers
		if (OriginalConsoleOutput)
		{
			cout.rdbuf(OriginalConsoleOutput);
			OriginalConsoleOutput = nullptr;
		}

		if (OriginalConsoleError)
		{
			cerr.rdbuf(OriginalConsoleError);
			OriginalConsoleError = nullptr;
		}

		// Delete Custom Stream Buffers
		if (ConsoleOutputBuffer)
		{
			delete ConsoleOutputBuffer;
			ConsoleOutputBuffer = nullptr;
		}

		if (ConsoleErrorBuffer)
		{
			delete ConsoleErrorBuffer;
			ConsoleErrorBuffer = nullptr;
		}
	}
	catch (...)
	{
		// Ignore Cleanup Errors
	}
}

void UConsoleWindow::AddSystemLog(const char* InText, bool bInIsError)
{
	if (!InText || strlen(InText) == 0)
	{
		return;
	}

	FString LogText;
	if (bInIsError)
	{
		LogText = "[Error] " + FString(InText);
	}
	else
	{
		LogText = "[System] " + FString(InText);
	}

	// 끝에 있는 개행 문자 제거
	if (!LogText.empty() && LogText.back() == '\n')
	{
		LogText.pop_back();
	}

	LogItems.push_back(LogText);

	// Auto Scroll
	bIsScrollToBottom = true;
}
