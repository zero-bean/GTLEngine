------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------

local bStarted = false
local bGameEnded = false
local bPaused = false  -- 일시정지 상태
local bEscPressedLastFrame = false  -- ESC 키 중복 입력 방지
local bRPressedLastFrame = false  -- R 키 중복 입력 방지
local bQPressedLastFrame = false  -- Q 키 중복 입력 방지

-- [Cached References] - BeginPlay에서 초기화
local cachedWorld = nil
local cachedLevel = nil
local cachedEnemySpawner = nil
local scriptComponent = nil  -- Player의 ScriptComponent (Delegate 등록용)

-- [Movement]
local moveSpeed = 100.0
local rotationSpeed = 30.0
local currentRotation = FVector(0, 0, 0)
local InitLocation = FVector(0, 0, 0)

-- [Health]
local MaxHP = 5
local currentHP = 0
local gameMode = nil
local PlayerCameraManager = nil
local lastHitTime = -1.0

-- [Light Exposure]
local LightCriticalPoint = 1.0
local MaxLightExposureTime = 3.0
local CurrentLightExposureTime = 3.0
local bLightWarningShown = false
local CurrentLightLevel = 0.0

-- [Time & Score]
local MaxTime = 180.0  -- 제한 시간 (3분)
local remainingTime = 180.0
local elapsedTime = 0.0
local finalScore = 0

---
-- [Health] HP가 0 이하가 되었는지 확인하고 GameMode의 OverGame을 호출
---
local function CheckForDeath()
    if currentHP <= 0 then
        if gameMode and gameMode.IsGameRunning then
            Log("PlayerHealth: HP is 0. Calling OverGame.")
            gameMode:OverGame()
        end
    end
end

---
-- [Health] 플레이어에게 데미지를 입힘
---
local function TakeDamage(damage)
    if currentHP <= 0 or remainingTime < 0 then return end
    if elapsedTime - lastHitTime < 1.0 then return end

    lastHitTime = elapsedTime

    currentHP = currentHP - damage

    local shake = UCameraModifier_CameraShake()
    shake:SetDuration(0.3)
    shake:SetIntensity(0.2)
    shake:SetBlendInTime(0.1)
    shake:SetBlendOutTime(0.1)
    shake:SetAlphaInCurve(Curve("EaseIn"))
    shake:SetAlphaOutCurve(Curve("EaseOut"))
    CurrentShake = PlayerCameraManager:AddCameraModifier(shake)

    Log(string.format("PlayerHealth: Took %d damage. HP remaining: %d", damage, currentHP))

    -- If dead now: stop warning loop and play fail SFX once
    if currentHP <= 0 then
        if Sound_StopLoopingSFX ~= nil and __warningLooping then
            Sound_StopLoopingSFX("Warning")
            __warningLooping = false
        end
        if Sound_PlaySFX ~= nil then
            Sound_PlaySFX("FailOnce", 1.0, 1.0)
        end
    end

    CheckForDeath()
end

---
-- [Game] 게임 종료 시퀀스 (EndGame 델리게이트에 연결)
---
function EndGameSequence()
    bGameEnded = true
    bStarted = false

    -- 점수 계산: 남은 시간이 많을수록 높은 점수
    -- 남은 시간 1초당 100점
    finalScore = math.floor(remainingTime * 100)

    Log(string.format("Game Ended! Remaining Time: %.2fs, Score: %d", remainingTime, finalScore))

    -- Stop BGM on game end
    if Sound_StopBGM ~= nil then
        Sound_StopBGM(0.3)
    end
end

function OnLightIntensityChanged(current, previous)
    CurrentLightLevel = current

    -- Enemy에게 추적 정보 브로드캐스트
    local player = Owner:ToAPlayer()
    if player then
        player:BroadcastTracking(current, Owner.Location)
    end

    local delta = current - previous
    if delta > 0 then
        Log(string.format("Light Changed: %.3f -> %.3f (Delta: %.3f)", previous, current, delta))
    end
