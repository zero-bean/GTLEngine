------------------------------------------------------------------
-- [사전 정의된 전역 변수]
-- Owner: 이 스크립트 컴포넌트를 소유한 C++ Actor (AActor) 객체입니다.
------------------------------------------------------------------
local World = nil
local GameMode = nil
local CameraManager = nil

-- 레터박스 & 대사 관련 변수
local isLetterBoxActive = false
local currentDialogue = ""
local dialogueAlpha = 0.0
local bInitialized = false  -- GameMode 초기화 완료 여부

---
-- 게임이 시작되거나 액터가 스폰될 때 1회 호출됩니다.
---
function BeginPlay()
    Log("[CinematicManager] BeginPlay called")
    World = GetWorld()
end

---
-- 대사 시퀀스를 재생하는 코루틴 (게임 시작 시)
---
function PlayDialogueSequence()
    local Color = FVector(0, 0, 0)
    local blendCurve = Curve("EaseOut")
    CameraManager:StartCameraFade(1, 0, 2, Color, false, blendCurve)

    Log("[CinematicManager] PlayDialogueSequence coroutine started!")

    local dialogues = {
        "다음 발제가 너무 궁금한 테크군...",
        "\"몰래 사무실에 들어가서 발제를 찾아보는건 어떨까?!!\"",
        "코치님들의 눈을 최대한 피해 들어가보자."
    }

    -- 레터박스 활성화
    Log("[CinematicManager] Enabling letterbox...")
    isLetterBoxActive = true
    if CameraManager then
        CameraManager:EnableLetterBox(2, 0.5) -- 0.5초 페이드 인
    end

    -- 레터박스 페이드 인 대기
    coroutine.yield(0.5)

    -- 3개의 대사를 각각 2초씩 표시
    for i = 1, #dialogues do
        currentDialogue = dialogues[i]
        dialogueAlpha = 1.0
        coroutine.yield(2.0) -- 2초 대기
    end

    -- 대사 페이드 아웃
    dialogueAlpha = 0.0
    currentDialogue = ""

    -- 레터박스 비활성화
    if CameraManager then
        CameraManager:DisableLetterBox(0.5) -- 0.5초 페이드 아웃
    end

    coroutine.yield(0.5)
    isLetterBoxActive = false

    Log("[CinematicManager] Cinematic finished, calling StartGame()")
    GameMode:StartGame()
end

---
-- 게임 오버 시퀀스를 재생하는 코루틴
---
function PlayGameOverSequence()
    Log("[CinematicManager] PlayGameOverSequence coroutine started!")

    local dialogues = {
        "앗! 코치님께 들켰다!",
        "\"다음 발제가 왜 궁금해요?\""
    }

    -- 카메라 뷰 타겟을 Owner로 변경
    local blendCurve = Curve("EaseOutBack")
    CameraManager:SetViewTargetWithBlend(Owner, 1.0, blendCurve)
    
    coroutine.yield(1.0) -- 카메라 전환 대기

    -- 레터박스 활성화
    Log("[CinematicManager] Enabling letterbox for Game Over...")
    isLetterBoxActive = true
    if CameraManager then
        CameraManager:EnableLetterBox(2, 0.5) -- 0.5초 페이드 인
    end

    -- 레터박스 페이드 인 대기
    coroutine.yield(0.5)

    -- 2개의 대사를 각각 3초씩 표시
    for i = 1, #dialogues do
        currentDialogue = dialogues[i]
        dialogueAlpha = 1.0
        coroutine.yield(3.0) -- 3초 대기
    end

    -- 대사 페이드 아웃
    dialogueAlpha = 0.0
    currentDialogue = ""

    -- 레터박스 비활성화
    if CameraManager then
        CameraManager:DisableLetterBox(0.5) -- 0.5초 페이드 아웃
    end

    coroutine.yield(0.5)
    isLetterBoxActive = false

    Log("[CinematicManager] Game Over sequence finished, calling EndGame()")
    GameMode:EndGame()
end

---
-- 매 프레임 호출됩니다.
-- @param dt (float): 이전 프레임으로부터 경과한 시간 (Delta Time)
---
function Tick(dt)
    -- 첫 Tick에서 GameMode/CameraManager 찾고 코루틴 시작
    if not bInitialized then
        if World then
            GameMode = World:GetGameMode()
            if GameMode then
                CameraManager = GameMode:GetPlayerCameraManager()
                if CameraManager then
                    Log("[CinematicManager] GameMode and CameraManager found, setting up delegates...")
                    bInitialized = true
                    Self:StartCoroutine("PlayDialogueSequence")

                    GameMode.OnGameStarted = RestartFadeIn
                    GameMode.OnGameOvered = function()
                        Self:StartCoroutine("PlayGameOverSequence")
                    end
                end
            end
        end
    end

    if isLetterBoxActive and currentDialogue ~= "" then
        DrawDialogue()
    end
end

---
-- 대사를 화면에 그립니다.
---
function DrawDialogue()
    -- 화면 크기 가져오기
    local screen_w = DebugDraw.GetViewportWidth()
    local screen_h = DebugDraw.GetViewportHeight()

    -- 화면 중앙 하단에 대사 표시
    local screen_center_x = screen_w / 2
    local dialogue_y = screen_h * 0.8 -- 화면 하단 20% 위치

    -- 대사 박스 배경 (반투명 검은색)
    local box_width = screen_w * 0.6 -- 화면 너비의 60%
    local box_height = screen_h * 0.1 -- 화면 높이의 10%
    local box_x = screen_center_x - box_width / 2
    local box_y = dialogue_y - box_height / 2

    DebugDraw.Rectangle(
        box_x, box_y, box_x + box_width, box_y + box_height,
        0.0, 0.0, 0.0, 0.7 * dialogueAlpha,
        true
    )

    -- 대사 텍스트
    local text_padding = box_width * 0.05 -- 양쪽 5% 여백
    DebugDraw.Text(
        currentDialogue,
        box_x + text_padding, box_y + box_height * 0.25,
        box_x + box_width - text_padding, box_y + box_height * 0.75,
        1.0, 1.0, 1.0, dialogueAlpha,
        20.0, true, true, "Consolas"
    )
end

function RestartFadeIn()
    local Color = FVector(0, 0, 0)
    local blendCurve = Curve("EaseOut")
    CameraManager:StartCameraFade(1, 0, 3, Color, false, blendCurve)
end

---
-- 게임이 종료되거나 액터가 파괴될 때 1회 호출됩니다.
---
function EndPlay()
    -- 레터박스가 활성화된 상태로 종료되면 정리
    if isLetterBoxActive and CameraManager then
        CameraManager:DisableLetterBox(0.0)
    end
end