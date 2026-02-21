--[[
    Sweep Test Example

    Demonstrates how to use Level:SweepActorSingle() and Level:SweepActorMulti()
    to test for collisions before actually moving an actor.

    Key Points:
    - Input: Actor (tests all PrimitiveComponents)
    - Output: Component (exact collision information)
    - Use cases: Wall collision prevention, Pre-movement validation, Area damage queries
--]]

local SweepTestExample = {}

-- Example 1: Single Sweep (Check if movement is safe)
function SweepTestExample:CheckMovementSafe(Actor, TargetLocation)
    local World = GetWorld()
    if not World then
        print("World not found!")
        return false
    end

    local Level = World:GetLevel()
    if not Level then
        print("Level not found!")
        return false
    end

    -- Test if moving Actor to TargetLocation would cause a collision
    -- Tests all PrimitiveComponents of the Actor
    local HitResult = Level:SweepActorSingle(Actor, TargetLocation)

    if HitResult then
        -- Collision detected!
        print("Movement blocked!")
        print("  Hit Location: (" .. HitResult.Location.X .. ", " .. HitResult.Location.Y .. ", " .. HitResult.Location.Z .. ")")
        print("  Hit Normal: (" .. HitResult.Normal.X .. ", " .. HitResult.Normal.Y .. ", " .. HitResult.Normal.Z .. ")")

        if HitResult.Actor then
            print("  Hit Actor: " .. HitResult.Actor.Name)
        end

        if HitResult.Component then
            print("  Hit Component: " .. HitResult.Component.Name)
        end

        return false  -- Movement would cause collision
    else
        -- No collision, safe to move
        print("Movement safe - no collision detected")
        return true
    end
end

-- Example 2: Multi Sweep (Get all potential collisions)
function SweepTestExample:GetAllPotentialCollisions(Actor, TargetLocation)
    local World = GetWorld()
    if not World then
        return {}
    end

    local Level = World:GetLevel()
    if not Level then
        return {}
    end

    -- Get all components that would overlap if Actor moved to TargetLocation
    local OverlappingComponents = Level:SweepActorMulti(Actor, TargetLocation)

    if OverlappingComponents then
        print("Found " .. #OverlappingComponents .. " potential collision(s)")

        for i, OtherComponent in ipairs(OverlappingComponents) do
            print("  [" .. i .. "] " .. OtherComponent.Name)

            -- You can access the component's owner actor
            local Owner = OtherComponent.Owner
            if Owner then
                print("      Owner: " .. Owner.Name)
            end
        end

        return OverlappingComponents
    else
        print("No collisions detected")
        return {}
    end
end

-- Example 3: Smart Movement (Move only if safe)
function SweepTestExample:MoveIfSafe(Actor, TargetLocation)
    if not Actor then
        return false
    end

    -- Check if movement is safe
    if self:CheckMovementSafe(Actor, TargetLocation) then
        -- Safe to move!
        Actor:SetActorLocation(TargetLocation)
        print("Moved to: (" .. TargetLocation.X .. ", " .. TargetLocation.Y .. ", " .. TargetLocation.Z .. ")")
        return true
    else
        -- Movement blocked
        print("Movement cancelled - collision detected")
        return false
    end
end

-- Example 4: Area Damage Query
function SweepTestExample:ApplyAreaDamage(SourceActor, TargetLocation, Damage)
    local World = GetWorld()
    if not World then return end

    local Level = World:GetLevel()
    if not Level then return end

    -- Get all components in the area
    local HitComponents = Level:SweepActorMulti(SourceActor, TargetLocation)

    if HitComponents then
        print("Applying " .. Damage .. " damage to " .. #HitComponents .. " target(s)")

        for i, HitComponent in ipairs(HitComponents) do
            local Owner = HitComponent.Owner
            if Owner then
                print("  Damaged: " .. Owner.Name)
                -- Here you could call a damage function on the actor
                -- Owner:TakeDamage(Damage)
            end
        end
    end
end

-- Example 5: Raycast-like behavior
function SweepTestExample:TestLineOfSight(MyActor, TargetPosition)
    local World = GetWorld()
    if not World then return false end

    local Level = World:GetLevel()
    if not Level then return false end

    -- Test if there's a clear line to target
    local HitResult = Level:SweepActorSingle(MyActor, TargetPosition)

    if HitResult then
        print("Line of sight blocked by: " .. (HitResult.Actor and HitResult.Actor.Name or "Unknown"))
        return false
    else
        print("Clear line of sight to target")
        return true
    end
end

-- Example 6: CollisionTag Filtering - Check for specific collision type only
function SweepTestExample:CheckEnemyCollision(Actor, TargetLocation)
    local World = GetWorld()
    if not World then return false end

    local Level = World:GetLevel()
    if not Level then return false end

    -- Only check for Enemy collisions (ignores Player, Wall, etc.)
    local HitResult = Level:SweepActorSingle(Actor, TargetLocation, CollisionTag.Enemy)

    if HitResult then
        print("Would hit enemy!")
        print("  Enemy Actor: " .. HitResult.Actor.Name)
        return true
    else
        print("No enemy collision detected")
        return false
    end
end

-- Example 7: Wall Collision Check (useful for movement validation)
function SweepTestExample:WouldHitWall(Actor, TargetLocation)
    local World = GetWorld()
    if not World then return false end

    local Level = World:GetLevel()
    if not Level then return false end

    -- Only check for Wall collisions
    local HitResult = Level:SweepActorSingle(Actor, TargetLocation, CollisionTag.Wall)

    if HitResult then
        print("Wall collision detected at: (" .. HitResult.Location.X .. ", " .. HitResult.Location.Y .. ", " .. HitResult.Location.Z .. ")")
        return true
    else
        print("No wall blocking movement")
        return false
    end
end

-- Example 8: Get all enemies in area (filtered multi-sweep)
function SweepTestExample:GetEnemiesInArea(SourceActor, TargetLocation)
    local World = GetWorld()
    if not World then return {} end

    local Level = World:GetLevel()
    if not Level then return {} end

    -- Get only Enemy-tagged components in the sweep area
    local EnemyComponents = Level:SweepActorMulti(SourceActor, TargetLocation, CollisionTag.Enemy)

    if EnemyComponents then
        print("Found " .. #EnemyComponents .. " enemy component(s) in area")

        for i, EnemyComp in ipairs(EnemyComponents) do
            local Owner = EnemyComp.Owner
            if Owner then
                print("  [" .. i .. "] Enemy: " .. Owner.Name)
            end
        end

        return EnemyComponents
    else
        print("No enemies in area")
        return {}
    end
end

-- Example 9: Safe movement with wall-only collision check
function SweepTestExample:MoveIfNoWall(Actor, TargetLocation)
    if not Actor then return false end

    local World = GetWorld()
    if not World then return false end

    local Level = World:GetLevel()
    if not Level then return false end

    -- Check only for walls, allowing movement through enemies/players
    local HitResult = Level:SweepActorSingle(Actor, TargetLocation, CollisionTag.Wall)

    if HitResult then
        print("Movement blocked by wall: " .. HitResult.Actor.Name)
        return false
    else
        -- Safe to move (no walls in the way)
        Actor:SetActorLocation(TargetLocation)
        print("Moved successfully (ignoring non-wall collisions)")
        return true
    end
end

return SweepTestExample
