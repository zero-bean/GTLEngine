------------------------------------------------------------------
-- EnemySpawner.lua
-- AEnemySpawnerActor의 Delegate를 통해 Enemy를 스폰하는 스크립트
-- 사용: AEnemySpawnerActor에 ScriptComponent를 추가하고 이 스크립트 할당
------------------------------------------------------------------

-- Enemy 템플릿 배열 (여러 종류)
local enemyTemplates = {}
local enemyTemplateNames = {"Enemy1", "Enemy2"}  -- 사용할 템플릿 이름들

-- 설정
local spawnRadiusMin = 80.0           -- Player로부터 최소 거리
local spawnRadiusMax = 200.0           -- Player로부터 최대 거리
local spawnDelay = 3.0                 -- 스폰 간격 (초)
local spawnTimer = 0.0                 -- 스폰 타이머
local maxEnemies = 10                  -- 최대 Enemy 개수

-- 생성된 Enemy 추적 (WeakAActor 사용)
local spawnedEnemies = {}              -- WeakAActor 배열

-- Player 참조
local world = nil
local GameMode = nil

-- 무효한 Enemy들을 배열에서 제거
local function CleanupInvalidEnemies()
    local validEnemies = {}
    local removedCount = 0

    for i, weakEnemy in ipairs(spawnedEnemies) do
        if weakEnemy:IsValid() then
            table.insert(validEnemies, weakEnemy)
        else
            removedCount = removedCount + 1
        end
    end

    spawnedEnemies = validEnemies

    if removedCount > 0 then
        Log("[EnemySpawner] Cleaned up " .. tostring(removedCount) .. " invalid enemies")
    end
end

function BeginPlay()
    world = GetWorld()
    if not world then
        return
    end

    GameMode = world:GetGameMode()
    if not GameMode then
        return
    end
    GameMode.OnGameEnded = OnGameReset

    -- Template Actor는 Editor World에만 존재하므로
    local editorWorld = world:GetSourceEditorWorld()
    if not editorWorld then
        -- Editor World인 경우 그대로 사용
        editorWorld = world
    end

    local level = editorWorld:GetLevel()
    if level then
        -- 여러 Enemy 템플릿 로드
        for i, templateName in ipairs(enemyTemplateNames) do
            local found = level:FindTemplateActorByName(templateName)
            if found ~= nil then
                table.insert(enemyTemplates, found)
                Log("[EnemySpawner] Cached template '" .. templateName .. "'")
            else
                Log("[EnemySpawner] WARNING: Could not find template actor named '" .. templateName .. "'")
            end
        end

        if #enemyTemplates == 0 then
            Log("[EnemySpawner] ERROR: No enemy templates found!")
        end
    end

    Log("[EnemySpawner] Ready for spawn requests with " .. tostring(#enemyTemplates) .. " templates")

    -- (moved) spawn SFX handled per-enemy in Enemy.lua BeginPlay
end

-- Owner의 OnEnemySpawnRequested Delegate가 호출할 콜백
function OnEnemySpawnRequested()
    -- 타이머 체크: 쿨다운 중이면 스폰하지 않음
    if spawnTimer > 0 then
        return  -- 아직 스폰 가능 시간이 아님
    end

    -- 무효한 Enemy 정리
    CleanupInvalidEnemies()

    -- 현재 살아있는 Enemy 개수 확인
    local currentEnemyCount = #spawnedEnemies

    if currentEnemyCount >= maxEnemies then
        Log("[EnemySpawner] Max enemy limit reached (" .. tostring(currentEnemyCount) .. "/" .. tostring(maxEnemies) .. "), spawn skipped")
        return
    end

    Log("[EnemySpawner] Spawn request received - spawning enemy (" .. tostring(currentEnemyCount + 1) .. "/" .. tostring(maxEnemies) .. ")")

    -- Player 위치 기준으로 랜덤 스폰
    local newEnemy = SpawnInternal()

    -- WeakAActor로 래핑하여 배열에 추가 (안전한 추적)
    if newEnemy ~= nil then
        local weakEnemy = MakeWeakAActor(newEnemy)
        table.insert(spawnedEnemies, weakEnemy)
    end

    -- 타이머 리셋
    spawnTimer = spawnDelay
end

-- Owner의 OnGameReset Delegate가 호출할 콜백 (게임 종료/리셋 시)
function OnGameReset()
    Log("[EnemySpawner] Game reset - destroying all enemies")

    local destroyedCount = 0

    -- 모든 생성된 Enemy 삭제
    for i, weakEnemy in ipairs(spawnedEnemies) do
        if weakEnemy:IsValid() then
            local enemy = weakEnemy:Get()
            if enemy then
                world:DestroyActor(enemy)
                destroyedCount = destroyedCount + 1
            end
        end
    end

    -- 배열 초기화
    spawnedEnemies = {}

    -- 타이머 리셋
    spawnTimer = 0.0

    Log("[EnemySpawner] Destroyed " .. tostring(destroyedCount) .. " enemies")
end

-- 내부 스폰 함수 (Player 위치 기준 반경 내 랜덤 스폰)
function SpawnInternal()
    if not GameMode then
        return nil
    end

    -- 템플릿 체크
    if #enemyTemplates == 0 then
        Log("[EnemySpawner] No templates available; spawn skipped")
        return nil
    end

    -- Player 체크 (nil이거나 삭제된 경우)
    local targetPlayer = GameMode:GetPlayer()
    if not targetPlayer then
        Log("[EnemySpawner] Player not found; spawn skipped")
        return nil
    end

    local world = GetWorld()
    local level = world and world:GetLevel() or nil
    if level == nil then
        Log("[EnemySpawner] Level not available; spawn skipped")
        return nil
    end

    -- 랜덤 템플릿 선택
    local randomIndex = math.random(1, #enemyTemplates)
    local selectedTemplate = enemyTemplates[randomIndex]

    -- Player 위치 기준으로 반경 내 랜덤 위치 계산
    local playerLocation = targetPlayer.Location
    local angle = math.random() * 2 * math.pi  -- 0 ~ 2π 랜덤 각도
    local radius = spawnRadiusMin + math.random() * (spawnRadiusMax - spawnRadiusMin)  -- Min~Max 사이 랜덤 거리

    local offsetX = math.cos(angle) * radius
    local offsetY = math.sin(angle) * radius
    local spawnLocation = FVector(playerLocation.X + offsetX, playerLocation.Y + offsetY, playerLocation.Z)

    -- Enemy 스폰
    local newEnemy = selectedTemplate:DuplicateFromTemplate(level, spawnLocation)
    if newEnemy ~= nil then
        Log("[EnemySpawner] Spawned enemy '" .. enemyTemplateNames[randomIndex] .. "' at: " .. tostring(spawnLocation.X) .. ", " .. tostring(spawnLocation.Y))
    else
        Log("[EnemySpawner] DuplicateFromTemplate failed")
    end
    return newEnemy
end

function Tick(dt)
    -- 스폰 타이머 감소
    if spawnTimer > 0 then
        spawnTimer = spawnTimer - dt
        if spawnTimer < 0 then
            spawnTimer = 0
        end
    end

    -- 주기적으로 무효한 Enemy 정리 (매 프레임 체크는 비효율적이므로 타이머와 함께)
    -- spawnTimer가 0이 될 때마다 정리 (즉, 스폰 간격마다)
end

function EndPlay()
    -- 필요 시 정리 (전역 API 제거는 생략: 핫리로드/재시작 시 최신 함수로 덮임)
end

