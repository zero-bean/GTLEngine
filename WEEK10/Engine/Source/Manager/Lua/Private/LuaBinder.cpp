#include "pch.h"
#include "Manager/Lua/Public/LuaBinder.h"
#include "Actor/Public/Actor.h"
#include "Actor/Public/GameMode.h"
#include "Actor/Public/EnemySpawnerActor.h"
#include "Actor/Public/PlayerCameraManager.h"
#include "Demo/Public/Player.h"
#include "Component/Public/ScriptComponent.h"
#include "Component/Public/PrimitiveComponent.h"
#include "Manager/Input/Public/InputManager.h"
#include "Level/Public/Level.h"
#include "Level/Public/CurveLibrary.h"
#include "Physics/Public/HitResult.h"
#include "Global/Enum.h"
#include "Global/CurveTypes.h"
#include "Render/UI/Overlay/Public/D2DOverlayManager.h"
// Sound
#include "Manager/Sound/Public/SoundManager.h"
#include "Render/Camera/Public/CameraModifier.h"
#include "Render/Camera/Public/CameraModifier_CameraShake.h"

#if WITH_EDITOR
#include "Editor/Public/EditorEngine.h"
#endif

void FLuaBinder::BindCoreTypes(sol::state& LuaState)
{
	// --- UWorld ---
    LuaState.new_usertype<UWorld>("UWorld",
        "SpawnActor", sol::overload(
            sol::resolve<AActor*(const std::string&)>(&UWorld::SpawnActor),
            sol::resolve<AActor*(UClass*, JSON*)>(&UWorld::SpawnActor)
        ),
        "GetTimeSeconds", &UWorld::GetTimeSeconds,
        "DestroyActor", &UWorld::DestroyActor,
        "GetGameMode", &UWorld::GetGameMode,
        "GetLevel", &UWorld::GetLevel,
        "FindTemplateActorOfName", &UWorld::FindTemplateActorOfName,
        "GetSourceEditorWorld", &UWorld::GetSourceEditorWorld
    );

    // --- GWorld 인스턴스 접근자 (전역 함수) ---
    LuaState.set_function("GetWorld", []() -> UWorld* {
    	return GWorld;
    });

	// --- ULevel ---
	LuaState.new_usertype<ULevel>("ULevel",
		// Get all actors in the level (returns Lua table)
		"GetLevelActors", [](ULevel* Level, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			const TArray<AActor*>& Actors = Level->GetLevelActors();
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];  // Lua는 1부터 시작
			}
			return result;
		},

		// Get all template actors in the level (returns Lua table)
		"GetTemplateActors", [](ULevel* Level, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			const TArray<AActor*>& Actors = Level->GetTemplateActors();
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Find template actor by name
		// Returns AActor* (can be nullptr) - supports polymorphism via sol2 RTTI
		"FindTemplateActorByName", [](ULevel* Level, const std::string& InName) -> AActor* {
			if (!Level)
				return nullptr;
			return Level->FindTemplateActorByName(FName(InName));
		},

		// Find all template actors by class (returns Lua table)
		"FindTemplateActorsOfClass", [](ULevel* Level, UClass* InClass, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level || !InClass)
				return result;

			TArray<AActor*> Actors = Level->FindTemplateActorsOfClass(InClass);
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Find regular actor by name (template actors excluded)
		// Returns AActor* (can be nullptr) - supports polymorphism via sol2 RTTI
		"FindActorByName", [](ULevel* Level, const std::string& InName) -> AActor* {
			if (!Level)
				return nullptr;
			return Level->FindActorByName(FName(InName));
		},

		// Find all regular actors by class (returns Lua table, template actors excluded)
		// 클래스명 문자열로 찾기
		"FindActorsOfClass", [](ULevel* Level, const std::string& ClassName, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Level)
				return result;

			TArray<AActor*> Actors = Level->FindActorsOfClassByName(FString(ClassName));
			for (int i = 0; i < Actors.Num(); ++i)
			{
				result[i + 1] = Actors[i];
			}
			return result;
		},

		// Sweep single collision test for Actor (returns first hit)
		// Tests all PrimitiveComponents of the Actor
		// Optional FilterTag parameter: only returns hits with matching CollisionTag
		"SweepActorSingle", sol::overload(
			// Without filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation) -> sol::optional<FHitResult> {
				if (!Level || !Actor)
					return sol::nullopt;

				FHitResult HitResult;
				bool bHit = Level->SweepActorSingle(Actor, TargetLocation, HitResult);
				if (bHit)
					return HitResult;
				return sol::nullopt;
			},
			// With filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation, ECollisionTag FilterTag) -> sol::optional<FHitResult> {
				if (!Level || !Actor)
					return sol::nullopt;

				FHitResult HitResult;
				bool bHit = Level->SweepActorSingle(Actor, TargetLocation, HitResult, FilterTag);
				if (bHit)
					return HitResult;
				return sol::nullopt;
			}
		),

		// Sweep multi collision test for Actor (returns all overlapping components)
		// Tests all PrimitiveComponents of the Actor
		// Optional FilterTag parameter: only returns hits with matching CollisionTag
		"SweepActorMulti", sol::overload(
			// Without filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation) -> sol::optional<TArray<UPrimitiveComponent*>> {
				if (!Level || !Actor)
					return sol::nullopt;

				TArray<UPrimitiveComponent*> OverlappingComponents;
				bool bHit = Level->SweepActorMulti(Actor, TargetLocation, OverlappingComponents);
				if (bHit && OverlappingComponents.Num() > 0)
					return OverlappingComponents;
				return sol::nullopt;
			},
			// With filter
			[](ULevel* Level, AActor* Actor, const FVector& TargetLocation, ECollisionTag FilterTag) -> sol::optional<TArray<UPrimitiveComponent*>> {
				if (!Level || !Actor)
					return sol::nullopt;

				TArray<UPrimitiveComponent*> OverlappingComponents;
				bool bHit = Level->SweepActorMulti(Actor, TargetLocation, OverlappingComponents, FilterTag);
				if (bHit && OverlappingComponents.Num() > 0)
					return OverlappingComponents;
				return sol::nullopt;
			}
		),

		// Get CurveLibrary from level
		"GetCurveLibrary", &ULevel::GetCurveLibrary
	);

	// --- FCurve ---
	LuaState.new_usertype<FCurve>("FCurve",
		sol::call_constructor, sol::factories(
			[]() { return FCurve(); },
			[](float CP1_X, float CP1_Y, float CP2_X, float CP2_Y) {
				return FCurve(CP1_X, CP1_Y, CP2_X, CP2_Y);
			}
		),
		// Members
		"ControlPoint1X", &FCurve::ControlPoint1X,
		"ControlPoint1Y", &FCurve::ControlPoint1Y,
		"ControlPoint2X", &FCurve::ControlPoint2X,
		"ControlPoint2Y", &FCurve::ControlPoint2Y,

		// Evaluate method
		"Evaluate", &FCurve::Evaluate,

		// Static factory methods
		"Linear", &FCurve::Linear,
		"EaseIn", &FCurve::EaseIn,
		"EaseOut", &FCurve::EaseOut,
		"EaseInOut", &FCurve::EaseInOut,
		"EaseInBack", &FCurve::EaseInBack,
		"EaseOutBack", &FCurve::EaseOutBack
	);

	// --- UCurveLibrary ---
	LuaState.new_usertype<UCurveLibrary>("UCurveLibrary",
		// Get curve by name (returns nil if not found)
		"GetCurve", [](UCurveLibrary* Library, const std::string& Name) -> FCurve* {
			if (!Library)
				return nullptr;

			FCurve* Curve = Library->GetCurve(FString(Name));
			return Curve; // Return pointer, not copy
		},

		// Check if curve exists
		"HasCurve", [](UCurveLibrary* Library, const std::string& Name) -> bool {
			if (!Library)
				return false;
			return Library->HasCurve(FString(Name));
		},

		// Get all curve names (returns Lua table)
		"GetCurveNames", [](UCurveLibrary* Library, sol::this_state ts) -> sol::table {
			sol::state_view lua(ts);
			sol::table result = lua.create_table();

			if (!Library)
				return result;

			TArray<FString> Names = Library->GetCurveNames();
			for (int i = 0; i < Names.Num(); ++i)
			{
				result[i + 1] = std::string(Names[i]);  // Convert FString to std::string for Lua
			}
			return result;
		},

		// Get curve count
		"GetCurveCount", [](UCurveLibrary* Library) -> int {
			if (!Library)
				return 0;
			return Library->GetCurveCount();
		}
	);


	// --- TimeManager ---
	LuaState.new_usertype<UTimeManager>("UTimeManager",
		"SetGlobalTimeDilation", &UTimeManager::SetGlobalTimeDilation,
		"GetDeltaTimesa", &UTimeManager::GetDeltaTime
	);

	// --- TimeManager 인스턴스 접근자 (전역 함수) ---
	LuaState.set_function("GetTimeManager", []() -> UTimeManager& {
		return UTimeManager::GetInstance();
	});
}

