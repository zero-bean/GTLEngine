-- Simple BGM controller: starts at game start, stops at game end

local BGM_PATH = "Asset/Audio/BGM/BGM_InGame.wav"
local BGM_LOOP = true
local BGM_FADE = 0.5
local STOP_FADE = 0.3

local world = GetWorld()
if not world then
    Log("[Sound.lua] GetWorld() is nil; skipping sound hooks")
    return
end

local gameMode = world:GetGameMode()
if not gameMode then
    Log("[Sound.lua] GetGameMode() is nil; skipping sound hooks")
    return
end

-- Hook game init/end to play/stop BGM
gameMode.OnGameInited = function()
    Sound_PlayBGM(BGM_PATH, BGM_LOOP, BGM_FADE)
    Log("[Sound.lua] BGM start (OnGameInited): " .. BGM_PATH)
end

gameMode.OnGameEnded = function()
    -- Stop everything on game end
    if Sound_StopAll ~= nil then
        Sound_StopAll()
    else
        Sound_StopBGM(STOP_FADE)
    end
    Log("[Sound.lua] Stop All Sounds")
end
