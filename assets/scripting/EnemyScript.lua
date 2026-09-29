speed = 50.0
enemyName = "Goblin"
isDead = false
maxHealth = 100
test = "TEST"

Export = { "speed", "enemyName", "isDead", "maxHealth", "test" }

function OnCreate()
    Engine.Log("Enemy " .. enemyName .. " spawned with " .. tostring(maxHealth) .. " HP")
    Engine.LogToScreen("TEST")
end

function OnUpdate(dt)
    if not isDead then
        -- logic
    end
end