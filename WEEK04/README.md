
# ✦ WEEK 04: 엔진 개발 기술 문서

이번 4주차 개발 과정에서는 **외부 3D 모델링 데이터를 엔진에 통합**하고, 이를 효율적으로 렌더링하며, 사용자 편의성을 높이는 에디터 기능을 구현하는 데 중점을 두었습니다.  
핵심 목표는 **Wavefront OBJ 파일을 엔진의 자체 형식으로 변환하는 데이터 파이프라인을 구축**하고, **StaticMesh를 씬에 배치하여 렌더링**하는 것이었습니다.  
또한, **다중 뷰포트와 개선된 에디터 UI**를 통해 작업 효율성을 극대화하고자 했습니다.  

본 문서는 이번 주에 구현된 주요 기능들을 **Editor & Rendering**과 **Engine Core** 두 가지 주제로 나누어, 각 기능의 구현 목표, 설계 방식, 그리고 설계 의도를 상세히 설명합니다.  

---

## 주요 구현 내용

## 1. Editor & Rendering

사용자가 3D 모델을 직접 씬에 배치하고, 여러 각도에서 관찰하며, 작업 내용을 저장하고 불러올 수 있는 **직관적인 환경**을 구축했습니다.

---

### 1.1 StaticMesh 렌더링 및 조작

- 사용자가 에디터 내에서 `.obj` 파일 기반의 **정적 메시(StaticMesh)** 를 씬에 배치하고,  
  위치·회전·크기를 조작하며, 그 결과를 **씬 파일(.json)에 저장 및 복원**할 수 있는 기능을 구현.
  
1. `UStaticMeshComp`를 `UPrimitiveComponent`의 자식 클래스로 구현 → 씬에 배치 가능한 기본 단위 생성.  
2. 에디터 UI 패널에 **드롭다운 리스트** 추가.  
   - `TObjectIterator<UStaticMesh>`를 활용해 엔진에 로드된 모든 `UStaticMesh Asset`을 동적으로 표시.  
3. 드롭다운에서 선택된 Asset을 `UStaticMeshComp`에 **Assign(할당)** 가능.  
4. 렌더링 전용 **StaticMeshShader.hlsl** 추가.  
   - `FVertexPNCT` 정점 구조체를 입력받아 **Texture & Material 처리** 수행.  

```cpp
// Vertex Layout
struct FVertexPNCT {
    FVector Position;
    FVector Normal;
    FVector4 Color;
    FVector2 UV;
};

// StaticMeshShader VS_INPUT
struct VS_INPUT {
    float3 p : POSITION;
    float3 n : NORMAL;
    float4 c : COLOR;
    float2 t : TEXTURE;
};
````

5. `UStaticMeshComp::Serialize` 함수를 통해 **씬 저장 시 JSON 포맷으로 Asset 정보 관리**.

   * 로드 시 `FObjManager`를 통해 재로딩 → 씬 복원.

```json
"Primitives" : {
  "3": {
    "ObjStaticMeshAsset": "Data/Cube.obj",
    "Location": [ -0.563450, 0.573828, -0.020000 ],
    "Rotation": [ 0.860000, 0.000000, 5.990000 ],
    "Scale": [ 1.000000, 1.000000, 1.000000 ],
    "Type": "StaticMeshComp"
  }
}
```

* **Asset ↔ Scene Object(컴포넌트) 분리** → 관리 효율성 극대화.
* 씬 파일에는 **경로만 저장**하여 파일 크기 최소화.
* `FObjManager`에서 **중앙 관리 → 중복 로드 방지 → 성능 및 확장성 향상**.

---

### 1.2 다중 뷰포트 및 직교 뷰 구현

* 단일 Perspective 뷰 → Orthogonal 뷰를 포함한 4분할 뷰포트 환경 구현.
* 사용자는 **스플리터** 및 **버튼**으로 뷰포트의 수와 크기 조절 가능.

1. `SSplitterH`, `SSplitterV` 클래스 구현 (재귀적 화면 분할).

```cpp
class SSplitter : public SWindow {
    SWindow* SideLT; // Left or Top
    SWindow* SideRB; // Right or Bottom
};
```

2. `FViewport`, `FViewportClient` 도입 → 뷰포트별 카메라(Perspective/Orthogonal) 관리.
3. `RSSetViewports` API 사용 → 각 뷰포트 영역에 맞게 렌더링 출력.
4. 스플리터 위치는 **Editor.ini에 저장** → 레이아웃 복원.
5. Perspective 뷰 카메라는 **씬 파일에 직렬화(Serializer)** → 마지막 카메라 구도 유지.

```json
"PerspectiveCamera": {
  "Location": [0.000000, 2.000000, -5.000000],
  "Rotation": [0.100000, 0.000000, 0.000000],
  "FOV": 60.0,
  "NearClip": 0.100000,
  "FarClip": 1000.000000
}
```

* 다중 뷰포트 + 직교 뷰 → **정밀한 오브젝트 배치 필수**.
* 전체 구도는 Perspective로 확인, 세부 축 조정은 Orthogonal 뷰로 처리.
* **편의성과 정확성 동시 확보**.

---

## 2. Engine Core (눈에 안 보이는 세상)

보이지 않는 영역에서는 외부 데이터를 **엔진 친화적 구조로 가공**하고, 효율적으로 관리하며,
다른 시스템에서 쉽게 접근 가능하도록 **견고한 기반을 구축**했습니다.

---

### 2.1 Wavefront OBJ 파싱 및 데이터 파이프라인

* `.obj` / `.mtl` 파일을 읽어, 엔진에서 렌더링 가능한 **FStaticMesh** 형식으로 변환.

1. `FObjImporter` 클래스 구현 → .obj 파싱 → Raw Data(`FObjInfo`) 생성.
2. Raw Data → \*\*GPU 최적화된 Cooked Data(`FStaticMesh`)\*\*로 변환.

   * **중복 제거된 고유 정점 생성 + 인덱스 버퍼 구성**.

```cpp
// Raw Data
struct FObjInfo { ... };

