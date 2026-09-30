-- SpawnerScript.lua
-- Demonstrates exporting a Template variable and instantiating it

-- 1. Declare the template as a variable using the global Template() constructor
bulletPrefab = Template("assets/templates/bullet.template")
spawnInterval = 1.0
spawnSpeed = 250.0

-- 2. Export variables to expose them in the Inspector
Export = { "bulletPrefab", "spawnInterval", "spawnSpeed" }

local timer = 0.0

function OnCreate(entity)
    Engine.Log("[Spawner] Initialized. Template path: " .. tostring(bulletPrefab.path))
end

function OnUpdate(entity, dt)
    timer = timer + dt
    if timer >= spawnInterval then
        timer = timer - spawnInterval

        local transform = GetTransform(entity)
        if transform then
            local spawnX = transform.worldX
            local spawnY = transform.worldY

            -- 3. Instantiate the template using the global Instantiate() function
            local bulletEntity = Instantiate(bulletPrefab, spawnX, spawnY)

            if bulletEntity and bulletEntity ~= 0 then
                Engine.Log("[Spawner] Spawned bullet entity ID: " .. tostring(bulletEntity))

                -- Optionally add velocity or adjust components on the spawned instance
                if HasVelocity(bulletEntity) then
                    local vel = GetVelocity(bulletEntity)
                    vel.dx = spawnSpeed
                    vel.dy = 0.0
                end
            end
        end
    end
end
