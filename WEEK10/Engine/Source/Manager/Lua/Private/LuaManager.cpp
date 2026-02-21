#include "pch.h"
#include "Manager/Lua/Public/LuaManager.h"
#include "Manager/Lua/Public/LuaBinder.h"
#include "Manager/Path/Public/PathManager.h"

IMPLEMENT_SINGLETON_CLASS(ULuaManager, UObject)

ULuaManager::ULuaManager()
{
}

ULuaManager::~ULuaManager()
{

}

void ULuaManager::Initialize()
{
    MasterLuaState.open_libraries(
		sol::lib::base,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::io,
		sol::lib::math,
		sol::lib::table);
    MasterLuaState.script("print('--- [LuaManager] sol2 & Lua link SUCCESS! ---')");

    BindTypesToLua();
    LoadAllLuaScripts();
}

void ULuaManager::Update(float DeltaTime)
{
    HotReloadTimer += DeltaTime;
    if (HotReloadTimer >= HotReloadInterval)
    {
        HotReloadTimer = 0.0f;
        CheckForHotReload();
    }
}

void ULuaManager::BindTypesToLua()
{
	FLuaBinder::BindCoreTypes(MasterLuaState);
	FLuaBinder::BindMathTypes(MasterLuaState);
	FLuaBinder::BindActorTypes(MasterLuaState);
	FLuaBinder::BindComponentTypes(MasterLuaState);
	FLuaBinder::BindCoreFunctions(MasterLuaState);
}

void ULuaManager::LoadAllLuaScripts()
{
    LuaScriptPath = UPathManager::GetInstance().GetLuaScriptPath();
    LuaScriptCaches.Empty();

    if (!exists(LuaScriptPath) || !filesystem::is_directory(LuaScriptPath))
    {
        UE_LOG("[LuaManager] Lua 스크립트 경로를 찾을 수 없거나 디렉터리가 아닙니다: %s", LuaScriptPath.string().c_str());
        return;
    }

    for (const auto& Entry : filesystem::recursive_directory_iterator(LuaScriptPath))
    {
        if (Entry.is_regular_file() && Entry.path().extension() == ".lua")
        {
            FString FileName = Entry.path().filename().stem().string();
            FString FullPath = Entry.path().string();
            filesystem::file_time_type LastModifiedTime = filesystem::last_write_time(Entry);

            if (LuaScriptCaches.Contains(FileName))
            {
                FString AlreadyExistFile = LuaScriptCaches[FileName].FullPath.string();
                UE_LOG_ERROR("[LuaManager] %s 경로에 있는 Lua 스크립트는 이미 %s 경로로 등록되어있습니다! (파일명 중복)", AlreadyExistFile.c_str(), FullPath.c_str());
                continue;
            }
            LuaScriptCaches[FileName] = FLuaScriptInfo(FullPath, LastModifiedTime);

            if (FileName == "Template" || FileName == "template")
            {
                LuaTemplatePath = FullPath;
            }
            else if (FileName == "Utility" || FileName == "utility")
            {
            	MasterLuaState.script_file(FullPath);
            }
            else if (FileName == "Sound" || FileName == "sound")
            {
                // Auto-run global sound hooks (BGM on StartGame, off on EndGame)
                MasterLuaState.script_file(FullPath);
            }
        }
    }
}

sol::environment ULuaManager::LoadLuaEnvironment(UScriptComponent* ScriptComponent, const FName& LuaScriptName)
{
    if (LuaScriptCaches.Contains(LuaScriptName))
    {
        sol::environment Env(MasterLuaState, sol::create, MasterLuaState.globals());
        MasterLuaState.script_file(LuaScriptCaches[LuaScriptName].FullPath.string(), Env);
        LuaScriptCaches[LuaScriptName].ScriptComponents.Add(ScriptComponent);
        return Env;
    }
    return sol::environment{};
}

