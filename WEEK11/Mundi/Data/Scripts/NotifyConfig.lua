-- NotifyConfig.lua
-- Key: animation file path (as used by UAnimSequenceBase::GetFilePath())
-- Value: table mapping NotifyName -> function(event)

-- ============================================================================
-- Audio Helper Functions (defined in Lua, not C++)
-- These use the generic PlaySound() function provided by C++
-- ============================================================================

function PlayFootstep()
    PlaySound("Data/Audio/FootSound.wav", 1.0)
end

function PlayAutumnFootstepR()
    PlaySound("Data/Audio/AutumnFoot_R.wav", 1.0)
end

function PlayAutumnFootstepL()
    PlaySound("Data/Audio/AutumnFoot_L.wav", 1.0)
end

function PlayPunch()
    PlaySound("Data/Audio/Punch.wav", 1.0)
end

function PlayCoin()
    PlaySound("Data/Audio/Coin.wav", 1.0)
end


function Attack()
 if _G.Monsters == nil then return end
    for i = 1, #_G.Monsters do
        Monster = Monsters[i]
        local dx = Monster.Location.X - _G.PlayerPosition.X
        local dy = Monster.Location.Y - _G.PlayerPosition.Y
        local dz = Monster.Location.Z - _G.PlayerPosition.Z
        local distSq = dx*dx + dy*dy + dz*dz

        local PlayerToMonster = Vector(dx, dy, dz)
        PlayerToMonster:Normalize()
        local PlayerForward = _G.PlayerPosition - _G.CameraPosition
        PlayerForward.Z = 0
        PlayerForward:Normalize()
        local InDirection = (PlayerToMonster.X * PlayerForward.X + PlayerToMonster.Y * PlayerForward.Y + PlayerToMonster.Z * PlayerForward.Z) > 0.6
        if distSq <= 1.3 and InDirection then
        _G.MonsterHits[i] = true
        end
    end
end

-- function PlayJump()
--     PlaySound("Data/Audio/Shot.wav", 1.2)
-- end

-- function PlayLand()
--     PlaySound("Data/Audio/HitFireball.wav", 0.9)
-- end

-- ============================================================================
-- Notify Configuration
-- Maps animation sequences to notify handlers
-- ============================================================================

NotifyConfig = {
    ["Data/Animations/Standard Walk.fbx"] = {
        TEST1 = function(event)
            -- Now uses Lua-defined function
            PlayFootstep()
        end,

        WalkSound = function(event)
            PlayFootstep()
        end,

        TEST = function(event)
            PlayAutumnFootstepR()
        end,
        
        TEST3 = function(event)
            PlayAutumnFootstepL()
        end,

        TEST4 = function(event)
            PlayPunch()
        end,
        TEST5 = function(event)
            PlayCoin()
        end,
        
    },

    -- Example: add another sequence with same notify names, different behavior
    ["Data/Animations/Standard Run.fbx"] = {
        RunSound = function(event)
            -- Run animation plays louder footsteps
            PlayFootstep()

        end,
    },

    ["Data/Animations/Punching.fbx"] = {
        Punch = function(event)
            -- Run animation plays louder footsteps
            PlayPunch()
        end,
        TEST4 = function(event)
            PlayPunch()
        end,
        TEST = function(event)
            PlayPunch()
        end,
         Attack = function(event)
            -- Run animation plays louder footsteps
            Attack()
        end,
    },
     ["Data/Animations/Punching2.fbx"] = {
        Punch = function(event)
            -- Run animation plays louder footsteps
            PlayPunch()
        end,
         Attack = function(event)
            -- Run animation plays louder footsteps
            Attack()
        end,
    },
}

