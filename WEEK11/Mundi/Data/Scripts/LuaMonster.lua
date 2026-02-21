

local SkeletalComp
local AnimInstance

local bAttacked = false
local Idx = -1
function BeginPlay()
_G.Monsters = _G.Monsters or {}
 -- 자기 자신을 전역 Monsters 목록에 추가
table.insert(_G.Monsters, Obj)
Idx = #_G.Monsters
_G.MonsterHits = _G.MonsterHits or {}
table.insert(_G.MonsterHits, false)

SkeletalComp = GetComponent(Obj, "USkeletalMeshComponent")
SkeletalComp:AddSequenceInState("Idle", "Data/Animations/Breathing Idle.fbx", 0)
SkeletalComp:AddSequenceInState("Attacked", "Data/Animations/Head Hit.fbx", 0)

SkeletalComp:SetStateLoop("Attacked", false)    
SkeletalComp:SetStateSpeed("Attacked", 1.3)    

SkeletalComp:AddTransition("Idle", "Attacked", 0.2, function() return bAttacked end)
SkeletalComp:AddTransition("Attacked", "Idle", 0.1, nil)
SkeletalComp:SetStartState("Idle")

end

function EndPlay()  
end

function Tick(dt)
    CurStateName = SkeletalComp:GetCurStateName()
    if bAttacked == true and CurStateName ~= "Attacked" then
    bAttacked = false
    end
    if  _G.MonsterHits[Idx] == true then
    Attacked()
    _G.MonsterHits[Idx] = false
    end
end

function Attacked()
    PlaySound("Data/Audio/Die.wav", 1.0)
if bAttacked == true then
SkeletalComp:Replay(0.3)
else
bAttacked = true
end
local SceneComponent = GetComponent(Obj,"USceneComponent")
ToPlayer = _G.PlayerPosition - Obj.Location
ToPlayer:Normalize()
SceneComponent:SetForward(ToPlayer)
end
