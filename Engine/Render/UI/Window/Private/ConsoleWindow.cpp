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
	AddLog(ELogType::Success, "ConsoleWindow: Game Console 초기화 성공");
	AddLog(ELogType::System, "ConsoleWindow: Logging System Ready");

	// Initialize System Output Redirection
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
		for (const auto& LogEntry : LogItems)
		{
			// ELogType을 기반으로 색상 결정
			ImVec4 Color = GetColorByLogType(LogEntry.Type);
			bool bShouldApplyColor = (LogEntry.Type != ELogType::Info);

			if (bShouldApplyColor)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Color);
			}

			ImGui::TextUnformatted(LogEntry.Message.c_str());

			if (bShouldApplyColor)
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

/**
 * @brief ELogType에 따른 색상 반환
 */
ImVec4 UConsoleWindow::GetColorByLogType(ELogType InType)
{
	switch (InType)
	{
	case ELogType::Info:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 흰색
	case ELogType::Warning:
		return {1.0f, 1.0f, 0.4f, 1.0f}; // 노란색
	case ELogType::Error:
	case ELogType::TerminalError:
		return {1.0f, 0.4f, 0.4f, 1.0f}; // 빨간색
	case ELogType::Success:
	case ELogType::UELog:
		return {0.4f, 1.0f, 0.4f, 1.0f}; // 초록색
	case ELogType::System:
		return {0.7f, 0.7f, 0.7f, 1.0f}; // 회색
	case ELogType::Debug:
		return {0.4f, 0.6f, 1.0f, 1.0f}; // 파란색
	case ELogType::Terminal:
		return {0.6f, 0.8f, 1.0f, 1.0f}; // 하늘색
	case ELogType::Command:
		return {1.0f, 0.8f, 0.6f, 1.0f}; // 주황색
	default:
		return {1.0f, 1.0f, 1.0f, 1.0f}; // 기본 흰색
	}
}

/**
 * @brief 기본 로그 추가 함수
 */
