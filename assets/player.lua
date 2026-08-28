local speed = 200

function OnCreate(self)
    if not HasCollision(self) then
        AddCollision(self, 0)
    end
end

function OnUpdate(self, dt)
    local t = GetTransform(self)
    if not t then return end

    local dx = 0
    local dy = 0

    if Input.IsKeyDown("w") then
        dy = dy - 1
    end
    if Input.IsKeyDown("s") then
        dy = dy + 1
    end
    if Input.IsKeyDown("a") then
        dx = dx - 1
    end
    if Input.IsKeyDown("d") then
        dx = dx + 1
    end

    t.x = t.x + dx * speed * dt
    t.y = t.y + dy * speed * dt
end