// Cooked Data
struct FStaticMesh {
    std::string PathFileName;
    TArray<FNormalVertex> Vertices;
    TArray<uint32> Indices;
    // + Material Section 정보
};
```

3. 다중 Material 지원 → `.mtl` 파일 파싱 후 FStaticMesh 내에 **Material Section 관리**.

* Raw Data (편집 친화) vs Cooked Data (GPU 최적화) **분리 필수**.
* Cooked Data로 Baking/Cooking → **로딩 속도 및 렌더링 성능 최적화**.

---

### 2.2 Asset 관리 시스템 (FObjManager & UStaticMesh)

1. `FObjManager` → FStaticMesh(Cooked Data) **싱글톤 캐싱 관리**.

```cpp
class FObjManager {
private:
    static std::map<string, FStaticMesh*> ObjStaticMeshMap;
public:
    static FStaticMesh* LoadObjStaticMeshAsset(const std::string& PathFileName);
    static UStaticMesh* LoadObjStaticMesh(const std::string& PathFileName);
};
```

2. `UStaticMesh` → `UObject` 상속, FStaticMesh를 **래핑(Wrapper)**.

```cpp
class UStaticMesh : public UObject {
    FStaticMesh* StaticMeshAsset;
    ...
};
```

3. `UStaticMeshComp` → 어떤 메시 데이터를 렌더링할지 `UStaticMesh` 포인터로 지정.

* `FObjManager` 캐싱 → **중복 파싱 방지**.
* `UStaticMesh`를 `UObject`화 → **GC, Reflection, Serializer 활용 가능**.
* 정적 메시가 단순 데이터가 아닌, 엔진 관리 **완전한 Asset**으로 기능.

---

### 2.3 TObjectIterator 구현

* 특정 타입의 모든 `UObject` 인스턴스를 순회할 수 있는 **템플릿 이터레이터** 구현.

1. 내부적으로 전역 배열 `GObjObjects` 순회.
2. `operator++` 오버로딩 → **유효한 TObject 타입**만 탐색.

```cpp
// 모든 UStaticMesh 순회 예시
for (TObjectIterator<UStaticMesh> It; It; ++It) {
    UStaticMesh* StaticMesh = *It;
    // ...
}
```

* **Iterator 패턴 적용** → 특정 타입 오브젝트 접근 로직 단순화.
* 에디터 UI, 씬 관리 등에서 코드 가독성과 유지보수성 향상.

---

## 결론

4주차 개발을 통해 **3D 데이터를 저장 및 로드하고 편집의 자유도가 존재하는 엔진**을 완성했습니다.

* **Raw → Cooked Data 변환**
* **Asset 관리 시스템 구축**
* **사용자 친화적 에디터 UI**

**Parser, Serializer, Iterator**와 같은 관리가 용이한 개념을 적용하고,
**StaticMesh, Material, Texture** 를 추가하고, 결합하여
**더 복잡하고 사실적인 기능을 보장하는 에디터**를 구성할 수 있는 발판을 가지게 되었습니다.

```

