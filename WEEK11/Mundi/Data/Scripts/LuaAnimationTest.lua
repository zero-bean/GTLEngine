
local SkeletalComp
local AnimInstance

local bWalk = true
local bJump = false
local bPunch = false
local bPunch2 = false
local Punch2Delay = 0.3
local CurPunch2Delay = 0
local bDance = false


local Speed = 0.75

local ForwardVector = Vector(1, 0, 0)
local UpVector = Vector(0, 0, 1)
local CameraLocation = Vector(0, 0, 0)

local BackDistance = 7.0
local UpDistance = 2.0

local YawSensitivity = 0.005
local PitchSensitivity = 0.0025

local MovementDelta = 10

WalkSpeed = 0.4
RunSpeed = 0.75

local StateCheckDelay = 0

function BeginPlay()
print("Begin")
SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
--State Anim 추가
SkeletalComp:AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Walk.fbx", WalkSpeed)
SkeletalComp:AddSequenceInState("Move", "Data/Animations/Standard Run.fbx", RunSpeed)
SkeletalComp:AddSequenceInState("Jump", "Data/Animations/Jumping.fbx", 0)
SkeletalComp:AddSequenceInState("Punch", "Data/Animations/Punching.fbx", 0)
SkeletalComp:AddSequenceInState("Punch2", "Data/Animations/Punching2.fbx", 0)
SkeletalComp:AddSequenceInState("Dance", "Data/Animations/Gangnam Style_blend_ascii.fbx", 0)

--State 설정
SkeletalComp:SetStateSpeed("Move", 1.7)
SkeletalComp:SetStateSpeed("Jump", 1.4)
SkeletalComp:SetStateLoop("Jump", false)    
SkeletalComp:SetStateExitTime("Jump", 0.55)   
SkeletalComp:SetStateSpeed("Punch", 2.5)
SkeletalComp:SetStateLoop("Punch", false)    
SkeletalComp:SetStateSpeed("Punch2", 2.5)
SkeletalComp:SetStateLoop("Punch2", false)    

