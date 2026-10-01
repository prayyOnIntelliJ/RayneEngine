-- VelocityTestScript.lua
-- Demonstrates and tests the overhauled VelocityComponent in RayneEngine

speed = 150.0

function OnCreate(self)
    Engine.Log("[VelocityTest] Entity created: " .. tostring(self))
    
    -- Ensure entity has a Velocity component set to move horizontally
    if not HasVelocity(self) then
        AddVelocity(self, speed, 0.0)
    else
        SetVelocity(self, speed, 0.0)
    end
    
    -- Test getter and aliases
    local vel = GetVelocity(self)
    if vel then
        Engine.Log(string.format("[VelocityTest] Initial Velocity: dx=%.1f, dy=%.1f, vx=%.1f, vy=%.1f, x=%.1f, y=%.1f",
            vel.dx, vel.dy, vel.vx, vel.vy, vel.x, vel.y))
    end
end

function OnUpdate(self, dt)
    -- Bounce left/right when reaching boundary or reverse on input
    local t = GetTransform(self)
    local vel = GetVelocity(self)
    
    if t and vel then
        -- Automatically reverse direction if hitting edges
        if t.x > 800 and vel.dx > 0 then
            SetVelocity(self, -speed, vel.dy)
            Engine.Log("[VelocityTest] Reversing direction to LEFT at x=" .. tostring(t.x))
        elseif t.x < 100 and vel.dx < 0 then
            SetVelocity(self, speed, vel.dy)
            Engine.Log("[VelocityTest] Reversing direction to RIGHT at x=" .. tostring(t.x))
        end
    end
end
