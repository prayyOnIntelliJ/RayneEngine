-- InputTestScript.lua
-- Demonstrates the OnInputReceived callback with InputEvent enum

function OnCreate(self)
    print("[InputTestScript] Initialized on entity: " .. tostring(self))
end

-- Triggers ONLY when an input event arrives (keyboard, mouse, joystick, text)
function OnInputReceived(self, event)
    if event.type == InputEvent.Keydown then
        print("[InputTest] KeyDown: " .. tostring(event.keyName) .. " (Code: " .. tostring(event.key) .. ")")
        if event.key == Key.Space then
            print("[InputTest] Space was pressed!")
        end
    elseif event.type == InputEvent.KeyUp then
        print("[InputTest] KeyUp: " .. tostring(event.keyName))
    elseif event.type == InputEvent.MouseDown then
        print("[InputTest] MouseDown: " .. tostring(event.buttonName) .. " at screen (" .. tostring(event.x) .. ", " .. tostring(event.y) .. ") World: (" .. tostring(event.worldX) .. ", " .. tostring(event.worldY) .. ")")
        if event.button == Mouse.Left then
            print("[InputTest] Left mouse button clicked!")
        end
    elseif event.type == InputEvent.MouseWheel then
        print("[InputTest] Mouse wheel delta: " .. tostring(event.delta))
    end
end
