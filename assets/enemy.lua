function OnCreate(self)
    if not HasCollision(self) then
        AddCollision(self, 0)
    end
end

function OnUpdate(self, dt)

end

function OnCollision(self, other)
    DestroyEntity(self)
end