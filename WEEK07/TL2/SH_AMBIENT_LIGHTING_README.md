# Spherical Harmonics (SH) Ambient Lighting System

언리얼 엔진과 소스 엔진 스타일의 실시간 SH 기반 Ambient Light 시스템을 DirectX11 렌더링 프레임워크에 구현했습니다.

## 개요

이 시스템은 6방향 큐브맵 캡처를 통해 환경광을 실시간으로 샘플링하고, Spherical Harmonics L2 (9개 계수)로 압축하여 픽셀 셰이더에서 방향성 있는 ambient lighting을 제공합니다.

## 주요 기능

### 1. 6방향 환경 캡처
- **큐브맵 방식**: +X, -X, +Y, -Y, +Z, -Z 6개 방향으로 씬을 렌더링
- **HDR 텍스처**: R16G16B16A16_FLOAT 포맷 사용
- **저해상도 최적화**: 기본 64×64 해상도로 성능 최적화 (설정 가능)

### 2. SH 투영 (Projection)
- **9개 기저함수**: L2 Spherical Harmonics (DC + Linear + Quadratic)
- **Radiance → Irradiance 변환**: 코사인 컨볼루션 계수 적용
  - L0 band (DC): π
  - L1 band (Linear): 2π/3
  - L2 band (Quadratic): π/4
- **정확한 입체각 계산**: 큐브맵 텍셀별 solid angle 가중치 적용

### 3. 시간 필터링 (Temporal Smoothing)
- **점진적 갱신**: `SH_smooth = lerp(SH_old, SH_new, smoothingFactor)`
- **프레임 간격 제어**: N프레임마다 한 번 캡처하여 성능 최적화
- **부드러운 전환**: 조명 변화 시 깜빡임 방지

### 4. 셰이더 통합
- **상수 버퍼 b10**: SH 계수 9개 × RGB (float3[9])
- **EvaluateSHLighting()**: 노멀 벡터 기반 ambient light 계산
- **기존 조명과 결합**: PointLight, DirectionalLight와 자동으로 합성

## 파일 구조

```
TL2/
├── CBufferTypes.h              # FSHAmbientLightBufferType 정의 (b10)
├── DynamicAmbientProbe.h       # Probe 컴포넌트 클래스
├── DynamicAmbientProbe.cpp     # 6방향 캡처, SH 투영, 필터링 구현
├── StaticMeshShader.hlsl       # SH 평가 함수 및 적용
├── Renderer.h/.cpp             # RenderSHAmbientLightPass() 추가
└── SH_AMBIENT_LIGHTING_README.md
```

## 사용 방법

### 1. DynamicAmbientProbe 컴포넌트 추가

씬에 DynamicAmbientProbe를 추가하여 ambient light를 활성화합니다:

```cpp
// Actor에 DynamicAmbientProbe 컴포넌트 추가
UDynamicAmbientProbe* Probe = CreateComponent<UDynamicAmbientProbe>("AmbientProbe");
Probe->SetupAttachment(RootComponent);

// 캡처 설정
Probe->SetCaptureResolution(64);      // 64x64 해상도 (기본값)
Probe->SetUpdateInterval(0.1f);       // 0.1초마다 업데이트
Probe->SetSmoothingFactor(0.1f);      // 10% 블렌딩 (부드럽게)
Probe->SetSHIntensity(1.0f);          // 강도 배율
```

### 2. 설정 파라미터

#### CaptureResolution (int32)
- **범위**: 32 ~ 256
- **기본값**: 64
- **설명**: 큐브맵 각 면의 해상도. 높을수록 정확하지만 성능 저하.

#### UpdateInterval (float)
- **범위**: 0.0 ~ 1.0 (초 단위)
- **기본값**: 0.1
- **설명**: 캡처 간격. 0.0은 매 프레임, 0.1은 10fps로 업데이트.

#### SmoothingFactor (float)
- **범위**: 0.0 ~ 1.0
- **기본값**: 0.1
- **설명**: 시간 필터링 강도. 0.0은 변화 없음, 1.0은 즉시 변경.

#### SHIntensity (float)
- **범위**: 0.0 ~ 10.0
- **기본값**: 1.0
- **설명**: Ambient light 전역 강도 배율.

### 3. 런타임 제어

```cpp
// 즉시 캡처 (수동)
Probe->ForceCapture();

// 설정 동적 변경
Probe->SetCaptureResolution(128);  // 더 높은 품질로 변경
Probe->SetSmoothingFactor(0.3f);   // 더 빠른 반응
```