void FLuaBinder::BindMathTypes(sol::state& LuaState)
{
	// -- Vector -- //
	LuaState.new_usertype<FVector>("FVector",
		sol::call_constructor,
		sol::factories(
			[]() { return FVector(); },
			[](float X, float Y, float Z) { return FVector(X, Y, Z); }
		),
		"X", &FVector::X,
		"Y", &FVector::Y,
		"Z", &FVector::Z,

		sol::meta_function::addition, sol::overload(
			[](const FVector& a, const FVector& b) { return a + b; }
		),

		sol::meta_function::subtraction, sol::overload(
			[](const FVector& a, const FVector& b) { return a - b; }
		),

		sol::meta_function::multiplication, sol::overload(
			[](const FVector& a, const FVector& b) { return a * b; },
			[](const FVector& v, float s) { return v * s; },
			[](float s, const FVector& v) { return s * v; }
		),

		sol::meta_function::division, sol::overload(
			[](const FVector& a, const FVector& b) { return a / b; },
			[](const FVector& v, float s) { return v / s; }
		),

		"Normalize", &FVector::Normalize,
		"Length", &FVector::Length
	);

	// -- Quaternion -- //
	LuaState.new_usertype<FQuaternion>("FQuaternion",
		sol::call_constructor,
	    sol::factories(
	       []() { return FQuaternion(); },
	       [](float X, float Y, float Z, float W) { return FQuaternion(X, Y, Z, W); }
	    ),
	    "X", &FQuaternion::X,
	    "Y", &FQuaternion::Y,
	    "Z", &FQuaternion::Z,
	    "W", &FQuaternion::W,

	    sol::meta_function::multiplication, &FQuaternion::operator*,

	    // --- Static Func ---
	    // Lua: local q = FQuaternion.Identity()
	    "Identity", &FQuaternion::Identity,
	    // Lua: local q = FQuaternion.FromAxisAngle(FVector(0,0,1), math.rad(90))
	    "FromAxisAngle", &FQuaternion::FromAxisAngle,
	    // Lua: local q = FQuaternion.FromEuler(FVector(0, 0, 90))
	    "FromEuler", &FQuaternion::FromEuler,
	    // Lua: local q = FQuaternion.FromRotationMatrix(someMatrix)
	    "FromRotationMatrix", &FQuaternion::FromRotationMatrix,
	    // Lua: local q = FQuaternion.MakeFromDirection(FVector(1,0,0))
	    "MakeFromDirection", &FQuaternion::MakeFromDirection,

	    // --- Member Func ---
	    // Lua: local eulerVec = myQuat:ToEuler()
	    "ToEuler", &FQuaternion::ToEuler,
	    // Lua: local matrix = myQuat:ToRotationMatrix()
	    "ToRotationMatrix", &FQuaternion::ToRotationMatrix,
	    // Lua: myQuat:Normalize()
	    "Normalize", &FQuaternion::Normalize,
	    // Lua: local conj = myQuat:Conjugate()
	    "Conjugate", &FQuaternion::Conjugate,
	    // Lua: local inv = myQuat:Inverse()
	    "Inverse", &FQuaternion::Inverse,

	    // --- 6. 오버로드된 함수 (정적 & 멤버) ---
	    "RotateVector", sol::overload(
	        // C++: static FVector RotateVector(const FQuaternion& q, const FVector& v)
	        // Lua: local rotatedVec = FQuaternion.RotateVector(myQuat, myVec)
	        sol::resolve<FVector(const FQuaternion&, const FVector&)>(&FQuaternion::RotateVector),

	        // C++: FVector FQuaternion::RotateVector(const FVector& V) const
	        // Lua: local rotatedVec = myQuat:RotateVector(myVec)
	        sol::resolve<FVector(const FVector&) const>(&FQuaternion::RotateVector)
	    )
	);

	// -- HitResult -- //
	LuaState.new_usertype<FHitResult>("FHitResult",
		sol::call_constructor,
		sol::factories(
			[]() { return FHitResult(); },
			[](const FVector& InLocation, const FVector& InNormal) { return FHitResult(InLocation, InNormal); }
		),
		// Members
		"Location", &FHitResult::Location,
		"Normal", &FHitResult::Normal,
		"PenetrationDepth", &FHitResult::PenetrationDepth,
		"Actor", &FHitResult::Actor,
		"Component", &FHitResult::Component,
		"bBlockingHit", &FHitResult::bBlockingHit
	);
}

