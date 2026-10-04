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

function BlaBlaBla()
    Engine.Log("[EnemyScript] BlaBlaBla() was called on " .. enemyName .. "!")
    Engine.LogToScreen("Enemy BlaBlaBla() called!", 3.0, 50, 220, 100)
end

function TakeDamage(amount)
    maxHealth = maxHealth - amount
    Engine.Log("[EnemyScript] " .. enemyName .. " took " .. tostring(amount) .. " damage. Health: " .. tostring(maxHealth))
    if maxHealth <= 0 then
        isDead = true
        Engine.Log("[EnemyScript] " .. enemyName .. " died!")
    end
end