------------------------------------------------------------------
-- CameraBlendVolume.lua
--
-- 이 볼륨(Owner)에 플레이어가 들어오면 지정된 카메라로 뷰를
-- 1초간 블렌드하고, 나가면 다시 플레이어 뷰로 되돌립니다.
--
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

-- 내부 상태 변수
local World = nil
local GameMode = nil
local CameraManager = nil
local bInitialized = false          -- 시스템(CameraManager, TargetCamera) 초기화 완료 여부
local OriginalViewTarget = nil      -- 뷰를 되돌리기 위해 저장할 원래 타겟 (플레이어)

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    World = GetWorld()
end

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    -- 첫 Tick에서 GameMode/CameraManager/TargetCamera를 찾습니다.
    if not bInitialized then
        if World then
            GameMode = World:GetGameMode()
            if GameMode then
                CameraManager = GameMode:GetPlayerCameraManager()
                if CameraManager then
                    Log("[CameraBlendVolume] Found CameraManager.")
                    bInitialized = true -- 모든 준비 완료
                end
            end
        end
    end
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
end

------------------------------------------------------------------
-- [Delegate 이벤트 콜백]
------------------------------------------------------------------

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    if CameraManager == nil or OriginalViewTarget ~= nil then
        return
    end

    if otherActor.Tag == CollisionTag.Player then
        Log("[CameraBlendVolume] Player entered. Blending to This Actor")
        OriginalViewTarget = otherActor 
        local blendCurve = Curve("EaseOutBack")
        CameraManager:SetViewTargetWithBlend(Owner, 1.0, blendCurve)
        Self:StartCoroutine("EndCinematic")
    end
end

function EndCinematic()
    if CameraManager == nil then
        return
    end

    coroutine.yield(2.0)
    if OriginalViewTarget ~= nil then
        Log("[CameraBlendVolume] Blending back")
        local blendCurve = Curve("EaseInBack")
        CameraManager:SetViewTargetWithBlend(OriginalViewTarget, 1.0, blendCurve)
        OriginalViewTarget = nil
    end
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
---
function OnActorEndOverlap(overlappedActor, otherActor)
end