--Transition
SkeletalComp:AddTransition("Idle", "Move", 0.2, function() return bWalk end)
SkeletalComp:AddTransition("Move", "Idle", 0.2, function() return bWalk==false end)
SkeletalComp:AddTransition("Idle", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Move", "Jump", 0.2, function() return bJump end)
SkeletalComp:AddTransition("Jump", "Idle", 0.1, nil)
SkeletalComp:AddTransition("Idle", "Punch", 0.25, function() return bPunch end)
SkeletalComp:AddTransition("Move", "Punch", 0.25, function() return bPunch end)
SkeletalComp:AddTransition("Punch", "Idle", 0.1, nil)
SkeletalComp:AddTransition("Punch", "Punch2", 0.2, function() return bPunch2 end)
SkeletalComp:AddTransition("Punch2", "Idle", 0.1, nil)
SkeletalComp:AddTransition("Idle", "Dance", 0.2, function() return bDance end)
SkeletalComp:AddTransition("Move", "Dance", 0.2, function() return bDance end)
SkeletalComp:AddTransition("Dance", "Move", 0.2, function() return bDance == false end)


--시작 State 설정
SkeletalComp:SetStartState("Idle")

-- 카메라 초기화
local Camera = GetCamera()
if Camera then
    Camera:SetCameraForward(ForwardVector)
end
ForwardVector = NormalizeCopy(ForwardVector)
end

function EndPlay()  
end

function SetCursorVisible(Show)
    InputManager:SetCursorVisible(Show)
    CurVisibilty = Show
end


function Tick(dt)
    if InputManager:IsKeyPressed("C") then
        SetCursorVisible(not CurVisibilty)
        end
_G.PlayerPosition = Obj.Location
    if  StateCheckDelay <= 0 then
    CurStateName = SkeletalComp:GetCurStateName()
    if bPunch == true and CurStateName ~= "Punch" then
    bPunch = false
    end
    if bJump == true and CurStateName ~= "Jump" then
    bJump = false
    end
    if bPunch2 == true and CurStateName ~= "Punch2" then
    bPunch2 = false
    end
    end

    --속도조절
    if InputManager:IsKeyDown('Q') then
    Speed = Speed - dt
    end
    if InputManager:IsKeyDown('E') then
    Speed = Speed + dt
    end

    --이동
    if InputManager:IsKeyDown('W') and bPunch == false and bPunch2 == false then
    bDance = false
    bWalk = true
    MoveForward(MovementDelta * dt * Speed) 
    else
    bWalk = false
    end

    --if InputManager:IsKeyDown('A') then
    --MoveRight(-MovementDelta * dt) 
    --end

    --if InputManager:IsKeyDown('D') then
    --MoveRight(MovementDelta * dt)
    --end

    --점프
    if InputManager:IsKeyPressed('F') and bJump == false and bDance == false then
    bJump = true
    StateCheckDelay = 0.1
    end
    
    --펀치
    if InputManager:IsKeyPressed("R") and bPunch == false and bPunch2 == false and bDance == false then
    bPunch = true
    CurPunch2Delay = Punch2Delay
    StateCheckDelay = 0.1
    end
    if InputManager:IsKeyPressed("R") and bPunch == true and bPunch2 == false and CurPunch2Delay < 0 and bDance == false then
    bPunch2 = true
    StateCheckDelay = 0.1
    end

    if InputManager:IsKeyPressed("T") and bPunch == false and bPunch2 == false and bDance == false then
    bDance = true
    end

    StateCheckDelay = StateCheckDelay - dt
    CurPunch2Delay = CurPunch2Delay - dt

    SkeletalComp:SetBlendValueInState("Move", Speed)
    if Speed < WalkSpeed then
    Speed = WalkSpeed
elseif Speed > RunSpeed then
    Speed = RunSpeed
end

    RotateCamera() -- 마우스로 카메라 회전
    UpdateCamera() -- 카메라 위치/방향 갱신
end

 -- ===== 카메라 관련 함수들 =====

 -- 벡터 정규화 (복사본 반환)
function NormalizeCopy(V)
    local Out = Vector(V.X, V.Y, V.Z)
    Out:Normalize()
    return Out
end

-- 축 기준 회전
function RotateAroundAxis(VectorIn, Axis, Angle)
    local UnitAxis = NormalizeCopy(Axis)
    local CosA, SinA = math.cos(Angle), math.sin(Angle)
    local AxisCrossVector = FVector.Cross(UnitAxis, VectorIn)
    local AxisDotVector   = FVector.Dot(UnitAxis, VectorIn)

    return VectorIn * CosA + AxisCrossVector * SinA + UnitAxis * (AxisDotVector * (1.0 - CosA))
end

-- 마우스 입력으로 카메라(=캐릭터) 회전
function RotateCamera()
    local MouseDelta = InputManager:GetMouseDelta()
    local MouseDeltaX = MouseDelta.X
    local MouseDeltaY = MouseDelta.Y

    -- 좌우 회전 (Yaw)
    local Yaw = MouseDeltaX * YawSensitivity
    ForwardVector = RotateAroundAxis(ForwardVector, UpVector, Yaw)
    ForwardVector = NormalizeCopy(ForwardVector)

    -- 상하 회전 (Pitch)
    local RightVector = FVector.Cross(UpVector, ForwardVector)
    RightVector = NormalizeCopy(RightVector) * -1.0

    local Pitch = MouseDeltaY * -PitchSensitivity
    local Candidate = RotateAroundAxis(ForwardVector, RightVector, Pitch)

    -- 수직 각도 제한
    if (Candidate.Z > 0.2) then
        Candidate.Z = 0.2
    end
    if (Candidate.Z < -0.45) then
        Candidate.Z = -0.45
    end

    ForwardVector = NormalizeCopy(Candidate)

    -- 캐릭터 회전 (지면 기준)
    local LookAt = Vector(ForwardVector.X, ForwardVector.Y, 0)
    SetPlayerForward(Obj, LookAt)
end

-- 캐릭터 앞으로 이동
function MoveForward(Delta)
    Obj.Location = Obj.Location + Vector(ForwardVector.X, ForwardVector.Y, 0) * Delta
end

function MoveRight(Delta)
    local RightVector = FVector.Cross(UpVector, ForwardVector)
    RightVector = NormalizeCopy(RightVector)
    Obj.Location = Obj.Location + Vector(RightVector.X, RightVector.Y, 0) * Delta
end

-- 카메라 위치/방향 갱신
function UpdateCamera()
    -- 1. 카메라를 캐릭터 뒤쪽에 배치
    local Camera = GetCamera()
    if Camera then
        CameraLocation = Obj.Location + (ForwardVector * -BackDistance) + (UpVector * UpDistance)
        Camera:SetLocation(CameraLocation)
        _G.CameraPosition = CameraLocation
        -- 2. 카메라가 캐릭터를 바라보도록 설정
        local Eye = CameraLocation
        local At = Obj.Location
        local Direction = Vector(At.X - Eye.X, At.Y - Eye.Y, At.Z - Eye.Z)
        Camera:SetCameraForward(Direction)
    end
end