void FLuaBinder::BindActorTypes(sol::state& LuaState)
{
	// -- Actor -- //
	LuaState.new_usertype<AActor>("AActor",
		"Name", sol::property(
			[](AActor* Actor) -> std::string { return static_cast<string>(Actor->GetName().ToString()); }
		),
		"Location", sol::property(
			&AActor::GetActorLocation,
			&AActor::SetActorLocation
		),
		"Rotation", sol::property(
			&AActor::GetActorRotation,
			&AActor::SetActorRotation
		),
		"Scale", sol::property(
			&AActor::GetActorScale3D,
			&AActor::SetActorScale3D
		),
		"UUID", sol::property(
			&AActor::GetUUID
		),
		"Tag", sol::property(
			[](AActor* Actor) -> int32 { return static_cast<int32>(Actor->GetCollisionTag()); }
		),
		"IsTemplate", sol::property(
			&AActor::IsTemplate,
			&AActor::SetIsTemplate
		),
		"Duplicate", &AActor::Duplicate,
		"DuplicateFromTemplate", sol::overload(
			// 파라미터 없음 - 템플릿의 Outer Level에 추가, 기본 위치/회전
			[](AActor* Self) {
				return Self->DuplicateFromTemplate();
			},
			// Level 지정 - 지정된 Level에 추가, 기본 위치/회전
			[](AActor* Self, ULevel* TargetLevel) {
				return Self->DuplicateFromTemplate(TargetLevel);
			},
			// Level + Location 지정
			[](AActor* Self, ULevel* TargetLevel, const FVector& InLocation) {
				return Self->DuplicateFromTemplate(TargetLevel, InLocation);
			},
			// Level + Location + Rotation 지정 (완전한 제어)
			[](AActor* Self, ULevel* TargetLevel, const FVector& InLocation, const FQuaternion& InRotation) {
				return Self->DuplicateFromTemplate(TargetLevel, InLocation, InRotation);
			}
		),
		"GetActorForwardVector", sol::resolve<FVector() const>(&AActor::GetActorForwardVector),
		"GetActorUpVector", sol::resolve<FVector() const>(&AActor::GetActorUpVector),
		"GetActorRightVector", sol::resolve<FVector() const>(&AActor::GetActorRightVector),

		// Type-safe casting functions - add new actor types here using the macro
		BIND_ACTOR_CAST(AEnemySpawnerActor),
		BIND_ACTOR_CAST(APlayer),
		BIND_ACTOR_CAST(APlayerCameraManager)
	);

	// --- AGameMode ---
    LuaState.new_usertype<AGameMode>("AGameMode",
        sol::base_classes, sol::bases<AActor>(),
        "InitGame", &AGameMode::InitGame,
        "StartGame", &AGameMode::StartGame,
        "EndGame", &AGameMode::EndGame,
        "OverGame", &AGameMode::OverGame,
        "GetPlayerCameraManager", &AGameMode::GetPlayerCameraManager,


        "IsGameRunning", sol::property(&AGameMode::IsGameRunning),
        "IsGameEnded", sol::property(&AGameMode::IsGameEnded),

        "OnGameInited", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& LuaFunc) {
                if (!Self || !LuaFunc.valid()) return;
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameInited.Add([WeakGameMode, LuaFunc]()
                {
                    if (WeakGameMode.IsValid())
                    {
                        auto Result = LuaFunc();
                        if (!Result.valid())
                        {
                            sol::error Err = Result;
                            UE_LOG_ERROR("[Lua Error] %s", Err.what());
                        }
                    }
                });
            }
        ),

        "OnGameStarted", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& LuaFunc) {
                if (!Self || !LuaFunc.valid()) return;
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameStarted.Add([WeakGameMode, LuaFunc]()
                {
                    if (WeakGameMode.IsValid())
                    {
                        auto Result = LuaFunc();
                        if (!Result.valid())
                        {
                            sol::error Err = Result;
                            UE_LOG_ERROR("[Lua Error] %s", Err.what());
                        }
                    }
                });
            }
        ),

        "OnGameEnded", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& InLuaFunc) {
                if (!Self || !InLuaFunc.valid()) return;
                sol::protected_function SafeFunc = InLuaFunc; // wrap for safety
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameEnded.Add([WeakGameMode, SafeFunc]() mutable
                {
                    if (!WeakGameMode.IsValid()) return;
                    if (!SafeFunc.valid()) return;
                    sol::protected_function_result Result = SafeFunc();
                    if (!Result.valid()) {
                        sol::error Err = Result;
                        UE_LOG_ERROR("[Lua Error][OnGameEnded] %s", Err.what());
                    }
                });
            }
        ),

        "OnGameOvered", sol::writeonly_property(
            [](AGameMode* Self, const sol::function& InLuaFunc) {
                if (!Self || !InLuaFunc.valid()) return;
                sol::protected_function SafeFunc = InLuaFunc; // wrap for safety
                TWeakObjectPtr<AGameMode> WeakGameMode(Self);
                Self->OnGameOvered.Add([WeakGameMode, SafeFunc]() mutable
                {
                    if (!WeakGameMode.IsValid()) return;
                    if (!SafeFunc.valid()) return;
                    sol::protected_function_result Result = SafeFunc();
                    if (!Result.valid()) {
                        sol::error Err = Result;
                        UE_LOG_ERROR("[Lua Error][OnGameOvered] %s", Err.what());
                    }
                });
            }
        ),
		"GetPlayer", &AGameMode::GetPlayer
    );

	// --- AEnemySpawnerActor ---
	LuaState.new_usertype<AEnemySpawnerActor>("AEnemySpawnerActor",
		sol::base_classes, sol::bases<AActor>(),
		"RequestSpawn", &AEnemySpawnerActor::RequestSpawn
	);

	// --- APlayer ---
	LuaState.new_usertype<APlayer>("APlayer",
		sol::base_classes, sol::bases<AActor>(),

		"OnPlayerTracking", sol::writeonly_property(
			[](APlayer* Self, const sol::function& LuaFunc) {
				if (!Self || !LuaFunc.valid()) return;
				TWeakObjectPtr<APlayer> WeakPlayer(Self);
				Self->OnPlayerTracking.Add([WeakPlayer, LuaFunc](float LightLevel, FVector PlayerLocation)
				{
					if (WeakPlayer.IsValid())
					{
						auto Result = LuaFunc(LightLevel, PlayerLocation);
						if (!Result.valid())
						{
							sol::error Err = Result;
							UE_LOG_ERROR("[Lua Error] OnPlayerTracking: %s", Err.what());
						}
					}
				});
			}
		),

		"BroadcastTracking", &APlayer::BroadcastTracking
	);

	// --- WeakObjectPtr Bindings ---
	// Add TWeakObjectPtr support for safe object tracking from Lua
	BIND_WEAK_PTR(AActor);
	// BIND_WEAK_PTR(AEnemySpawnerActor);  // Example: add more types as needed

	// --- FOscillator ---
	LuaState.new_usertype<FOscillator>("FOscillator",
		sol::call_constructor, sol::factories(
			[]() { return FOscillator(); },
			[](float Amplitude, float Frequency) { return FOscillator(Amplitude, Frequency); }
		),
		"Amplitude", &FOscillator::Amplitude,
		"Frequency", &FOscillator::Frequency,
		"Phase", &FOscillator::Phase
	);

	// --- UCameraModifier (base class) ---
	LuaState.new_usertype<UCameraModifier>("UCameraModifier",
		"DisableModifier", &UCameraModifier::DisableModifier,
		"EnableModifier", &UCameraModifier::EnableModifier,
		"IsDisabled", &UCameraModifier::IsDisabled,
		"GetAlpha", &UCameraModifier::GetAlpha,

		// Alpha blend time
		"GetAlphaInTime", &UCameraModifier::GetAlphaInTime,
		"SetAlphaInTime", &UCameraModifier::SetAlphaInTime,
		"GetAlphaOutTime", &UCameraModifier::GetAlphaOutTime,
		"SetAlphaOutTime", &UCameraModifier::SetAlphaOutTime,

		// Alpha blend curves
		"SetAlphaInCurve", &UCameraModifier::SetAlphaInCurve,
		"SetAlphaOutCurve", &UCameraModifier::SetAlphaOutCurve
	);

	// --- UCameraModifier_CameraShake ---
	LuaState.new_usertype<UCameraModifier_CameraShake>("UCameraModifier_CameraShake",
		sol::base_classes, sol::bases<UCameraModifier>(),
		sol::call_constructor, sol::factories(
			[]() { return NewObject<UCameraModifier_CameraShake>(); }
		),
		// Public properties
		"Duration", &UCameraModifier_CameraShake::Duration,
		"ElapsedTime", sol::readonly(&UCameraModifier_CameraShake::ElapsedTime),
		"bIsPlaying", sol::readonly(&UCameraModifier_CameraShake::bIsPlaying),
		"FOVOscillation", &UCameraModifier_CameraShake::FOVOscillation,
		// Array accessors for LocOscillation[3] - lua uses 1-based indexing
		"GetLocOscillation", [](UCameraModifier_CameraShake* self, int index) -> FOscillator& {
			if (index < 1 || index > 3) {
				throw std::out_of_range("LocOscillation index must be 1-3");
			}
			return self->LocOscillation[index - 1];
		},
		// Array accessors for RotOscillation[3]
		"GetRotOscillation", [](UCameraModifier_CameraShake* self, int index) -> FOscillator& {
			if (index < 1 || index > 3) {
				throw std::out_of_range("RotOscillation index must be 1-3");
			}
			return self->RotOscillation[index - 1];
		},
		// Setter methods for convenience
		"SetIntensity", &UCameraModifier_CameraShake::SetIntensity,
		"SetDuration", &UCameraModifier_CameraShake::SetDuration,
		"SetBlendInTime", &UCameraModifier_CameraShake::SetBlendInTime,
		"SetBlendOutTime", &UCameraModifier_CameraShake::SetBlendOutTime
	);

	// --- APlayerCameraManager ---
	LuaState.new_usertype<APlayerCameraManager>("APlayerCameraManager",
		sol::base_classes, sol::bases<AActor>(),
		"EnableLetterBox", &APlayerCameraManager::EnableLetterBox,
		"DisableLetterBox", &APlayerCameraManager::DisableLetterBox,
		"SetVignetteIntensity", &APlayerCameraManager::SetVignetteIntensity,
		"AddCameraModifier", &APlayerCameraManager::AddCameraModifier,
		"RemoveCameraModifier", &APlayerCameraManager::RemoveCameraModifier,
		"ClearCameraModifiers", &APlayerCameraManager::ClearCameraModifiers,

		// SetViewTargetWithBlend with optional curve parameter
		"SetViewTargetWithBlend", [](APlayerCameraManager* Self, AActor* NewViewTarget, float BlendTime, sol::optional<FCurve*> Curve)
		{
			const FCurve* CurvePtr = Curve.value_or(nullptr);
			Self->SetViewTargetWithBlend(NewViewTarget, BlendTime, CurvePtr);
		},

		// StartCameraFade with optional curve parameter
		"StartCameraFade", [](APlayerCameraManager* Self, float FromAlpha, float ToAlpha, float Duration,
		                       FVector Color, sol::optional<bool> bHoldWhenFinished, sol::optional<FCurve*> FadeCurve)
		{
			bool bHold = bHoldWhenFinished.value_or(false);
			const FCurve* CurvePtr = FadeCurve.value_or(nullptr);
			Self->StartCameraFade(FromAlpha, ToAlpha, Duration, Color, bHold, CurvePtr);
		},

		// StopCameraFade
		"StopCameraFade", &APlayerCameraManager::StopCameraFade,

		// SetManualCameraFade
		"SetManualCameraFade", &APlayerCameraManager::SetManualCameraFade
	);
}

