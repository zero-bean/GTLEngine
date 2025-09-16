// UScene.cpp
#include "stdafx.h"
#include "json.hpp"
#include "UScene.h"
#include "UObject.h"
#include "USceneComponent.h"
#include "UPrimitiveComponent.h"
#include "UGizmoGridComp.h"
#include "URaycastManager.h"
#include "UCamera.h"
#include "ShowFlagManager.h"

IMPLEMENT_UCLASS(UScene, UObject)
UScene::UScene()
	: isInitialized(false)
	, renderer(nullptr)
	, meshManager(nullptr)
	, inputManager(nullptr)
	, camera(nullptr)
	, ShowFlagManager(nullptr)
	, backBufferWidth(0)
	, backBufferHeight(0)
{
	version = 1;
	primitiveCount = 0;
}

UScene::~UScene()
{
	OnShutdown();
	for (UObject* Object : objects)
	{
		delete Object;
	}
	delete camera;
}

bool UScene::Initialize(URenderer* r, UMeshManager* mm, UShowFlagManager* InShowFlagManager, UInputManager* im)
{
	renderer = r;
	meshManager = mm;
	inputManager = im;
	ShowFlagManager = InShowFlagManager;

	backBufferWidth = 0;
	backBufferHeight = 0;

	// 모든 Primitive 컴포넌트 초기화
	for (UObject* obj : objects)
	{
		if (UPrimitiveComponent* primitive = obj->Cast<UPrimitiveComponent>())
		{
			primitive->Init(meshManager);
		}
	}
	// TODO - UApplication에서 하면 가장 마지막에 처리되므로 
	// 카메라가 생성되기 전에 값이 로드되지않음.
	// 따라서 위치를 카메라 생성되기 직전에 둔다.
	// 필요하면 경로 커스터마이즈
	CEditorIni::Get().SetPath(std::filesystem::path("./config/editor.ini"));
	CEditorIni::Get().Load(); // 없으면 걍 실패하고 넘어감(기본값 사용)
	camera = new UCamera();
	camera->LookAt({ -5,0,0 }, { 0,0,0 }, { 0,0,1 });
	return OnInitialize();
}

UScene* UScene::Create(json::JSON data)
{
	UScene* scene = NewObject<UScene>();
	scene->Deserialize(data);
	return scene;
}

void UScene::AddObject(USceneComponent* obj)
{
	// 런타임에서만 사용 - Scene이 Initialize된 후에 호출할 것
	assert(meshManager != nullptr && "AddObject should only be called after Scene initialization");

	if (!meshManager)
	{
		// 릴리즈 빌드에서 안전성 확보
		return;
	}

	objects.push_back(obj);

	// 일단 표준 RTTI 사용
	if (UPrimitiveComponent* primitive = obj->Cast<UPrimitiveComponent>())
	{
		primitive->Init(meshManager);
		if (obj->CountOnInspector())
			++primitiveCount;
	}
}

json::JSON UScene::Serialize() const
{
	json::JSON Result;
	// UScene 특성에 맞는 JSON 구성
	Result["Version"] = version;
	Result["NextUUID"] = std::to_string(UEngineStatics::GetNextUUID());

	// Primitives 키를 항상 Object로 생성해 빈 씬에서도 키가 존재하도록 보장
	Result["Primitives"] = json::JSON::Make(json::JSON::Class::Object);

	for (UObject* Object : objects)
	{
		if (Object == nullptr) 
		{
			continue;
		}

		json::JSON Json = Object->Serialize();
		if (!Json.IsNull())
		{
			Result["Primitives"][std::to_string(Object->UUID)] = Json;
		}
	}
	return Result;
}