### 4. 여러 Probe 사용 (선택사항)

현재는 씬의 **첫 번째 활성화된 Probe**만 사용합니다. 여러 Probe를 동적으로 전환하려면:

```cpp
// 특정 Probe 비활성화
Probe1->SetActive(false);

// 다른 Probe 활성화
Probe2->SetActive(true);
```

## 렌더링 파이프라인 통합

SH Ambient Light는 다음 순서로 렌더링됩니다:

```
RenderScene()
├── RenderPointLightPass()        // PointLight 수집
│   └── RenderSHAmbientLightPass() // SH 계수 업로드 (b10)
├── RenderBasePass()               // 메시 렌더링 (ambient 적용)
├── RenderFogPass()
└── RenderFXAAPaxx()
```

### 셰이더에서의 처리

`StaticMeshShader.hlsl` 픽셀 셰이더:

```hlsl
// 1. SH 계수로 ambient 계산
float3 shAmbient = EvaluateSHLighting(normal) * baseColor;

// 2. 기존 조명과 합성
float3 finalColor = shAmbient + diffuseLit + specularLit;
```

## 성능 최적화

### 메모리 사용량
- **큐브맵 6면**: 64×64×6 × 8 bytes (RGBA16F) = **196 KB**
- **SH 계수**: 9 × 3 × 4 bytes = **108 bytes**
- **총합**: ~200 KB per Probe

### GPU 부하
- **캡처**: 6번의 작은 렌더 패스 (64×64)
- **투영 계산**: CPU에서 한 번 (간격 제어 가능)
- **평가**: 픽셀 셰이더에서 9개 내적 연산

### 권장 설정

| 시나리오 | CaptureResolution | UpdateInterval | SmoothingFactor |
|---------|-------------------|----------------|-----------------|
| 정적 씬 | 128 | 1.0 | 0.05 |
| 동적 씬 | 64 | 0.1 | 0.1 |
| 고성능 | 32 | 0.2 | 0.2 |

## 기술 세부사항

### SH 기저함수 (L2, 9개)

```cpp
// L0 (DC)
Y₀₀ = 0.282095

// L1 (Linear)
Y₁₋₁ = 0.488603 * y
Y₁₀  = 0.488603 * z
Y₁₁  = 0.488603 * x

// L2 (Quadratic)
Y₂₋₂ = 1.092548 * x * y
Y₂₋₁ = 1.092548 * y * z
Y₂₀  = 0.315392 * (3z² - 1)
Y₂₁  = 1.092548 * x * z
Y₂₂  = 0.546274 * (x² - y²)
```

### 입체각 계산

큐브맵 텍셀의 정확한 기여도:

```cpp
float TexelSolidAngle(int x, int y, int size) {
    float u = (x + 0.5) / size * 2 - 1;
    float v = (y + 0.5) / size * 2 - 1;

    auto area = [](float x, float y) {
        return atan2(x*y, sqrt(x*x + y*y + 1));
    };

    return area(u-ε, v-ε) - area(u-ε, v+ε)
         - area(u+ε, v-ε) + area(u+ε, v+ε);
}
```

## 알려진 제한사항

1. **씬 캡처 미구현**: `CaptureEnvironment()` 내부의 실제 렌더링 코드는 렌더링 파이프라인에 맞게 구현 필요
2. **단일 Probe**: 현재는 하나의 전역 Probe만 지원. 로컬 볼륨 블렌딩은 미지원
3. **CPU 투영**: SH 투영이 CPU에서 수행됨. 컴퓨트 셰이더로 이동 시 성능 향상 가능
4. **Half-float 변환**: R16G16B16A16 readback 시 간단한 정규화 사용. 정확한 half→float 변환 권장

## 향후 개선 방향

- [ ] 컴퓨트 셰이더 기반 SH 투영
- [ ] 여러 Probe 블렌딩 (거리 기반)
- [ ] Probe 볼륨 (3D 그리드)
- [ ] 미리 계산된 Static Probe
- [ ] L3 (16개 계수) 옵션

## 참고 자료

- [Spherical Harmonics - Robin Green (2003)](https://www.ppsloan.org/publications/StupidSH36.pdf)
- [Real-Time Rendering, 4th Edition - Chapter 10.5](https://www.realtimerendering.com/)
- [Unreal Engine: Indirect Lighting](https://docs.unrealengine.com/5.0/en-US/indirect-lighting-in-unreal-engine/)

## 라이선스

DirectX11 렌더링 프레임워크의 일부로 제공됩니다.