sol::environment ULuaManager::CreateLuaEnvironment(UScriptComponent* ScriptComponent, const FName& LuaScriptName)
{
    FString ScriptFileNameStr = LuaScriptName.ToString() + ".lua";
    path DestPath = LuaScriptPath / ScriptFileNameStr;

    // --- 안전 검사 (1): 템플릿 파일 확인 ---
    if (!exists(LuaTemplatePath))
    {
        UE_LOG("[LuaManager] Lua 템플릿 파일이 존재하지 않습니다: %ls", LuaTemplatePath.c_str());
        return sol::environment{};
    }

    // --- 안전 검사 (2): 대상 파일이 이미 존재하는지 확인 ---
    if (exists(DestPath))
    {
        UE_LOG("[LuaManager] 파일이 이미 존재합니다: %s. 생성을 건너뜁니다.", ScriptFileNameStr.c_str());
        return sol::environment{};
    }

    // 복사
    try
    {
        std::filesystem::copy_file(LuaTemplatePath, DestPath);
    }
    catch (const std::filesystem::filesystem_error& Error)
    {
        UE_LOG("[LuaManager] Lua 템플릿 복사 실패: %hs", Error.what());
        return sol::environment{};
    }

    LuaScriptCaches[LuaScriptName] = FLuaScriptInfo(DestPath, filesystem::last_write_time(DestPath));
    LuaScriptCaches[LuaScriptName].ScriptComponents.Add(ScriptComponent);

    UE_LOG("[LuaManager] 새 Lua 스크립트 생성됨: %s", ScriptFileNameStr.c_str());
    sol::environment Env(MasterLuaState, sol::create, MasterLuaState.globals());

    MasterLuaState.script_file(DestPath.string(), Env);

    return Env;
}

void ULuaManager::OpenScriptInEditor(const FName& LuaScriptName)
{
    if (LuaScriptName.IsNone())
    {
        UE_LOG("[LuaManager] 선택된 스크립트가 없습니다.");
        return;
    }

    FLuaScriptInfo* Info = LuaScriptCaches.Find(LuaScriptName);

    if (Info == nullptr)
    {
        UE_LOG_ERROR("[LuaManager] %s 스크립트 경로를 찾을 수 없습니다.", LuaScriptName.ToString().c_str());
        return;
    }

    wstring FullPathWStr = Info->FullPath.wstring();
    HINSTANCE InstanceHandle = ShellExecute(nullptr, L"open", FullPathWStr.data(),
        nullptr, LuaScriptPath.c_str(), SW_SHOWNORMAL
    );

    // 오류
    if (reinterpret_cast<INT_PTR>(InstanceHandle) <= 32)
    {
        MessageBox(nullptr, L"파일 열기에 실패했습니다.", L"Error", MB_OK | MB_ICONERROR);
    }
}

void ULuaManager::UnregisterComponent(UScriptComponent* ScriptComponent, const FName& LuaScriptName)
{
    if (LuaScriptName.IsNone()) return;

    FLuaScriptInfo* Info = LuaScriptCaches.Find(LuaScriptName);
    if (Info)
    {
        Info->ScriptComponents.Remove(ScriptComponent);
    }
}

void ULuaManager::CheckForHotReload()
{
    TArray<path> DeletedScripts;
    for (auto& Pair : LuaScriptCaches)
    {
        FName ScriptName = Pair.first;
        FLuaScriptInfo& Info = Pair.second;

        try
        {
            if (!exists(Info.FullPath))
            {
                DeletedScripts.Add(Info.FullPath);

                // TODO - ScriptComponent에게 바인딩 해제 알려주기
                continue;
            }

            auto CurrentModifiedTime = std::filesystem::last_write_time(Info.FullPath);
            if (CurrentModifiedTime > Info.LastModifiedTime)
            {
                UE_LOG("[LuaManager] 핫 리로드: %s", ScriptName.ToString().c_str());
                Info.LastModifiedTime = CurrentModifiedTime;

                for (int32 Idx = Info.ScriptComponents.Num() - 1; Idx >= 0; --Idx)
                {
                    TWeakObjectPtr<UScriptComponent> WeakComp = Info.ScriptComponents[Idx];

                    if (UScriptComponent* Comp = WeakComp.Get())
                    {
                        // 새 환경 생성
                        sol::environment NewEnv(MasterLuaState, sol::create, MasterLuaState.globals());
                        MasterLuaState.script_file(Info.FullPath.string(), NewEnv);
                        Comp->HotReload(NewEnv);
                    }
                    else
                    {
                        Info.ScriptComponents.RemoveAt(Idx);
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& Error)
        {
            UE_LOG_ERROR("[LuaManager] 핫 리로드 파일 검사 중 오류: %hs", Error.what());
        }
    }
}
