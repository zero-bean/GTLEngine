-- ============================================================================
-- FirefighterController.lua
-- Firefighter 캐릭터 애니메이션 컨트롤러: WASD 입력으로 Idle/Walking/Running 전환
-- ============================================================================

-- ============================================================================
-- 상태를 전역 테이블에 저장 (핫 리로드 시 보존됨)
-- ============================================================================
State = State or {}

-- 이동 속도 설정 (상수는 local로 유지)
local WALK_SPEED = 6.0    -- 기본 걷기 속도
local RUN_SPEED = 8.0     -- 달리기 속도

-- 애니메이션 상태 이름
local STATE_IDLE = "Idle"
local STATE_WALKING = "Walking"
local STATE_RUNNING = "Running"
local STATE_PICKUP = "PickUp"

-- 애니메이션 에셋 경로 (Without_Cloth 버전)
local ANIM_WITHOUT_CLOTH = {
    IDLE = "Data/firefighter/Firefighter_Without_Cloth_Idle_mixamo.com",
    WALKING = "Data/firefighter/Firefighter_Without_Cloth_Walking_mixamo.com",
    RUNNING = "Data/firefighter/Firefighter_Without_Cloth_Running_mixamo.com",
    PICKUP = "Data/firefighter/Firefighter_Without_Cloth_Picking_Up_Object_mixamo.com"
}

-- 애니메이션 에셋 경로 (With_Cloth 버전 - 소방복 장착 시)
local ANIM_WITH_CLOTH = {
    IDLE = "Data/firefighter/Firefighter_With_Cloth_Idle_mixamo.com",
    WALKING = "Data/firefighter/Firefighter_With_Cloth_Walking_mixamo.com",
    RUNNING = "Data/firefighter/Firefighter_With_Cloth_Running_mixamo.com",
    PICKUP = "Data/firefighter/Firefighter_With_Cloth_Picking_Up_Object_mixamo.com"
}

-- 메시 경로
local MESH_WITHOUT_CLOTH = "Data/firefighter/Firefighter_Without_Cloth.fbx"
local MESH_WITH_CLOTH = "Data/firefighter/Firefighter_With_Cloth.fbx"

-- 현재 애니메이션 세트 (기본: Without_Cloth)
local CurrentAnims = ANIM_WITHOUT_CLOTH

-- ============================================================================
-- 참조 초기화 함수 (BeginPlay와 HotReload에서 공통 사용)
-- ============================================================================
local function InitReferences()
    State.Character = GetOwnerAs(Obj, "ACharacter")
    State.SkeletalMeshComp = GetComponent(Obj, "USkeletalMeshComponent")
    State.MovementComp = GetComponent(Obj, "UCharacterMovementComponent")

    if State.SkeletalMeshComp then
        State.StateMachine = State.SkeletalMeshComp:GetOrCreateStateMachine()
    end
end

function OnBeginPlay()
    print("[FirefighterController] BeginPlay called")

    InitReferences()

    if State.Character then
        print("[FirefighterController] Character found!")
    else
        print("[FirefighterController] ERROR: Could not cast owner to ACharacter")
        return
    end

    if not State.SkeletalMeshComp then
        print("[FirefighterController] ERROR: No USkeletalMeshComponent found")
        return
    end

    if State.MovementComp then
        print("[FirefighterController] MovementComponent found!")
        State.MovementComp.JumpZVelocity = 5.0
    end

    -- 상태 초기화
    State.bIsMoving = false
    State.bIsRunning = false
    State.bIsPerformingAction = false
    State.bHasFireSuit = false  -- 소방복 장착 상태

    -- 애니메이션 상태 머신 설정
    SetupAnimationStateMachine()
end

-- ============================================================================
-- 핫 리로드 콜백 (스크립트 파일 변경 시 호출됨)
-- ============================================================================
function OnHotReload()
    print("[FirefighterController] === HOT RELOAD ===")
    InitReferences()
    --SetupAnimationStateMachine()
    print("[FirefighterController] References re-initialized")
end