void FLuaBinder::BindComponentTypes(sol::state& LuaState)
{
	LuaState.new_usertype<UScriptComponent>("ScriptComponent",
		"StartCoroutine", &UScriptComponent::StartCoroutine,
		"StopCoroutine", &UScriptComponent::StopCoroutine,
		"StopAllCoroutines", &UScriptComponent::StopAllCoroutines
	);
}

void FLuaBinder::BindCoreFunctions(sol::state& LuaState)
{
	// -- Log -- //
	LuaState.set_function("Log", [](sol::variadic_args Vars) {
			std::stringstream ss;
			sol::state_view Lua(Vars.lua_state());

			for (auto v : Vars)
			{
				sol::protected_function ToString = Lua["tostring"];
				sol::protected_function_result Result = ToString(v);

				FString Str;
				if (Result.valid())
				{
					Str = Result.get<std::string>();
				}
				else
				{
					Str = "[nil or error]";
				}
				ss << Str << "\t";
			}

			std::string FinalLogMessage = ss.str();
			if (!FinalLogMessage.empty())
			{
				FinalLogMessage.pop_back();
			}

			UE_LOG("%s", FinalLogMessage.c_str());
		}
	);

	// -- Curve Helper -- //
	// Global Curve() function for convenience
	// Usage: local curve = Curve("EaseOutBack")
	LuaState.set_function("Curve", [](const std::string& CurveName) -> FCurve* {
		if (!GWorld)
		{
			UE_LOG_ERROR("Lua Curve(): GWorld is null");
			return nullptr;
		}

		ULevel* Level = GWorld->GetLevel();
		if (!Level)
		{
			UE_LOG_ERROR("Lua Curve(): Level is null");
			return nullptr;
		}

		UCurveLibrary* Library = Level->GetCurveLibrary();
		if (!Library)
		{
			UE_LOG_ERROR("Lua Curve(): CurveLibrary is null");
			return nullptr;
		}

		FCurve* FoundCurve = Library->GetCurve(CurveName);
		if (!FoundCurve)
		{
			UE_LOG_WARNING("Lua Curve(): Curve '%s' not found in CurveLibrary", CurveName.c_str());
			return nullptr;
		}

		return FoundCurve;
	});

	// -- InputManager -- //
	UInputManager& InputMgr = UInputManager::GetInstance();
	LuaState.set_function("IsKeyDown", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyDown(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("IsKeyPressed", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyPressed(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("IsKeyReleased", [&InputMgr](int32 Key) {
		// PIE World에서 입력 차단 중이면 false 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return false;
		}
		return InputMgr.IsKeyReleased(static_cast<EKeyInput>(Key));
	});
	LuaState.set_function("GetMouseNDCPosition", [&InputMgr]() {
		return InputMgr.GetMouseNDCPosition();
	});
	LuaState.set_function("GetMousePosition", [&InputMgr]() {
		return InputMgr.GetMousePosition();
	});
	LuaState.set_function("GetMouseDelta", [&InputMgr]() {
		// PIE World에서 입력 차단 중이면 0 벡터 반환
		if (GWorld && GWorld->IsIgnoringInput())
		{
			return FVector(0, 0, 0);
		}

		// PIE와 StandAlone 모두 ConsumeMouseDelta 사용
		// Editor 모드(WorldType::Editor)에서만 일반 GetMouseDelta 사용
#if WITH_EDITOR
		if (GEditor && GEditor->IsPIESessionActive() && !GEditor->IsPIEMouseDetached())
		{
			return InputMgr.ConsumeMouseDelta();
		}
		// Editor 모드 (PIE 아님)
		return InputMgr.GetMouseDelta();
#else
		// StandAlone 모드: PIE와 동일하게 ConsumeMouseDelta 사용
		return InputMgr.ConsumeMouseDelta();
#endif
	});

	// -- D2D Overlay Manager -- //
    sol::table DebugDraw = LuaState.create_table("DebugDraw");

	// --- Line ---
	// Lua: DebugDraw.Line(startX, startY, endX, endY, r, g, b, a, thickness)
	DebugDraw.set_function("Line",
	    [](float startX, float startY, float endX, float endY,
	       float cR, float cG, float cB, float cA,
	       float Thickness)
	    {
	        D2D1_POINT_2F Start = D2D1::Point2F(startX, startY);
	        D2D1_POINT_2F End = D2D1::Point2F(endX, endY);
	        D2D1_COLOR_F color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddLine(Start, End, color, Thickness);
	    }
	);

	// --- Ellipse ---
	// Lua: DebugDraw.Ellipse(cX, cY, rX, rY, r, g, b, a, bFilled)
	DebugDraw.set_function("Ellipse",
	    [](float cX, float cY, float RadiusX, float RadiusY,
	       float cR, float cG, float cB, float cA,
	       bool bFilled)
	    {
	        D2D1_POINT_2F Center = D2D1::Point2F(cX, cY);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddEllipse(Center, RadiusX, RadiusY, Color, bFilled);
	    }
	);

	// --- Rectangle ---
	// Lua: DebugDraw.Rectangle(l, t, r, b, r, g, b, a, bFilled)
	DebugDraw.set_function("Rectangle",
	    [](float rL, float rT, float rR, float rB,
	       float cR, float cG, float cB, float cA,
	       bool bFilled)
	    {
	        D2D1_RECT_F Rect = D2D1::RectF(rL, rT, rR, rB);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        FD2DOverlayManager::GetInstance().AddRectangle(Rect, Color, bFilled);
	    }
	);

	// --- Text ---
	// Lua: DebugDraw.Text(text, l, t, r, b, r, g, b, a, fontSize, bBold, bCentered, fontName)
	DebugDraw.set_function("Text",
	    [](const std::string& Text,
	       float rL, float rT, float rR, float rB,
	       float cR, float cG, float cB, float cA,
	       float FontSize, bool bBold, bool bCentered, const std::string& FontName)
	    {
	        D2D1_RECT_F Rect = D2D1::RectF(rL, rT, rR, rB);
	        D2D1_COLOR_F Color = D2D1::ColorF(cR, cG, cB, cA);

	        std::wstring w_text = StringToWideString(Text);
	        std::wstring w_fontName = StringToWideString(FontName);

	        FD2DOverlayManager::GetInstance().AddText(
	            w_text.c_str(), Rect, Color, FontSize, bBold, bCentered, w_fontName.c_str()
	        );
	    }
	);

	// --- GetViewportWidth ---
	// Lua: DebugDraw.GetViewportWidth()
	DebugDraw.set_function("GetViewportWidth",
	    []()
	    {
	        return FD2DOverlayManager::GetInstance().GetViewportWidth();
	    }
	);

	// --- GetViewportHeight ---
	// Lua: DebugDraw.GetViewportHeight()
	DebugDraw.set_function("GetViewportHeight",
	    []()
	    {
	        return FD2DOverlayManager::GetInstance().GetViewportHeight();
	    }
	);
	// -- Math -- //
	LuaState.set_function("Clamp", &Clamp<float>);

    // -- Sound (BGM only for now) -- //
    // Initialize (idempotent)
    LuaState.set_function("Sound_Initialize", []()
    {
        USoundManager::GetInstance().InitializeAudio();
    });

    // Play BGM with overloads
    LuaState.set_function("Sound_PlayBGM",
        sol::overload(
            // path only (loop=true, fade=0.5)
            [](const std::string& Path)
            {
                auto& Manager = USoundManager::GetInstance();
                Manager.InitializeAudio();
                Manager.PlayBGM(FString(Path), true, 0.5f);
            },
            // path + loop
            [](const std::string& Path, bool bLoop)
            {
                auto& Manager = USoundManager::GetInstance();
                Manager.InitializeAudio();
                Manager.PlayBGM(FString(Path), bLoop, 0.5f);
            },
            // path + loop + fadeSeconds
            [](const std::string& Path, bool bLoop, float FadeSeconds)
            {
                auto& Manager = USoundManager::GetInstance();
                Manager.InitializeAudio();
                Manager.PlayBGM(FString(Path), bLoop, FadeSeconds);
            }
        )
    );

    // Stop BGM with optional fade
    LuaState.set_function("Sound_StopBGM",
        sol::overload(
            []()
            {
                USoundManager::GetInstance().StopBGM(0.3f);
            },
            [](float FadeSeconds)
            {
                USoundManager::GetInstance().StopBGM(FadeSeconds);
            }
        )
    );

    // --- SFX (basic bindings) ---
    LuaState.set_function("Sound_PreloadSFX",
        [](const std::string& Name, const std::string& Path, bool bIsThreeDimensional, float MinDistance, float MaxDistance)
        {
            auto& Manager = USoundManager::GetInstance();
            Manager.InitializeAudio();
            Manager.PreloadSFX(FName(Name.c_str()), FString(Path), bIsThreeDimensional, MinDistance, MaxDistance);
        }
    );

    LuaState.set_function("Sound_PlaySFX",
        sol::overload(
            [](const std::string& Name)
            {
                USoundManager::GetInstance().PlaySFX(FName(Name.c_str()), 1.0f, 1.0f);
            },
            [](const std::string& Name, float Volume)
            {
                USoundManager::GetInstance().PlaySFX(FName(Name.c_str()), Volume, 1.0f);
            },
            [](const std::string& Name, float Volume, float Pitch)
            {
                USoundManager::GetInstance().PlaySFX(FName(Name.c_str()), Volume, Pitch);
            }
        )
    );

    LuaState.set_function("Sound_PlaySFXAt",
        sol::overload(
            [](const std::string& Name, const FVector& Position)
            {
                USoundManager::GetInstance().PlaySFXAt(FName(Name.c_str()), Position, FVector::Zero(), 1.0f, 1.0f);
            },
            [](const std::string& Name, const FVector& Position, float Volume)
            {
                USoundManager::GetInstance().PlaySFXAt(FName(Name.c_str()), Position, FVector::Zero(), Volume, 1.0f);
            },
            [](const std::string& Name, const FVector& Position, float Volume, float Pitch)
            {
                USoundManager::GetInstance().PlaySFXAt(FName(Name.c_str()), Position, FVector::Zero(), Volume, Pitch);
            },
            [](const std::string& Name, const FVector& Position, const FVector& Velocity, float Volume, float Pitch)
            {
                USoundManager::GetInstance().PlaySFXAt(FName(Name.c_str()), Position, Velocity, Volume, Pitch);
            }
        )
    );

    // Looping SFX
    LuaState.set_function("Sound_PlayLoopingSFX",
        sol::overload(
            [](const std::string& Name, const std::string& Path)
            {
                auto& Manager = USoundManager::GetInstance();
                Manager.InitializeAudio();
                Manager.PlayLoopingSFX(FName(Name.c_str()), FString(Path), 1.0f);
            },
            [](const std::string& Name, const std::string& Path, float Volume)
            {
                auto& Manager = USoundManager::GetInstance();
                Manager.InitializeAudio();
                Manager.PlayLoopingSFX(FName(Name.c_str()), FString(Path), Volume);
            }
        )
    );
    LuaState.set_function("Sound_StopLoopingSFX",
        [](const std::string& Name)
        {
            USoundManager::GetInstance().StopLoopingSFX(FName(Name.c_str()));
        }
    );

    // Stop all sounds (BGM + SFX)
    LuaState.set_function("Sound_StopAll", []()
    {
        USoundManager::GetInstance().StopAllSounds();
    });

    // Application Exit
    LuaState.set_function("ExitApplication", []()
    {
#if WITH_EDITOR
        // PIE 모드에서는 PIE만 종료 (다음 프레임에 처리)
        if (GEditor && GEditor->IsPIESessionActive())
        {
            GEditor->RequestEndPIE();
        }
#else
        // StandAlone 모드에서는 애플리케이션 전체 종료
        PostQuitMessage(0);
#endif
    });
}