end

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    bStarted = false
    bGameEnded = false

    -- [Cached References] 초기화
    cachedWorld = GetWorld()
    if cachedWorld then
        cachedLevel = cachedWorld:GetLevel()
        gameMode = cachedWorld:GetGameMode()
        local PlayerCameraManagers = cachedLevel:FindActorsOfClass("APlayerCameraManager")
        if PlayerCameraManagers and #PlayerCameraManagers > 0  then
            PlayerCameraManager = PlayerCameraManagers[1]:ToAPlayerCameraManager()
        end

        -- EnemySpawner 캐싱
        if cachedLevel then
            local spawnerActor = cachedLevel:FindActorByName("EnemySpawner")
            if spawnerActor then
                cachedEnemySpawner = spawnerActor:ToAEnemySpawnerActor()
                if cachedEnemySpawner then
                    Log("[Player] Cached EnemySpawner")
                else
                    Log("[Player] WARNING: EnemySpawner cast failed")
                end
            else
                Log("[Player] WARNING: EnemySpawner not found")
            end
        end

        gameMode.OnGameStarted = StartGame
        gameMode.OnGameEnded = EndGameSequence  -- EndGame 시 UI 표시
    end

    -- Preload result SFX so they play instantly on end
    if Sound_PreloadSFX ~= nil then
        Sound_PreloadSFX("FailOnce", "Asset/Audio/SFX/Fail.wav", false, 1.0, 30.0)
        Sound_PreloadSFX("SuccessOnce", "Asset/Audio/SFX/Success.wav", false, 1.0, 30.0)
    end
end

---
-- 매 프레임 호출됩니다.
---
function Tick(dt)
    -- 게임 종료 화면
    if bGameEnded then
        DrawGameOverUI()

        -- Space 키로 재시작
        if IsKeyDown(Keys.Space) then
            RestartGame()
        end
        return
    end

    -- 게임 진행 중
    if bStarted == false then
        return
    end

    -- ESC 키로 일시정지 및 재개
    local bEscDown = IsKeyDown(Keys.Esc)

    if bEscDown and not bEscPressedLastFrame then
        bPaused = not bPaused
        Log(string.format("Game %s", bPaused and "Paused" or "Resumed"))
    end
    bEscPressedLastFrame = bEscDown

    -- 일시정지 중일 때
    if bPaused then
        DrawPauseMenu()
        return
    end

    -- 제한 시간 감소
    remainingTime = remainingTime - dt
    elapsedTime = elapsedTime + dt

    -- 시간 초과 체크
    if remainingTime <= 0 then
        remainingTime = 0
        Log("Time Over! Game Failed.")
        if gameMode then
            gameMode:OverGame()  -- OverGame만 호출
        end
        return
    end

    Movement(dt)
    UpdateLightExposure(dt)

    -- HP가 0보다 클 때만 빨간 오버레이 표시
    if currentHP > 0 then
        DrawDangerOverlay()
    end

    DrawUI()
end

function Movement(dt)
    --- [Movement] 이동 ---
    local ForwardScale = 0.0
    local RightScale = 0.0
    local Forward = Owner:GetActorForwardVector()
    local Right = Owner:GetActorRightVector()

    if IsKeyDown(Keys.W) then
        ForwardScale = ForwardScale + 1.0
    end
    if IsKeyDown(Keys.S) then
        ForwardScale = ForwardScale - 1.0
    end
    if IsKeyDown(Keys.A) then
        RightScale = RightScale - 1.0
    end
    if IsKeyDown(Keys.D) then
        RightScale = RightScale + 1.0
    end
    MoveDirection = Forward * ForwardScale + Right * RightScale
    MoveDirection.Z = 0

    if MoveDirection:Length() > 0.0 then
        MoveDirection:Normalize()
        local targetLocation = Owner.Location + (MoveDirection * moveSpeed * dt)

        local world = GetWorld()
        local level = nil
        if world then
            level = world:GetLevel()
        end

        local hitResult = nil
        if level then
            hitResult = level:SweepActorSingle(Owner, targetLocation, CollisionTag.Wall)
        end

        if hitResult == nil then
            Owner.Location = targetLocation
        else
            -- Log("Movement blocked by wall.")
        end
    end

    --- [Movement] 회전 ---
    local mouseDelta = GetMouseDelta()

    currentRotation.X = 0
    local newPitch = currentRotation.Y - (mouseDelta.Y * rotationSpeed * dt)
    currentRotation.Y = Clamp(newPitch, -89.0, 89.0)
    currentRotation.Z = currentRotation.Z + (mouseDelta.X * rotationSpeed * dt)

    Owner.Rotation = FQuaternion.FromEuler(currentRotation)
