-- PhysicsDemoScript.lua
-- Demonstrates 2D Physics, Rigidbodies, Forces, Impulses, and Callbacks in RayneEngine

jumpForce = 450.0
moveSpeed = 300.0

function OnCreate(self)
    print("[PhysicsDemo] OnCreate called on entity " .. tostring(self))
    -- Optionally ensure we have a Rigidbody if not added via Editor Inspector
    if not HasRigidbody(self) then
        AddRigidbody(self, BodyType.Dynamic, 1.0, 1.0)
    end
end

function OnUpdate(self, dt)
    -- Horizontal movement via velocity or force
    local vx = 0
    if Input.IsKeyDown(KeyCode.A) or Input.IsKeyDown(KeyCode.Left) then
        vx = -moveSpeed
    elseif Input.IsKeyDown(KeyCode.D) or Input.IsKeyDown(KeyCode.Right) then
        vx = moveSpeed
    end

    if vx ~= 0 then
        local currentVx, currentVy = Physics.GetVelocity(self)
        Physics.SetVelocity(self, vx, currentVy)
    end

    -- Jump impulse when pressing Space or W
    if Input.IsKeyJustPressed(KeyCode.Space) or Input.IsKeyJustPressed(KeyCode.W) then
        print("[PhysicsDemo] Jump impulse applied!")
        Physics.ApplyImpulse(self, 0, -jumpForce)
    end
end

-- Called when a solid physical collision begins with contact normal
function OnCollisionEnter(self, other, normalX, normalY)
    print(string.format("[PhysicsDemo] Collided with entity %s, contact normal: (%.2f, %.2f)", tostring(other), normalX, normalY))
end

-- Called when entering a trigger / sensor collider
function OnTriggerEnter(self, other)
    print("[PhysicsDemo] Trigger entered with entity " .. tostring(other))
end
