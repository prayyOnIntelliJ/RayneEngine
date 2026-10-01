-- CameraTestScript.lua
-- Comprehensive test & demonstration script for the Camera system in RayneEngine

local following = false
local boundsActive = false
local currentMode = CameraMultiFollowMode.AutoFrame

function OnStart(self)
    Log("CameraTestScript initialized on entity " .. tostring(self))
    Log("Controls:")
    Log("  W / A / S / D  : Free camera movement (Camera.Move)")
    Log("  Q / E          : Zoom in / Zoom out (Camera.Zoom)")
    Log("  R / T          : Rotate / Reset rotation (Camera.Rotate / SetRotation)")
    Log("  SPACE          : Screen shake (Camera.Shake)")
    Log("  F              : Toggle single follow on self (Camera.Follow / Camera.StopFollow)")
    Log("  G              : Add/Remove self to multi-target follow group (Camera.AddFollowTarget)")
    Log("  1 / 2 / 3      : Set MultiFollowMode (1=Priority, 2=Average, 3=AutoFrame)")
    Log("  B              : Toggle world bounds (Camera.SetBounds / Camera.ClearBounds)")
    Log("  Z              : Reset camera (Camera.Reset)")
end

function OnUpdate(self, dt)
    local moveSpeed = 600.0 * dt
    local dx = 0.0
    local dy = 0.0

    if IsKeyDown(Key.W) or IsKeyDown(Key.Up) then
        dy = dy - moveSpeed
    end
    if IsKeyDown(Key.S) or IsKeyDown(Key.Down) then
        dy = dy + moveSpeed
    end
    if IsKeyDown(Key.A) or IsKeyDown(Key.Left) then
        dx = dx - moveSpeed
    end
    if IsKeyDown(Key.D) or IsKeyDown(Key.Right) then
        dx = dx + moveSpeed
    end

    if dx ~= 0.0 or dy ~= 0.0 then
        if following or Camera.GetFollowTargetCount() > 0 then
            Camera.StopFollow()
            Camera.ClearFollowTargets()
            following = false
            Log("Follow disabled due to manual camera movement")
        end
        Camera.Move(dx, dy)
    end

    -- Zooming with Q and E
    if IsKeyDown(Key.Q) then
        Camera.Zoom(1.0 + (1.5 * dt))
    end
    if IsKeyDown(Key.E) then
        Camera.Zoom(1.0 - (1.5 * dt))
    end

    -- Rotating with R
    if IsKeyDown(Key.R) then
        Camera.Rotate(90.0 * dt)
    end
end

function OnInputReceived(self, event)
    if event.type == InputEvent.KeyDown then
        -- Space: Screen shake
        if event.key == Key.Space then
            Log("Triggering Camera.Shake(20.0, 0.5, true)")
            Camera.Shake(20.0, 0.5, true)
        end

        -- T: Reset rotation
        if event.key == Key.T then
            Log("Resetting camera rotation to 0")
            Camera.SetRotation(0.0)
        end

        -- F: Toggle Follow single target
        if event.key == Key.F then
            following = not following
            if following then
                Log("Camera following entity " .. tostring(self) .. " with smoothSpeed=5.0")
                Camera.Follow(self, 5.0, 0.0, 0.0)
            else
                Log("Camera stopped following")
                Camera.StopFollow()
            end
        end

        -- G: Toggle self in multi-target follow list
        if event.key == Key.G then
            local targets = Camera.GetFollowTargets()
            local found = false
            for _, t in ipairs(targets) do
                if t == self then
                    found = true
                    break
                end
            end
            if found then
                Camera.RemoveFollowTarget(self)
                Log("Removed entity " .. tostring(self) .. " from follow targets. Remaining count: " .. tostring(Camera.GetFollowTargetCount()))
            else
                Camera.AddFollowTarget(self)
                Log("Added entity " .. tostring(self) .. " to follow targets. Total count: " .. tostring(Camera.GetFollowTargetCount()))
            end
        end

        -- 1 / 2 / 3: MultiFollowMode switching
        if event.key == Key.Num1 then
            Camera.SetMultiFollowMode(CameraMultiFollowMode.Priority)
            Log("Multi-follow mode set to: Priority (0)")
        elseif event.key == Key.Num2 then
            Camera.SetMultiFollowMode(CameraMultiFollowMode.Average)
            Log("Multi-follow mode set to: Average / Midpoint (1)")
        elseif event.key == Key.Num3 then
            Camera.SetMultiFollowMode(CameraMultiFollowMode.AutoFrame)
            Camera.SetAutoFramePadding(250.0)
            Camera.SetAutoFrameZoomLimits(0.3, 2.5)
            Log("Multi-follow mode set to: AutoFrame (2) with padding=250")
        end

        -- B: Toggle Bounds
        if event.key == Key.B then
            boundsActive = not boundsActive
            if boundsActive then
                Log("Camera bounds enabled: (-1000, -1000, 3000, 3000)")
                Camera.SetBounds(-1000.0, -1000.0, 3000.0, 3000.0, true)
            else
                Log("Camera bounds cleared")
                Camera.ClearBounds()
            end
        end

        -- Z: Reset Camera
        if event.key == Key.Z then
            Log("Camera.Reset()")
            Camera.Reset()
            following = false
            boundsActive = false
        end
    end

    -- Mouse wheel zoom
    if event.type == InputEvent.MouseWheel then
        if event.delta and event.delta ~= 0 then
            local factor = event.delta > 0 and 1.1 or 0.9
            Camera.Zoom(factor)
            Log("Camera Zoom: " .. string.format("%.2f", Camera.GetZoom()))
        end
    end

    -- Mouse click coordinate conversion demonstration
    if event.type == InputEvent.MouseDown and event.button == Mouse.Left then
        local sx, sy = event.x or 0, event.y or 0
        local wx, wy = Camera.ScreenToWorld(sx, sy)
        local backSx, backSy = Camera.WorldToScreen(wx, wy)
        Log(string.format("ScreenToWorld: (%.1f, %.1f) -> World: (%.1f, %.1f) -> WorldToScreen: (%.1f, %.1f)", sx, sy, wx, wy, backSx, backSy))
    end
end
