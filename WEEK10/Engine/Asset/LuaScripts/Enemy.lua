------------------------------------------------------------------
-- Enemy.lua
-- Enemy Actor의 기본 동작 스크립트 (Player 추적)
-- 사용: Enemy 템플릿 Actor에 ScriptComponent를 추가하고 이 스크립트 할당
------------------------------------------------------------------

local moveSpeed = 0.0  -- BeginPlay에서 랜덤 설정
local targetPlayer = nil
local bInitialized = false  -- 첫 Tick 체크용

-- Jump settings
local initialZ = 0.0
local jumpTime = 0.0
local jumpFrequency = 2.0  -- 점프 주기 (초)
local jumpHeight = 30.0    -- 점프 높이

-- Light-based tracking settings
local currentLightLevel = 0.0      -- 플레이어가 받는 현재 빛 세기
local targetPlayerLocation = nil   -- 플레이어의 현재 위치
local lightThreshold = 1.0         -- 이 값 이상일 때만 추적 시작

---
-- Player의 OnPlayerTracking delegate에서 호출되는 콜백
-- @param lightLevel 플레이어가 받는 현재 빛 세기
-- @param playerLocation 플레이어의 현재 위치 (FVector)
---
function Follow(lightLevel, playerLocation)
    currentLightLevel = lightLevel
    -- FVector 복사 (참조 문제 방지)
    targetPlayerLocation = FVector(playerLocation.X, playerLocation.Y, playerLocation.Z)
end

---
-- 첫 번째 Tick에서 Player를 찾고 delegate 바인딩
---
local function InitializeTracking()
    local world = GetWorld()
    local level = world and world:GetLevel()
    if level then
        local players = level:FindActorsOfClass("APlayer")
        if players and #players > 0 then
            local playerActor = players[1]
            targetPlayer = playerActor:ToAPlayer()  -- AActor*를 APlayer*로 캐스팅
            if targetPlayer then
                targetPlayer.OnPlayerTracking = Follow
                Log("[Enemy] Bound to Player's tracking delegate")
            else
                Log("[Enemy] WARNING: Player cast failed")
            end
        else
            Log("[Enemy] WARNING: Player not found")
        end
    end
end

---
-- Enemy가 생성될 때 호출됩니다.
---
function BeginPlay()
    -- 각 Enemy마다 다른 속도 설정 (15~25 사이)
    moveSpeed = 80.0 + math.random() * 40.0

    Log("[Enemy] Spawned at: " .. tostring(Owner.Location.X) .. ", " .. tostring(Owner.Location.Y) .. " with speed: " .. tostring(moveSpeed))

    -- 초기 Z 위치 저장
    initialZ = 10.0

    -- Player는 첫 Tick에서 찾기 (BeginPlay 순서 보장 안됨)

    -- Preload hit SFX so first playback has no delay
    if Sound_PreloadSFX ~= nil then
        Sound_PreloadSFX("EnemyHit", "Asset/Audio/SFX/EnemyHit.wav", false, 1.0, 30.0)
    end

    -- Preload & play spawn SFX on creation (only when game is running)
    local world = GetWorld()
    local gm = world and world:GetGameMode() or nil
    local isRunning = gm and gm.IsGameRunning or false
    if isRunning then
        if Sound_PreloadSFX ~= nil then
            Sound_PreloadSFX("EnemySpawn", "Asset/Audio/SFX/EnemySpawn.wav", false, 1.0, 30.0)
        end
        if Sound_PlaySFX ~= nil then
            Sound_PlaySFX("EnemySpawn", 1.0, 1.0)
        end
    end
end

---
-- 매 프레임 호출됩니다.
---
function Tick(dt)
    -- 첫 번째 Tick: Player 찾고 delegate 바인딩
    if not bInitialized then
        InitializeTracking()
        bInitialized = true
    end

    -- 게임이 종료되었으면 더 이상 추적/공격하지 않음
    local world = GetWorld()
    if world then
        local gameMode = world:GetGameMode()
        if gameMode and not gameMode.IsGameRunning then
            return
        end
    end

    local enemyPos = Owner.Location
    local newLocation = FVector(enemyPos.X, enemyPos.Y, enemyPos.Z)

    -- 점프는 항상 수행 (Sin 곡선)
    jumpTime = jumpTime + dt
    local jumpOffset = math.abs(math.sin(jumpTime * math.pi * jumpFrequency)) * jumpHeight
    newLocation.Z = initialZ + jumpOffset

    -- 추적은 light level이 threshold 이상일 때만 수행
    if targetPlayerLocation and currentLightLevel >= lightThreshold then
        -- X/Y 평면에서만 방향 계산 (Z축 무시)
        local direction = FVector(
            targetPlayerLocation.X - enemyPos.X,
            targetPlayerLocation.Y - enemyPos.Y,
            0.0  -- Z축은 항상 0으로 설정
        )

        direction:Normalize()  -- Normalize가 이미 0 나누기 처리함

        -- X/Y 평면에서만 이동
        newLocation.X = enemyPos.X + direction.X * moveSpeed * dt
        newLocation.Y = enemyPos.Y + direction.Y * moveSpeed * dt

        -- Player 방향을 바라보도록 회전 (X/Y 평면 기준)
        Owner.Rotation = FQuaternion.MakeFromDirection(direction)
    end
    -- else: 어두울 때는 제자리에서 점프만

    Owner.Location = newLocation
end

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    -- 게임이 종료되었으면 데미지 주지 않음
    local world = GetWorld()
    if world then
        local gameMode = world:GetGameMode()
        if gameMode and not gameMode.IsGameRunning then
            return
        end
    end

    Log("[Enemy] Overlap with: " .. otherActor.Name)

    -- Player와 충돌 시 처리
    if otherActor.Tag == CollisionTag.Player then
        Log("[Enemy] Hit player!")
        -- 여기에 플레이어에게 데미지를 주는 로직 추가 가능
        if Sound_PlaySFX ~= nil then
            Sound_PlaySFX("EnemyHit", 1.0, 1.0)
        end
    end
end

---
-- 다른 액터와 오버랩이 종료될 때 호출됩니다.
---
function OnActorEndOverlap(overlappedActor, otherActor)
    -- 필요시 구현
end

---
-- Enemy가 파괴될 때 호출됩니다.
---
function EndPlay()
    Log("[Enemy] EndPlay")
    targetPlayer = nil
end
