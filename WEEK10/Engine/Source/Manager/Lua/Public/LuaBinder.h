#pragma once
#include "sol/sol.hpp"
#include "Core/Public/WeakObjectPtr.h"

class FLuaBinder
{
public:
	static void BindCoreTypes(sol::state& LuaState);
	static void BindMathTypes(sol::state& LuaState);
	static void BindActorTypes(sol::state& LuaState);
	static void BindComponentTypes(sol::state& LuaState);
	static void BindCoreFunctions(sol::state& LuaState);
};

//=============================================================================
// Lua Binding Macros
//=============================================================================

// Helper macro for adding type-safe cast functions to AActor
// Usage: Add new types easily by adding a single line with this macro
// Example: BIND_ACTOR_CAST(AEnemySpawnerActor) adds ToAEnemySpawnerActor() method
#define BIND_ACTOR_CAST(ClassName) \
	"To" #ClassName, [](AActor* Self) -> ClassName* { \
		if (!Self) return nullptr; \
		if (Self->GetClass()->IsChildOf(ClassName::StaticClass())) { \
			return static_cast<ClassName*>(Self); \
		} \
		return nullptr; \
	}

// Helper macro for binding TWeakObjectPtr<T> to Lua
// Creates: WeakClassName usertype with Get(), IsValid() methods and MakeWeakClassName() global function
// Usage: BIND_WEAK_PTR(AActor) creates:
//   - WeakAActor type with Get(), IsValid() methods
//   - MakeWeakAActor(obj) global function
// Example Lua usage:
//   local weakEnemy = MakeWeakAActor(enemy)
//   if weakEnemy:IsValid() then
//       local actor = weakEnemy:Get()
//       actor.Location = ...
//   end
#define BIND_WEAK_PTR(ClassName) \
	LuaState.new_usertype<TWeakObjectPtr<ClassName>>( \
		"Weak" #ClassName, \
		"Get", [](const TWeakObjectPtr<ClassName>& weak) -> ClassName* { \
			return weak.Get(); \
		}, \
		"IsValid", [](const TWeakObjectPtr<ClassName>& weak) -> bool { \
			return weak.IsValid(); \
		}, \
		sol::meta_function::equal_to, [](const TWeakObjectPtr<ClassName>& a, const TWeakObjectPtr<ClassName>& b) -> bool { \
			return a == b; \
		}, \
		sol::meta_function::to_string, [](const TWeakObjectPtr<ClassName>& weak) -> std::string { \
			return weak.IsValid() ? "Weak" #ClassName "(valid)" : "Weak" #ClassName "(invalid)"; \
		} \
	); \
	LuaState.set_function("MakeWeak" #ClassName, [](ClassName* obj) -> TWeakObjectPtr<ClassName> { \
		return TWeakObjectPtr<ClassName>(obj); \
	});
