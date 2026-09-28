speed = 50.0
enemyName = "Goblin"
isDead = false
maxHealth = 100

Export = { "speed", "enemyName", "isDead", "maxHealth" }

function OnCreate()
    Engine.Log("Enemy " .. enemyName .. " spawned with " .. tostring(maxHealth) .. " HP")
end

function OnUpdate(dt)
    if not isDead then
        -- logic
    end
end
