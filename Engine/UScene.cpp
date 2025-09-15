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

IMPLEMENT_UCLASS(UScene, UObject)
UScene::UScene()
{
	version = 1;
	primitiveCount = 0;
}

UScene::~UScene()
{
	OnShutdown();
	for (UObject* object : objects)
	{
		delete object;
	}
	delete camera;
}

bool UScene::Initialize(URenderer* r, UMeshManager* mm, UInputManager* im)
{
	renderer = r;
	meshManager = mm;
	inputManager = im;

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
	json::JSON result;
	// UScene 특성에 맞는 JSON 구성
	result["Version"] = version;
	result["NextUUID"] = std::to_string(UEngineStatics::GetNextUUID());
	int32 validCount = 0;
	for (UObject* object : objects)
	{
		if (object == nullptr) continue;
		json::JSON _json = object->Serialize();
		if (!_json.IsNull())
		{
			//result["Primitives"][std::to_string(validCount)] = _json;
			result["Primitives"][std::to_string(object->UUID)] = _json;
			++validCount;
		}
	}
	return result;
}

bool UScene::Deserialize(const json::JSON& data)
{
	version = data.at("Version").ToInt();
	// nextUUID = data.at("NextUUID").ToInt();

	objects.clear();
	json::JSON primitivesJson = data.at("Primitives");

	UEngineStatics::SetUUIDGeneration(false);
	UObject::ClearFreeIndices();
	for (auto& primitiveJson : primitivesJson.ObjectRange())
	{
		uint32 uuid = stoi(primitiveJson.first);
		json::JSON _data = primitiveJson.second;

		UClass* _class = UClass::FindClassWithDisplayName(_data.at("Type").ToString());
		USceneComponent* component = nullptr;
			if(_class != nullptr) component = _class->CreateDefaultObject()->Cast<USceneComponent>();

		component->Deserialize(_data);
		component->SetUUID(uuid);

		objects.push_back(component);
		if (component->CountOnInspector())
			++primitiveCount;
	}

	// 왜 있는 건지 모름!
	/*USceneComponent* gizmoGrid = NewObject<UGizmoGridComp>(
		FVector{ 0.3f, 0.3f, 0.3f },
		FVector{ 0.0f, 0.0f, 0.0f },
		FVector{ 0.2f, 0.2f, 0.2f }
	);
	objects.push_back(gizmoGrid);*/

	FString uuidStr = data.at("NextUUID").ToString();

	UEngineStatics::SetNextUUID((uint32)stoi(uuidStr));
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
			primitive->Draw(*renderer);
		}
	}

	renderer->FlushBatchLineList();
	renderer->BeginBatchLineList();
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

	////// TODO(PYB) ///////////////////
	// 아마도.. 여기에 물체 피지컬 검사?
	////////////////////////////////////
}

bool UScene::OnInitialize()
{

	return true;
}