function SetupAnimationStateMachine()
    print("[FirefighterController] Setting up Animation State Machine")

    -- 상태 머신 사용 활성화 및 인스턴스 가져오기
    State.SkeletalMeshComp:UseStateMachine()
    State.StateMachine = State.SkeletalMeshComp:GetOrCreateStateMachine()

    if not State.StateMachine then
        print("[FirefighterController] ERROR: Failed to create state machine")
        return
    end

    -- 기존 상태들 클리어 (메시 변경 후 재설정 시 필요)
    State.StateMachine:Clear()

    -- 상태 추가 (이름, 에셋 경로, 재생 속도, 루핑 여부)
    local idleIdx = State.StateMachine:AddState(STATE_IDLE, CurrentAnims.IDLE, 1.0, true)
    local walkIdx = State.StateMachine:AddState(STATE_WALKING, CurrentAnims.WALKING, 1.0, true)
    local runIdx = State.StateMachine:AddState(STATE_RUNNING, CurrentAnims.RUNNING, 1.5, true)  -- 달리기 속도 1.5배
    local pickupIdx = State.StateMachine:AddState(STATE_PICKUP, CurrentAnims.PICKUP, 1.0, false)  -- 루핑 안함

    print("[FirefighterController] States added: Idle=" .. tostring(idleIdx) .. ", Walking=" .. tostring(walkIdx) .. ", Running=" .. tostring(runIdx) .. ", PickUp=" .. tostring(pickupIdx))

    -- 전이(Transition) 추가 (BlendTime 0.2초)
    -- Idle <-> Walking
    State.StateMachine:AddTransitionByName(STATE_IDLE, STATE_WALKING, 0.2)
    State.StateMachine:AddTransitionByName(STATE_WALKING, STATE_IDLE, 0.2)

    -- Idle <-> Running
    State.StateMachine:AddTransitionByName(STATE_IDLE, STATE_RUNNING, 0.2)
    State.StateMachine:AddTransitionByName(STATE_RUNNING, STATE_IDLE, 0.2)

    -- Walking <-> Running
    State.StateMachine:AddTransitionByName(STATE_WALKING, STATE_RUNNING, 0.15)
    State.StateMachine:AddTransitionByName(STATE_RUNNING, STATE_WALKING, 0.15)

    -- PickUp 전이 (모든 상태에서 PickUp으로, PickUp에서 Idle로)
    State.StateMachine:AddTransitionByName(STATE_IDLE, STATE_PICKUP, 0.1)
    State.StateMachine:AddTransitionByName(STATE_WALKING, STATE_PICKUP, 0.1)
    State.StateMachine:AddTransitionByName(STATE_RUNNING, STATE_PICKUP, 0.1)
    State.StateMachine:AddTransitionByName(STATE_PICKUP, STATE_IDLE, 0.2)

    -- 초기 상태: Idle
    State.StateMachine:SetState(STATE_IDLE, 0.0)

    print("[FirefighterController] Animation State Machine setup complete!")
end

-- ============================================================================
-- 소방복 장착 함수 (외부에서 호출 가능)
-- ============================================================================
function EquipFireSuit()
    if State.bHasFireSuit then
        print("[FirefighterController] Already wearing fire suit!")
        return
    end

    print("[FirefighterController] Equipping Fire Suit...")

    -- 현재 상태와 시간 저장 (애니메이션 이어서 재생용)
    local prevStateName = nil
    local prevStateTime = 0
    if State.StateMachine then
        prevStateName = State.StateMachine:GetCurrentStateName()
        if prevStateName and prevStateName ~= "" then
            prevStateTime = State.StateMachine:GetStateTime(prevStateName)
            print("[FirefighterController] Saving state: " .. prevStateName .. " at time " .. prevStateTime)
        end
    end

    State.bHasFireSuit = true
    -- bIsPerformingAction은 유지 (픽업 중이면 계속 픽업 상태)
    CurrentAnims = ANIM_WITH_CLOTH

    -- 메시 변경
    if State.SkeletalMeshComp then
        State.SkeletalMeshComp:SetSkeletalMesh(MESH_WITH_CLOTH)
        print("[FirefighterController] Mesh changed to With_Cloth")
    end

    -- 참조 재초기화 (메시 변경 후 필요할 수 있음)
    InitReferences()

    -- 상태 머신 재설정 (새 애니메이션 경로로)
    SetupAnimationStateMachine()

    -- 이전 상태와 시간 복원 (애니메이션 이어서 재생)
    if prevStateName and prevStateName ~= "" and State.StateMachine then
        State.StateMachine:SetState(prevStateName, 0)  -- 즉시 전환 (블렌드 없음)
        State.StateMachine:SetStateTime(prevStateName, prevStateTime)
        print("[FirefighterController] Restored state: " .. prevStateName .. " at time " .. prevStateTime)
    end

    print("[FirefighterController] Fire Suit equipped!")
