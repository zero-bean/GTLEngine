# 심볼 서버 통합 가이드

## 개요

이 프로젝트는 팀 전체가 PDB 심볼을 공유할 수 있는 **자동 심볼 서버 시스템**을 통합했습니다.

(2025.11.24 ~ 2025.11.27 동안은 `YOUR-PC-NAME` == \\172.21.11.95\SymbolServer)


### 주요 기능

1. **자동 심볼 검색**: 로컬 캐시 → 심볼 서버 → 자동 다운로드
2. **자동 업로드**: 빌드 시 PDB를 심볼 서버에 자동 업로드
3. **팀 공유**: 모든 팀원이 같은 심볼 서버 사용

---

## 1. 초기 설정 (각 팀원 1회만)

### 1.1 심볼 서버 경로 확인

심볼 서버 공유 폴더 경로:
```
\\YOUR-PC-NAME\SymbolServer
```

### 1.2 CrashHandler 심볼 서버 경로 설정

`Mundi\Source\Runtime\Debug\CrashHandler.cpp` 파일의 33번 줄을 수정:

```cpp
// 심볼 서버 초기화 (로컬 캐시 + 네트워크 심볼 서버)
FSymbolServerManager::Initialize(L"C:\\SymbolCache", L"\\\\YOUR-PC-NAME\\SymbolServer");
```

**`YOUR-PC-NAME`을 실제 심볼 서버 PC 이름으로 변경하세요!**

### 1.3 빌드 시스템 설정 (선택사항)

빌드 후 자동으로 PDB를 업로드하려면:

**방법 1: Visual Studio Post-Build Event**

프로젝트 속성 → Build Events → Post-Build Event → Command Line:

```powershell
powershell.exe -ExecutionPolicy Bypass -File "$(SolutionDir)Scripts\UploadSymbols.ps1" -ExePath "$(TargetPath)" -PdbPath "$(TargetDir)$(TargetName).pdb" -SymbolServer "\\YOUR-PC-NAME\SymbolServer"
```

**방법 2: 수동 업로드 (필요할 때만)**

```cpp
#include "Debug/SymbolServerManager.h"

// 프로그램 시작 시 1회 호출
FSymbolServerManager::Initialize(L"C:\\SymbolCache", L"\\\\YOUR-PC-NAME\\SymbolServer");

// 빌드 후 수동 업로드
FSymbolServerManager::UploadSymbolsIfNeeded(
    L"Binaries\\Debug\\Mundi.exe",
    L"Binaries\\Debug\\Mundi.pdb"
);
```

---

## 2. 동작 방식

### 2.1 덤프 분석 시 심볼 검색 순서

```
덤프 파일 열기
    ↓
1. 로컬 심볼 캐시 확인 (C:\SymbolCache)
    ↓ (없음)
2. 심볼 서버에서 검색 (C:\SymbolServer 또는 \\172.21.11.95\SymbolServer)
    ↓ (서버 접근 불가 시 로컬 캐시만 사용)
    ↓ (있으면 자동 다운로드 → 로컬 캐시에 복사)
3. 심볼 로드 완료 → 덤프 분석 가능
```

### 2.2 서버 접근 실패 시 동작 (Graceful Degradation)

**서버 PC가 꺼진 경우:**
- ✅ 프로그램은 정상 실행 (로컬 캐시만 사용)
- ⚠️ 새 심볼은 다운로드 불가 (캐시에 있는 것만 사용)
- 📝 콘솔 메시지: `[SymbolServer] Server unavailable, using local cache only`

**다시 켜진 경우:**
- ✅ 자동으로 서버 재연결
- ✅ 새 심볼 다운로드 가능
```

### 2.3 빌드 시 자동 업로드

```
빌드 완료
    ↓
Post-Build 이벤트 실행
    ↓
1. 심볼 서버에 이미 있는지 확인 (PDB GUID 비교)
    ↓ (없으면)
2. symstore.exe로 심볼 서버에 업로드
    ↓
3. 다른 팀원들도 즉시 사용 가능
```

---

## 3. 검증 방법

### 4.1 심볼 서버 연결 확인

```powershell
# PowerShell에서 실행
Test-Path "\\YOUR-PC-NAME\SymbolServer"
```

출력: `True` (성공) / `False` (실패)

### 4.2 심볼 업로드 확인

빌드 후 Output 창에서 확인:

```
[SymbolUpload] Uploading symbols to server...
[SymbolUpload] Upload successful: Mundi.pdb
```

## 4. 문제 해결

### 문제 1: "Symbol server not accessible"

**해결:**
1. 네트워크 공유 폴더 접근 권한 확인
2. 방화벽 설정 확인
3. 공유 폴더 경로 확인 (`\\PC-NAME\SymbolServer`)

### 문제 2: "PDB not found on symbol server"

**해결:**
1. 빌드를 수행한 팀원이 Post-Build 이벤트를 실행했는지 확인
2. 수동 업로드:
   ```powershell
   cd C:\Users\Jungle\Desktop\GTL_Week12\Scripts
   .\UploadSymbols.ps1 -ExePath "..\Binaries\Debug\Mundi.exe" -PdbPath "..\Binaries\Debug\Mundi.pdb" -SymbolServer "\\YOUR-PC-NAME\SymbolServer"
   ```

### 문제 3: "symstore.exe not found"

**해결:**
Windows SDK 설치:
1. Visual Studio Installer 실행
2. Modify → Individual Components
3. "Debugging Tools for Windows" 체크
4. Install

---
