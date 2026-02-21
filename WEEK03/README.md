# Game Tech Lab - Week03 Engine
## 📖 프로젝트 개요
본 프로젝트는 Game Tech Lab 과정의 일환으로 개발된 **C++ 및 DirectX 11 기반의 미니 게임 엔진**입니다.

Windows 애플리케이션으로서, 기본적인 엔진 아키텍처와 3D 렌더링 파이프라인 구축에 중점을 두고 학습 및 개발을 진행했습니다.
## ✨ 주요 기능
- 🎮 **렌더링 엔진**: DirectX 11 API를 기반으로 3D 그래픽 렌더링을 지원합니다.
- 🎨 **에디터 UI**: 디버깅 및 실시간 객체 제어를 위해 **ImGui** 라이브러리를 통합했습니다.
- 📦 **에셋 관리**: 셰이더(.hlsl), 텍스처 등 필수 에셋을 로드하고 관리하는 기본 시스템을 갖추고 있습니다.
- 👁️ **뷰 모드**: **Lit**, **Unlit**, **Wireframe** 뷰 모드 간의 전환을 지원하여 다양한 관점에서 씬을 분석할 수 있습니다.
- 🌳 **씬 관리자**: 씬(Scene)에 배치된 모든 물체의 목록을 계층 구조로 시각화하고 관리합니다.
  - 이 엔진엔 액터가 없는 구조라서 현재는 씬 관리자가 프리미티브 컴포넌트들을 관리합니다.

## 💻 구현 내용

### Editor & Rendering
- 텍스쳐 아틀라스 (Atlas) 를 이해한다.
- 텍스쳐 맵핑과 UV 좌표계, VS Texture Sampler 에 대해 이해한다.
- Sprite (Billboard) 렌더링을 구현하고 Screen-Aligned Quad 렌더링을 이해한다.
- Batch Line 렌더링을 구현한다.
- Vertex와 Index의 관계를 이해하고 Index Buffer 사용해 본다.
- 프리미티브의 Bounding Box (AABB)를 계산하고 렌더링한다.
- 씬 (Scene)안에 배치되어 있는 오브젝트들을 관리할 수 있는 Scene Manager를 구현한다.
### Engine Core
- FName System
- UObject System
- RTTI

## 🛠️ 시작하기
### 요구 사항
- OS: Windows 10 이상
- IDE: Visual Studio 2022 이상
  - 반드시 "C++를 사용한 데스크톱 개발" 워크로드가 설치되어 있어야 합니다.
- SDK: Windows SDK (최신 버전 권장)
### 빌드 순서
1.  `git`을 사용하여 이 저장소를 로컬 컴퓨터에 복제합니다.
    ```bash
    git clone [저장소 URL]
    ```
4. Visual Studio에서 `Engine.sln` 솔루션 파일을 엽니다.
5. 솔루션 구성을 `Debug` 또는 `Release` 모드로, 플랫폼을 `x64`로 설정합니다.
6. 메뉴에서 `빌드 > 솔루션 빌드`를 선택하거나 단축키 F7을 눌러 프로젝트를 빌드합니다.
7. 빌드가 성공하면 `Build/Debug` 또는 `Build/Release` 디렉터리에서 실행 파일`(Engine.exe)`을 찾을 수 있습니다.