end

-- 소방복 장착 여부 확인 함수
function HasFireSuit()
    return State.bHasFireSuit or false
end

function Update(DeltaTime)
    if not State.Character or not State.StateMachine then
        return
    end

    -- 소방복 장착 플래그 체크 (GameInstance에서)
    if not State.bHasFireSuit then
        local gi = GetGameInstance()
        if gi and gi:GetBool("bEquipFireSuit", false) then
            EquipFireSuit()
            gi:SetBool("bEquipFireSuit", false)  -- 플래그 리셋
        end
    end

    -- 현재 프레임 이동 상태 초기화
    State.bIsMoving = false
    State.bIsRunning = false

    -- 액션 수행 중이면 이동 불가 및 다른 입력 무시
    if State.bIsPerformingAction then
        if State.MovementComp then
            State.MovementComp.MaxWalkSpeed = 0
        end
        return
    end

    -- 왼쪽 마우스 클릭으로 줍기 (LButton = 0)
    if Input:IsMouseButtonPressed(0) then
        print("[FirefighterController] PickUp triggered!")
        State.bIsPerformingAction = true
        State.StateMachine:SetState(STATE_PICKUP, 0.1)
        return
    end

    -- Shift 키로 달리기 (VK_SHIFT = 16)
    if Input:IsKeyDown(16) then
        State.bIsRunning = true
    end

    -- WASD 입력 체크 (W=87, A=65, S=83, D=68)
    if Input:IsKeyDown(87) or Input:IsKeyDown(65) or
       Input:IsKeyDown(83) or Input:IsKeyDown(68) then
        State.bIsMoving = true
    end

    -- 이동 속도 조절
    if State.MovementComp then
        if State.bIsRunning then
            State.MovementComp.MaxWalkSpeed = RUN_SPEED
        else
            State.MovementComp.MaxWalkSpeed = WALK_SPEED
        end
    end

    -- 애니메이션 상태 업데이트
    UpdateAnimationState()
end

function UpdateAnimationState()
    if not State.StateMachine then
        return
    end

    local currentState = State.StateMachine:GetCurrentStateName()
    local desiredState = STATE_IDLE

    if State.bIsMoving then
        if State.bIsRunning then
            desiredState = STATE_RUNNING
        else
            desiredState = STATE_WALKING
        end
    end

    -- 상태가 다르면 전환
    if currentState ~= desiredState then
        State.StateMachine:SetState(desiredState, -1.0)  -- -1.0: 전이 시간 사용
    end
end

-- AnimNotify 콜백 (C++에서 호출됨)
function OnAnimNotify(NotifyName)
    print("[FirefighterController] AnimNotify received: " .. tostring(NotifyName))

    -- EndPickUp 노티파이 처리: 액션 종료, Idle로 전환
    if NotifyName == "EndPickUp" then
        print("[FirefighterController] EndPickUp notify triggered! Returning to Idle.")
        State.bIsPerformingAction = false
        if State.StateMachine then
            State.StateMachine:SetState(STATE_IDLE, 0.2)
        end
    end
end

function OnEndPlay()
    print("[FirefighterController] EndPlay called")
    State.Character = nil
    State.SkeletalMeshComp = nil
    State.StateMachine = nil
    State.MovementComp = nil
end
