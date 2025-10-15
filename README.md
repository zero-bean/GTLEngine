## GTL Week 6+ — 이번 주 구현 요약

이번 주에는 실시간 렌더링 품질과 장면 표현력을 높이기 위한 기능들을 추가/개선했습니다. 주요 초점은 “라이트처럼 보이는 파이어볼”, 장면 심도(Depth) 디버깅, 고도 안개(Height Fog), 포스트 프로세싱 안티앨리어싱(FXAA), 그리고 액터 이동 컴포넌트 계열입니다.

구현 항목
- Fireball Component 추가 (가산합 조명 + 차폐)
- Scene Depth View Mode 추가 (풀스크린 뎁스 시각화)
- Height Fog Component 추가 (고도 기반 대기 산란)
- FXAA 추가 (오프스크린→백버퍼 적용)
- Movement/Rotating/Projectile Movement Component 추가


## Fireball Component (빛처럼 보이도록 개선)

개요
- `UFireBallComponent`는 색/반경/강도/감쇠를 가진 포인트형 라이트 연출용 컴포넌트입니다. 결과는 “가산합(Additive) 조명”으로 합쳐져 실제 빛처럼 다른 색을 밝히고 누적됩니다.

핵심 구현
- 셰이더: `Engine/Asset/Shader/FireBallShader.hlsl`
  - VS: 모델/뷰/프로젝션 변환, 월드 노말 출력
  - PS: 거리 기반 감쇠 + 노말 점곱(diffuse). 출력은 `float4(color * strength, 0)`로 RGB 에너지만 누적
- 블렌딩: `D3D11_BLEND_ONE + D3D11_BLEND_ONE` (Renderer에서 FireBall 전용 블렌드 상태 구성)
- 차폐(occlusion): 두 단계
  1) CPU 코스 테스트: BVH 레이캐스트로 파이어볼→수신자 중심 구간에 더 가까운 오브젝트가 있으면 차폐 후보로 분기
  2) GPU 세밀 테스트: `Engine/Asset/Shader/ShadowProjectionShader.hlsl`에서 픽셀 단위로 “파이어볼-픽셀” 선분과 오클루더 AABB 교차 여부 검사. 교차 시 광량 0을 리턴해 뒤쪽 픽셀에 빛이 번지지 않음
- 상수버퍼
  - b4: `FFireBallConstants`(위치/반경/색/강도/감쇠)
  - b5: `FOccluderConstants`(오클루더 World/InvWorld/Scale). CPU 측 상수버퍼 크기를 구조체 크기와 일치시켜 정렬 문제 해결

사용 팁
- 머티리얼처럼 보이는 색은 과한 강도에서 쉽게 날아가므로 `Intensity/Radius/Falloff`를 함께 튜닝하세요.
- 차폐가 과하거나 덜하면 오클루더 스케일/트랜스폼이 올바른지 확인합니다.


## Scene Depth View Mode

개요
- 현재 뷰포트의 Z 버퍼를 이용해 카메라로부터의 선형 거리(Linear Eye Distance)를 시각화합니다. 디버깅/튜닝에 유효합니다.

핵심 구현
- 셰이더: `Engine/Asset/Shader/SceneDepthShader.hlsl`
  - 상수버퍼로 `InvViewProj`, 카메라 파라미터, 뷰포트 정규화 사각형을 전달
  - 깊이를 재구성해 그레이스케일로 출력
- 렌더러: `URenderer::RenderSceneDepthView(...)`에서 풀스크린 패스 수행, 엔진 뎁스 SRV 바인딩

사용 팁
- 원거리 범위 가시화 스케일은 셰이더 상수(`farVis`)로 튜닝할 수 있습니다.


## Height Fog Component

개요
- `UHeightFogComponent`는 고도 기반의 대기 산란을 구현합니다. 안개 밀도, 컷오프 거리, 높이, 최대 불투명도 등을 조절합니다.

핵심 구현
- 컴포넌트: `Engine/Source/Component/Public/HeightFogComponent.h`
  - 직렬화/복제 지원, 런타임 파라미터 조정 API 제공
- 셰이더: `Engine/Asset/Shader/HeightFogShader.hlsl` (풀스크린 포스트 연산)
- 블렌딩: 반투명 합성 전용 블렌딩 상태(`HeightFogBlendState`) 사용
- 렌더러: `URenderer::RenderHeightFog(...)`에서 상수버퍼(b1)에 `FHeightFogConstants` 바인딩 후 적용

사용 팁
- 강한 안개에서 물체 색이 씻기지 않도록 `FogMaxOpacity`와 `InscatteringColor`를 함께 보정하세요.


## FXAA (Fast Approximate Anti-Aliasing)

개요
- 씬을 오프스크린 렌더타겟에 먼저 그린 후, FXAA 픽셀 셰이더로 스무딩하여 백버퍼로 출력합니다.

핵심 구현
- 패스: `Engine/Source/Render/Renderer/Public/FXAAPass.h` / `.../Private/FXAAPass.cpp`
  - 오프스크린 `FXAASceneRTV/SRV`를 관리하고, `Apply(...)`에서 각 뷰포트 단위로 합성
- 파이프라인: 전용 VS/PS/샘플러/상수버퍼 구성, 리사이즈 대응
- 토글: `URenderer::SetFXAAEnabled(bool)` 제공

사용 팁
- 해상도 변화 시 `OnResize`를 통해 내부 리소스가 갱신됩니다.


## Movement Components

개요
- 액터 이동을 컴포넌트로 분리하여 재사용합니다. 기본 이동/회전/투사체 이동 3종을 제공합니다.

구성
- `UMovementComponent`
  - `UpdatedComponent`(보통 루트)를 대상으로 속도/이동을 적용
  - 직렬화/복제 지원, `MoveUpdatedComponent/UpdateComponentVelocity` 제공
- `URotatingMovementComponent`
  - `RotationRate`(deg/s)와 `PivotTranslation`으로 원운동/자전 구현
- `UProjectileMovementComponent`
  - `Velocity + Gravity`로 포물선 이동, 간단한 수치 적분으로 위치 업데이트

사용법
- 액터에 원하는 Movement 컴포넌트를 추가하고 `UpdatedComponent`를 지정합니다.
- JSON/에디터를 통해 `Velocity`, `RotationRate`, `ProjectileGravity` 등 속성을 설정할 수 있습니다.


## 실행/빌드 안내 (간략)
- 요구 사항: Windows + Visual Studio + Direct3D 11 런타임
- 실행: 솔루션을 빌드 후 실행. 에디터 뷰포트에서 컴포넌트 추가/파라미터 편집 가능


## 기술 메모 / 한계
- Fireball은 가산합 기반이므로 밝기/색이 쉽게 누적됩니다. `Intensity/Radius/Falloff` 튜닝이 중요합니다.
- Fireball 차폐는 코스(BVH) + 픽셀(AABB-선분 교차) 2단계입니다. 오클루더 바운딩이 실제 메시에 보수적으로 맞는지 확인하세요.
- Scene Depth는 카메라 파라미터 불일치 시 왜곡될 수 있습니다. `InvViewProj` 갱신을 보장해야 합니다.


## 디버그/통계
- 성능 오버레이/콘솔은 기존 시스템을 그대로 사용합니다. 필요 시 FXAA/안개/파이어볼 렌더 구간을 프로파일링해 병목을 파악하세요.