end

---
-- [UI] 위험 상황 전체 화면 오버레이
---
function DrawDangerOverlay()
    -- Light Critical Point를 넘어섰을 때만 표시
    if CurrentLightLevel < LightCriticalPoint then
        return
    end

    -- 빠르게 깜빡이는 효과
    local pulse = math.abs(math.sin(remainingTime * 8.0))

    -- 노출 시간이 적을수록 더 강하게 깜빡임
    local intensity = 0.0
    if CurrentLightExposureTime < MaxLightExposureTime then
        local danger_ratio = 1.0 - (CurrentLightExposureTime / MaxLightExposureTime)
        intensity = danger_ratio * 0.4  -- 최대 40% 불투명도
    end

    -- 화면 전체를 덮는 빨간 오버레이
    if intensity > 0.05 then
        local alpha = intensity * pulse
        local screen_w = DebugDraw.GetViewportWidth()
        local screen_h = DebugDraw.GetViewportHeight()

        DebugDraw.Rectangle(
            0, 0, screen_w, screen_h,
            1.0, 0.0, 0.0, alpha,  -- 빨간색, 가변 투명도
            true
        )
    end
end

---
-- [Light Exposure] 매 프레임 빛 노출 시간 업데이트
---
function UpdateLightExposure(dt)
    -- If dead, stop warning loop immediately and skip
    if currentHP ~= nil and currentHP <= 0 then
        if Sound_StopLoopingSFX ~= nil and __warningLooping then
            Sound_StopLoopingSFX("Warning")
            __warningLooping = false
        end
        return
    end
    -- Do nothing if game not running; ensure warning sound stops
    local isRunning = false
    do
        local world = GetWorld()
        local gm = world and world:GetGameMode() or nil
        if gm and gm.IsGameRunning then isRunning = true end
    end
    if not isRunning then
        if Sound_StopLoopingSFX ~= nil and __warningLooping then
            Sound_StopLoopingSFX("Warning")
            __warningLooping = false
        end
        return
    end

    -- Warning loop SFX toggle at threshold 1.5s (only when running)
    local threshold = 1.5
    if CurrentLightExposureTime < threshold then
        if Sound_PlayLoopingSFX ~= nil and not __warningLooping then
            Sound_PlayLoopingSFX("Warning", "Asset/Audio/SFX/Warning.wav", 1.0)
            __warningLooping = true
        end
    else
        if Sound_StopLoopingSFX ~= nil and __warningLooping then
            Sound_StopLoopingSFX("Warning")
            __warningLooping = false
        end
    end

    if CurrentLightLevel >= LightCriticalPoint then
        CurrentLightExposureTime = CurrentLightExposureTime - dt

        if CurrentLightExposureTime < 0 then
            CurrentLightExposureTime = 0
        end

        if CurrentLightExposureTime <= 0 then
            RequestSpawnEnemy()

            if not bLightWarningShown then
                Log("WARNING: Light Exposure Time has reached 0! Spawning enemies!")
                bLightWarningShown = true
            end
        end
    else
        CurrentLightExposureTime = CurrentLightExposureTime + dt

        if CurrentLightExposureTime > MaxLightExposureTime then
            CurrentLightExposureTime = MaxLightExposureTime
        end

        if CurrentLightExposureTime > 0 then
            bLightWarningShown = false
        end
    end
end

