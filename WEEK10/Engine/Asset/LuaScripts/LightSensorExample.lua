------------------------------------------------------------------
-- LightSensorExample.lua
-- LightSensorComponent 기본 사용 예시
------------------------------------------------------------------
-- [사용 방법]
-- 1. Actor에 LightSensorComponent 추가
-- 2. Actor에 ScriptComponent 추가
-- 3. 이 스크립트를 ScriptComponent에 할당
------------------------------------------------------------------

function BeginPlay()
    Log("LightSensor Example Started")
    Log("Current Luminance: " .. tostring(GetCurrentLuminance()))
end

function Tick(dt)
    -- 매 프레임 현재 조명 세기 출력 (성능 테스트용)
    -- 실제 게임에서는 OnLightIntensityChanged 이벤트 사용 권장
end

---
-- 조명 세기가 변경되었을 때 호출됩니다.
-- @param current (float): 현재 측정된 Luminance 값 (0.0 ~ 1.0+)
-- @param previous (float): 이전 측정된 Luminance 값
---
function OnLightIntensityChanged(current, previous)
    local delta = current - previous

    -- 조명 변화 로그
    Log(string.format("Light Changed: %.3f -> %.3f (Delta: %.3f)",
        previous, current, delta))

    -- 밝기 상태 출력
    if current < 0.2 then
        Log("  Status: DARK - Hidden in shadows")
    elseif current < 0.5 then
        Log("  Status: DIM - Partially visible")
    elseif current < 0.8 then
        Log("  Status: BRIGHT - Clearly visible")
    else
        Log("  Status: VERY BRIGHT - Fully exposed!")
    end
end

---
-- LightSensorComponent에서 현재 Luminance 값을 가져오는 헬퍼 함수
-- @return (float): 현재 조명 세기
---
function GetCurrentLuminance()
    -- Component 접근 예시 (C++에서 바인딩 필요)
    -- 현재는 Delegate를 통해서만 값을 받을 수 있음
    return 0.0
end

function EndPlay()
    Log("LightSensor Example Ended")
end