void UConsoleWindow::AddLog(const char* fmt, ...)
{
	char buf[1024];

	// Variable Argument List 접근
	va_list args;
	va_start(args, fmt);

	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;

	va_end(args);

	// 기본 Info 타입으로 로그 추가
	FLogEntry LogEntry;
	LogEntry.Type = ELogType::Info;
	LogEntry.Message = FString(buf);
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

/**
 * @brief 타입 특정 로그 추가 함수
 */
void UConsoleWindow::AddLog(ELogType InType, const char* fmt, ...)
{
	char buf[1024];

	// Variable Argument List 접근
	va_list args;
	va_start(args, fmt);

	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	buf[sizeof(buf) - 1] = 0;

	va_end(args);

	// 지정된 타입으로 로그 추가
	FLogEntry LogEntry;
	LogEntry.Type = InType;
	LogEntry.Message = FString(buf);
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}

void UConsoleWindow::ProcessCommand(const char* InCommand)
{
	if (!InCommand || strlen(InCommand) == 0)
	{
		return;
	}

	// 명령 히스토리에 추가
	CommandHistory.push_back(FString(InCommand));
	HistoryPosition = -1;

	FString Input = InCommand;

	// UE_LOG( 우선 탐색
	size_t StartPosition = Input.find("UE_LOG(");
	if (StartPosition != FString::npos && StartPosition == 0)
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
								FLogEntry LogEntry;
								LogEntry.Type = ELogType::UELog;
								LogEntry.Message = FString(Buffer);
								LogItems.push_back(LogEntry);
								bIsScrollToBottom = true;
							}
							catch (...)
							{
								FLogEntry ErrorEntry;
								ErrorEntry.Type = ELogType::Error;
								ErrorEntry.Message = "Failed To Parse Integer: " + RemainingArguments;
								LogItems.push_back(ErrorEntry);
								bIsScrollToBottom = true;
							}
						}
					else if (FormatString.find("%s") != FString::npos)
					{
						// Remove quotes from string argument
						FString StringArg = RemainingArguments;
						if (StringArg.length() >= 2 && StringArg.front() == '"' && StringArg.back() == '"')
						{
							StringArg = StringArg.substr(1, StringArg.length() - 2);
						}
						char Buffer[512];
						(void)snprintf(Buffer, sizeof(Buffer), FormatString.c_str(),
						               StringArg.c_str());
						FLogEntry UELogEntry;
						UELogEntry.Type = ELogType::UELog;
						UELogEntry.Message = FString(Buffer);
						LogItems.push_back(UELogEntry);
						bIsScrollToBottom = true;
					}
						else
						{
							FLogEntry UELogEntry;
							UELogEntry.Type = ELogType::UELog;
							UELogEntry.Message = FormatString + RemainingArguments;
							LogItems.push_back(UELogEntry);
							bIsScrollToBottom = true;
						}
					}
					else
					{
						// 인수 없는 경우
						FLogEntry UELogEntry;
						UELogEntry.Type = ELogType::UELog;
						UELogEntry.Message = FormatString;
						LogItems.push_back(UELogEntry);
						bIsScrollToBottom = true;
					}
				}
				else
				{
					AddLog(ELogType::Error, "UELog: Error: Invalid Format: 문자열 종료 따옴표가 없습니다");
					bIsScrollToBottom = true;
				}
			}
			else
			{
				AddLog(ELogType::Error, "UELog: Error: Invalid Format: 첫 인자는 반드시 문자열이어야 합니다");
				bIsScrollToBottom = true;
			}
		}
		else
		{
			AddLog(ELogType::Error, "UELog: Error: Invalid Format: 맨 뒤 괄호가 존재하지 않습니다");
			bIsScrollToBottom = true;
		}
	}

	// Clear 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "clear")
	{
		ClearLog();
	}

	// Help 명령어 입력
	else if (FString CommandLower = InCommand;
		std::transform(CommandLower.begin(), CommandLower.end(), CommandLower.begin(), ::tolower),
		CommandLower == "help")
	{
		AddLog(ELogType::System, "Available Commands:");
		AddLog(ELogType::Info, "  CLEAR - Clear The Console");
		AddLog(ELogType::Info, "  HELP - Show This Help");
		AddLog(ELogType::Info, "  UE_LOG(\"String with format\", Args...) - Log With printf Formatting");
		AddLog(ELogType::Debug, "    Example: UE_LOG(\"Hello World %%d\", 2025)");
		AddLog(ELogType::Debug, "    Example: UE_LOG(\"User: %%s\", \"John\")");
		AddLog(ELogType::Info, "");
		AddLog(ELogType::System, "Terminal Commands:");
		AddLog(ELogType::Info, "  dir, ls - List directory contents");
		AddLog(ELogType::Info, "  cd [path] - Change directory");
		AddLog(ELogType::Info, "  echo [text] - Display text");
		AddLog(ELogType::Info, "  type [file] - Display file contents");
		AddLog(ELogType::Info, "  ping [host] - Network ping");
		AddLog(ELogType::Info, "  Any Windows command will be executed directly");
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

		AddLog(ELogType::System, "ConsoleWindow: Console Output Redirection Initialized");
	}
	catch (...)
	{
		AddLog(ELogType::Error, "ConsoleWindow: Failed To Initialize Console Output Redirection");
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
	ELogType LogType;

	if (bInIsError)
	{
		LogText = "[Error] " + FString(InText);
		LogType = ELogType::Error;
	}
	else
	{
		LogText = "[System] " + FString(InText);
		LogType = ELogType::System;
	}

	// 끝에 있는 개행 문자 제거
	if (!LogText.empty() && LogText.back() == '\n')
	{
		LogText.pop_back();
	}

	FLogEntry LogEntry;
	LogEntry.Type = LogType;
	LogEntry.Message = LogText;
	LogItems.push_back(LogEntry);

	// Auto Scroll
	bIsScrollToBottom = true;
}