---
-- [UI] 게임 플레이 중 UI
---
function DrawUI()
    local ui_x = 80.0
    local ui_y = 60.0
    local bar_width = 200.0
    local bar_height = 20.0
    local bar_spacing = 10.0

    -- ============ TIME DISPLAY ============
    local time_y = ui_y - 30.0

    -- 남은 시간을 분:초 형식으로 변환
    local minutes = math.floor(remainingTime / 60)
    local seconds = math.floor(remainingTime % 60)
    local time_text = string.format("남은 시간: %d:%02d", minutes, seconds)

    -- 시간이 30초 미만이면 빨간색으로 경고
    local time_r, time_g, time_b = 1.0, 1.0, 0.3
    if remainingTime < 30.0 then
        local time_pulse = 0.5 + 0.5 * math.abs(math.sin(remainingTime * 5.0))
        time_r, time_g, time_b = 1.0, time_pulse * 0.3, 0.0
    end

    DebugDraw.Text(
        time_text,
        ui_x, time_y, ui_x + bar_width, time_y + 25.0,
        time_r, time_g, time_b, 1.0,
        16.0, true, false, "Consolas"
    )

    -- ============ HP BAR ============
    local hp_ratio = currentHP / MaxHP

    local hp_c_r, hp_c_g, hp_c_b = 0.1, 1.0, 0.1
    if hp_ratio <= 0.3 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 0.1, 0.1
    elseif hp_ratio <= 0.6 then
        hp_c_r, hp_c_g, hp_c_b = 1.0, 1.0, 0.1
    end

    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + bar_width, ui_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )

    local hp_fill_width = bar_width * hp_ratio
    DebugDraw.Rectangle(
        ui_x, ui_y, ui_x + hp_fill_width, ui_y + bar_height,
        hp_c_r, hp_c_g, hp_c_b, 1.0,
        true
    )

    local bar_end_x = ui_x + bar_width
    local bar_end_y = ui_y + bar_height
    DebugDraw.Line(ui_x, ui_y, bar_end_x, ui_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, bar_end_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, ui_y, ui_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, ui_y, bar_end_x, bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)

    local hp_text = string.format("체력: %d / %d", currentHP, MaxHP)
    DebugDraw.Text(
        hp_text,
        ui_x, ui_y, bar_end_x, bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        14.0, true, true, "Consolas"
    )

    -- ============ LIGHT EXPOSURE BAR ============
    local light_bar_y = ui_y + bar_height + bar_spacing
    local exposure_ratio = CurrentLightExposureTime / MaxLightExposureTime

    local light_c_r, light_c_g, light_c_b = 0.1, 0.5, 1.0
    if exposure_ratio <= 0.2 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.0, 0.0
    elseif exposure_ratio <= 0.5 then
        light_c_r, light_c_g, light_c_b = 1.0, 0.5, 0.0
    end

    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + bar_width, light_bar_y + bar_height,
        0.1, 0.1, 0.1, 0.8,
        true
    )

    local light_fill_width = bar_width * exposure_ratio
    DebugDraw.Rectangle(
        ui_x, light_bar_y, ui_x + light_fill_width, light_bar_y + bar_height,
        light_c_r, light_c_g, light_c_b, 1.0,
        true
    )

    local light_bar_end_y = light_bar_y + bar_height
    DebugDraw.Line(ui_x, light_bar_y, bar_end_x, light_bar_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_end_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(ui_x, light_bar_y, ui_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)
    DebugDraw.Line(bar_end_x, light_bar_y, bar_end_x, light_bar_end_y, 0.8, 0.8, 0.8, 1.0, 2.0)

    local light_text = string.format("빛 노출 가능 시간: %.1fs", CurrentLightExposureTime)
    DebugDraw.Text(
        light_text,
        ui_x, light_bar_y, bar_end_x, light_bar_end_y,
        1.0, 1.0, 1.0, 1.0,
        12.0, true, true, "Consolas"
    )

    if CurrentLightExposureTime < 1.0 then
        local pulse = math.abs(math.sin(remainingTime * 5.0))
        DebugDraw.Text(
            "! DANGER !",
            ui_x + bar_width + 15.0, light_bar_y, ui_x + bar_width + 150.0, light_bar_end_y,
            1.0, 0.0, 0.0, 0.5 + pulse * 0.5,
            14.0, true, false, "Arial"
        )
    end

    -- ============ CREDITS (좌하단) ============
    local screen_w = DebugDraw.GetViewportWidth()
    local screen_h = DebugDraw.GetViewportHeight()

    local credits_x = 20.0
    local credits_y = screen_h - 60.0
    local credits_w = 500.0
    local credits_h = 45.0

    -- 반투명 배경
    DebugDraw.Rectangle(
        credits_x, credits_y, credits_x + credits_w, credits_y + credits_h,
        0.0, 0.0, 0.0, 0.5,
        true
    )

    -- 제작진 텍스트
    DebugDraw.Text(
        "제작: Krafton GTL 2nd 국동희 김윤수 김희준 나예찬",
        credits_x + 8.0, credits_y, credits_x + credits_w - 8.0, credits_y + credits_h,
        0.8, 0.8, 0.8, 0.95,
        14.0, false, true, "Malgun Gothic"
    )
end

---
-- [UI] 게임 오버 화면
---
function DrawGameOverUI()
    -- 화면 전체를 덮는 반투명 배경 (뷰포트 크기 동적 가져오기)
    local screen_w = DebugDraw.GetViewportWidth()
    local screen_h = DebugDraw.GetViewportHeight()

    DebugDraw.Rectangle(
        0, 0, screen_w, screen_h,
        0.0, 0.0, 0.0, 0.7,  -- 어두운 배경
        true
    )

    -- 중앙 패널
    local panel_w = 800.0
    local panel_h = 500.0
    local panel_x = (screen_w - panel_w) / 2.0
    local panel_y = (screen_h - panel_h) / 2.0
    local padding = 40.0  -- 내부 여백

    DebugDraw.Rectangle(
        panel_x, panel_y, panel_x + panel_w, panel_y + panel_h,
        0.1, 0.1, 0.15, 0.95,
        true
    )

    -- 패널 테두리 (빛나는 효과)
    local pulse = 0.5 + 0.5 * math.abs(math.sin(remainingTime * 2.0))
    DebugDraw.Line(panel_x, panel_y, panel_x + panel_w, panel_y, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y + panel_h, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y, panel_x, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x + panel_w, panel_y, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)

    -- Title Text
    local content_x = panel_x + padding
    local content_w = panel_w - (padding * 2)
    local content_y = panel_y + padding

    local title_text = "??? : 다음 발제가 왜 궁금해요?"
    local title_color_r, title_color_g, title_color_b = 1.0, 0.2, 0.2

    local sub_text = "??? : 현재에 집중하세요!"
    local sub_color_r, sub_color_g, sub_color_b = 1.0, 0.0, 0.0

    if currentHP > 0 and remainingTime > 0 then
        title_text = "꿈에서 깼다..."
        title_color_r, title_color_g, title_color_b = 0.2, 1.0, 0.2
        sub_text = "일어나보니 발표 시간을 놓쳤다..."
        sub_color_r, sub_color_g, sub_color_b = 0.2, 1.0, 0.2
    end

    DebugDraw.Text(
        title_text,
        content_x, content_y + 20.0, content_x + content_w, content_y + 80.0,
        title_color_r, title_color_g, title_color_b, 1.0,
        48.0, true, true, "Arial"
    )

    DebugDraw.Text(
        sub_text,
        content_x, content_y + 100.0, content_x + content_w, content_y + 140.0,
        sub_color_r, sub_color_g, sub_color_b, 1.0,
        24.0, true, true, "Arial"
    )

    -- 시간 표시
    local minutes = math.floor(remainingTime / 60)
    local seconds = math.floor(remainingTime % 60)
    local time_text = string.format("남은 시간: %d:%02d", minutes, seconds)
    DebugDraw.Text(
        time_text,
        content_x, content_y + 170.0, content_x + content_w, content_y + 210.0,
        1.0, 1.0, 1.0, 1.0,
        24.0, false, true, "Consolas"
    )

    if currentHP > 0 and remainingTime > 0 then
        -- 점수 표시
        local score_text = string.format("점수: %d", finalScore)
        local score_y_start = content_y + 230.0

        DebugDraw.Text(
            score_text,
            content_x, score_y_start, content_x + content_w, score_y_start + 80.0,
            1.0, 0.84, 0.0, 1.0,  -- 금색
            44.0, true, true, "Arial"  -- 폰트 크기 44, 중앙 정렬
        )
    end

    -- 재시작 안내
    local restart_pulse = 0.6 + 0.4 * math.abs(math.sin(remainingTime * 3.0))
    local restart_y = panel_y + panel_h - padding - 50.0
    DebugDraw.Text(
        "Press SPACE to Restart",
        content_x, restart_y, content_x + content_w, restart_y + 40.0,
        1.0, 1.0, 1.0, restart_pulse,
        22.0, false, true, "Arial"
    )
end

---
-- [Game] 게임 재시작
---
function RestartGame()
    Log("Restarting game...")

    -- 게임 상태 초기화
    bGameEnded = false
    bStarted = true
    bPaused = false  -- 일시정지 해제
    bEscPressedLastFrame = false
    bRPressedLastFrame = false
    bQPressedLastFrame = false
    remainingTime = MaxTime
    finalScore = 0

    -- 플레이어 상태 초기화
    currentHP = MaxHP
    CurrentLightExposureTime = MaxLightExposureTime
    bLightWarningShown = false

    Owner.Location = InitLocation

    if gameMode then
        gameMode:StartGame()
    end

    Log("Game restarted successfully!")
end

---
-- 일시정지 메뉴
---
function DrawPauseMenu()
    -- 화면 크기 가져오기
    local screen_w = DebugDraw.GetViewportWidth()
    local screen_h = DebugDraw.GetViewportHeight()

    -- 반투명 배경
    DebugDraw.Rectangle(
        0, 0, screen_w, screen_h,
        0.0, 0.0, 0.0, 0.7,
        true
    )

    -- 중앙 메뉴 패널
    local panel_w = screen_w * 0.4
    local panel_h = screen_h * 0.6
    local panel_x = (screen_w - panel_w) / 2.0
    local panel_y = (screen_h - panel_h) / 2.0
    local padding = 40.0

    -- 패널 배경
    DebugDraw.Rectangle(
        panel_x, panel_y, panel_x + panel_w, panel_y + panel_h,
        0.1, 0.1, 0.15, 0.95,
        true
    )

    -- 패널 테두리
    local pulse = 0.5 + 0.5 * math.abs(math.sin(remainingTime * 2.0))
    DebugDraw.Line(panel_x, panel_y, panel_x + panel_w, panel_y, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y + panel_h, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x, panel_y, panel_x, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)
    DebugDraw.Line(panel_x + panel_w, panel_y, panel_x + panel_w, panel_y + panel_h, 0.2, 0.8, 1.0, pulse, 4.0)

    -- 타이틀
    local title_y = panel_y + padding
    DebugDraw.Text(
        "PAUSED",
        panel_x + padding, title_y, panel_x + panel_w - padding, title_y + 60.0,
        1.0, 1.0, 1.0, 1.0,
        48.0, true, true, "Arial"
    )

    -- 버튼 크기 및 위치
    local button_w = panel_w - (padding * 2)
    local button_h = 60.0
    local button_spacing = 20.0
    local button_x = panel_x + padding
    local first_button_y = panel_y + panel_h / 2.0 - button_h * 1.5 - button_spacing

    -- Resume 버튼 (ESC 키)
    local resume_y = first_button_y
    DebugDraw.Rectangle(button_x, resume_y, button_x + button_w, resume_y + button_h, 0.2, 0.3, 0.5, 0.6, true)
    DebugDraw.Line(button_x, resume_y, button_x + button_w, resume_y, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, resume_y + button_h, button_x + button_w, resume_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, resume_y, button_x, resume_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x + button_w, resume_y, button_x + button_w, resume_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Text(
        "Resume (ESC)",
        button_x, resume_y, button_x + button_w, resume_y + button_h,
        1.0, 1.0, 1.0, 1.0,
        24.0, true, true, "Arial"
    )

    -- Restart 버튼 (R 키)
    local restart_y = first_button_y + button_h + button_spacing
    DebugDraw.Rectangle(button_x, restart_y, button_x + button_w, restart_y + button_h, 0.5, 0.5, 0.2, 0.6, true)
    DebugDraw.Line(button_x, restart_y, button_x + button_w, restart_y, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, restart_y + button_h, button_x + button_w, restart_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, restart_y, button_x, restart_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x + button_w, restart_y, button_x + button_w, restart_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Text(
        "Restart (R)",
        button_x, restart_y, button_x + button_w, restart_y + button_h,
        1.0, 1.0, 1.0, 1.0,
        24.0, true, true, "Arial"
    )

    -- Quit 버튼 (Q 키)
    local quit_y = first_button_y + (button_h + button_spacing) * 2
    DebugDraw.Rectangle(button_x, quit_y, button_x + button_w, quit_y + button_h, 0.5, 0.2, 0.2, 0.6, true)
    DebugDraw.Line(button_x, quit_y, button_x + button_w, quit_y, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, quit_y + button_h, button_x + button_w, quit_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x, quit_y, button_x, quit_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Line(button_x + button_w, quit_y, button_x + button_w, quit_y + button_h, 1.0, 1.0, 1.0, 1.0, 2.0)
    DebugDraw.Text(
        "Quit (Q)",
        button_x, quit_y, button_x + button_w, quit_y + button_h,
        1.0, 1.0, 1.0, 1.0,
        24.0, true, true, "Arial"
    )

    -- 키 입력 처리
    local bEscDown = IsKeyDown(Keys.Esc)
    local bRDown = IsKeyDown(Keys.R)
    local bQDown = IsKeyDown(Keys.Q)

    -- ESC 키로 Resume (중복 방지는 Tick에서 처리)
    if bEscDown and not bEscPressedLastFrame then
        bPaused = false
        bEscPressedLastFrame = false
        bRPressedLastFrame = false
        bQPressedLastFrame = false
        Log("Game Resumed")
        return
    end

    -- R 키로 Restart
    if bRDown and not bRPressedLastFrame then
        bPaused = false
        bEscPressedLastFrame = false
        bRPressedLastFrame = false
        bQPressedLastFrame = false
        Log("Restarting game from pause menu...")
        RestartGame()
        return
    end
    bRPressedLastFrame = bRDown

    -- Q 키로 종료
    if bQDown and not bQPressedLastFrame then
        Log("Quit Game - Q key pressed, exiting...")
        ExitApplication()
    end
    bQPressedLastFrame = bQDown
end

function StartGame()
    local blendCurve = Curve("EaseOut")
    PlayerCameraManager:SetViewTargetWithBlend(Owner, 1.0, blendCurve)
    remainingTime = MaxTime
    finalScore = 0
    bStarted = true
    bGameEnded = false
    currentHP = MaxHP
    CurrentLightExposureTime = MaxLightExposureTime
    currentRotation = Owner.Rotation:ToEuler()
    InitLocation = Owner.Location

    -- Start BGM on game start (looped)
    if Sound_PlayBGM ~= nil then
        Sound_PlayBGM("Asset/Audio/BGM/BGM_InGame.wav", true, 0.5)
    end

    Log(string.format("Player.lua BeginPlay. Owner: %s, HP: %d", Owner.UUID, currentHP))
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
    bStarted = false
    Log("Player.lua EndPlay.")
end

---
-- 다른 액터와 오버랩이 시작될 때 호출됩니다.
---
function OnActorBeginOverlap(overlappedActor, otherActor)
    Log("Player: Overlap started with: " .. otherActor.Name)

    if otherActor.Tag == CollisionTag.Enemy then
        TakeDamage(1)
    elseif otherActor.Tag == CollisionTag.Clear then
        Log("Clear!")
        if Sound_PlaySFX ~= nil then
            Sound_PlaySFX("SuccessOnce", 1.0, 1.0)
        end
        -- 클리어 시에도 OverGame 호출 (시퀀스 먼저 재생 후 EndGame)
        if gameMode then
            gameMode:OverGame()
        end
    end
end

---
-- [Enemy Spawning] EnemySpawner에게 스폰 요청
---
-- [Enemy Spawn] 캐시된 EnemySpawner를 사용하여 Enemy 스폰 요청
---
function RequestSpawnEnemy()
    if cachedEnemySpawner then
        cachedEnemySpawner:RequestSpawn()
    else
        Log("[Player] WARNING: EnemySpawner not available")
    end
end