bool UScene::Deserialize(const json::JSON& data)
{
	// Version 안전 접근 (없으면 기본값)
	if (data.hasKey("Version")) {
		version = data.at("Version").ToInt();
	}
	else {
		version = 1;
	}

	objects.clear();
	primitiveCount = 0;

	// Primitives가 없으면 빈 Object로 대체하여 예외 방지
	json::JSON PrimitivesJson = data.hasKey("Primitives")
		? data.at("Primitives")
		: json::JSON::Make(json::JSON::Class::Object);

	UEngineStatics::SetUUIDGeneration(false);
	UObject::ClearFreeIndices();

	if (PrimitivesJson.JSONType() == json::JSON::Class::Object)
	{
		for (auto& PrimJson : PrimitivesJson.ObjectRange())
		{
			uint32 Uuid = 0;
			try {
				Uuid = static_cast<uint32>(std::stoul(PrimJson.first));
			}
			catch (...) {
				continue; // 잘못된 키는 스킵
			}

			const json::JSON& Data = PrimJson.second;

			// Type 누락/미지원 타입 방어
			if (!Data.hasKey("Type")) 
			{
				continue;
			}
			UClass* Class = UClass::FindClassWithDisplayName(Data.at("Type").ToString());
			if (Class == nullptr) 
			{
				continue;
			}

			USceneComponent* Component = Class->CreateDefaultObject()->Cast<USceneComponent>();
			if (!Component) 
			{
				continue;
			}

			Component->Deserialize(Data);
			Component->SetUUID(Uuid);

			objects.push_back(Component);
			if (Component->CountOnInspector())
			{
				++primitiveCount;
			}
		}
	}

	// NextUUID가 있을 때만 설정
	if (data.hasKey("NextUUID")) {
		FString uuidStr = data.at("NextUUID").ToString();
		try { UEngineStatics::SetNextUUID(static_cast<uint32>(std::stoul(uuidStr))); }
		catch (...) {}
	}

	UEngineStatics::SetUUIDGeneration(true);
	return true;
}

void UScene::Render()
{
	// 카메라가 바뀌면 원하는 타이밍(매 프레임도 OK)에 알려주면 됨
	renderer->SetTargetAspect(camera->GetAspect());
	renderer->SetViewProj(camera->GetView(), camera->GetProj());

	ID3D11DeviceContext* context = renderer->GetDeviceContext();
	context->IASetInputLayout(renderer->GetInputLayout("Default"));
	context->VSSetShader(renderer->GetVertexShader("Default"), nullptr, 0);
	context->PSSetShader(renderer->GetPixelShader("Default"), nullptr, 0);
	for (UObject* obj : objects)
	{
		if (UPrimitiveComponent* primitive = obj->Cast<UPrimitiveComponent>())
		{
			if (ShowFlagManager->IsEnabled(EEngineShowFlags::SF_Primitives))
			{
				primitive->Draw(*renderer);
			}
		}
	}

	renderer->FlushBatchLineList();
	renderer->FlushBatchSprite();
}

void UScene::Update(float deltaTime)
{
	renderer->GetBackBufferSize(backBufferWidth, backBufferHeight);

	if (backBufferHeight > 0)
	{
		camera->SetAspect((float)backBufferWidth / (float)backBufferHeight);
	}

	float dx = 0, dy = 0, dz = 0;
	bool boost = inputManager->IsKeyDown(VK_SHIFT); // Shift로 가속

	// --- 마우스룩: RMB 누른 동안 회전 ---
	if (inputManager->IsMouseLooking())
	{
		// 마우스룩 모드는 WndProc에서 Begin/End로 관리
		float mdx = 0.f, mdy = 0.f;
		inputManager->ConsumeMouseDelta(mdx, mdy);

		const float sens = 0.005f; // 일단 크게 해서 동작 확인
		camera->AddYawPitch(mdx * sens, mdy * sens);
	}
	if (inputManager->IsKeyDown('W'))
	{
		dy += 1.0f; // 전진
	}
	if (inputManager->IsKeyDown('A'))
	{
		dx -= 1.0f; // 좌
	}
	if (inputManager->IsKeyDown('S'))
	{
		dy -= 1.0f; // 후진
	}
	if (inputManager->IsKeyDown('D'))
	{
		dx += 1.0f; // 우
	}
	if (inputManager->IsKeyDown('E'))
	{
		dz += 1.0f; // 상
	}
	if (inputManager->IsKeyDown('Q'))
	{
		dz -= 1.0f; // 하
	}

	static float t = 0.0f; t += deltaTime;
	// 대각선 이동 속도 보정(선택): 벡터 정규화
	float len = sqrtf(dx * dx + dy * dy + dz * dz);
	if (len > 0.f) { dx /= len; dy /= len; dz /= len; }
	float moveSpeed = camera->GetMoveSpeed();
	camera->MoveLocal(dx, dy, dz, deltaTime, boost, moveSpeed);
}

bool UScene::OnInitialize()
{

	return true;
}

