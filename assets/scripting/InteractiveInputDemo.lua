-- InteractiveInputDemo.lua
-- Demonstrates how OnInputReceived (and OnInputReceiced) works in RayneEngine

function OnCreate(self)
    print("==================================================")
    print("[InputDemo] Script initialized on Entity ID: " .. tostring(self))
    print("[InputDemo] Controls:")
    print("  - WASD / Pfeiltasten: Bewegen des Objekts")
    print("  - Space: Sprung / Impuls nach oben")
    print("  - Linksklick: Teleportiere Objekt zur Maus-Weltposition")
    print("  - Mausrad: Skaliere das Objekt groesser / kleiner")
    print("  - Andere Tasten: Werden in der Konsole geloggt")
    print("==================================================")
end

-- HINWEIS: OnInputReceived wird NUR aufgerufen, wenn wirklich ein Input
-- (Tastatur, Maus, Scrollrad, Joystick etc.) registriert wurde.
-- Keine Strings noetig: Nutze InputEvent.Keydown, Key.Space, Mouse.Left etc.!
function OnInputReceived(self, event)
    -- 1. TASTATUR: Taste gedrueckt (InputEvent.Keydown oder InputEvent.KeyDown)
    if event.type == InputEvent.Keydown then
        print(string.format("[InputDemo] KeyDown: '%s' (Code: %d, Shift: %s, Ctrl: %s)",
            event.keyName or "Unknown",
            event.key or 0,
            tostring(event.shift),
            tostring(event.control)
        ))

        local speed = 20.0
        if event.shift then speed = 50.0 end -- Sprinten mit Shift

        -- Pruefe Taste direkt mit Key.<Name> (oder KeyCode.<Name>)
        if event.key == Key.W or event.key == Key.Up then
            Transform.Translate(self, 0, -speed)
        elseif event.key == Key.S or event.key == Key.Down then
            Transform.Translate(self, 0, speed)
        elseif event.key == Key.A or event.key == Key.Left then
            Transform.Translate(self, -speed, 0)
        elseif event.key == Key.D or event.key == Key.Right then
            Transform.Translate(self, speed, 0)
        elseif event.key == Key.Space then
            print("[InputDemo] Space gedrueckt -> Sprung!")
            Transform.Translate(self, 0, -30)
        end

    -- 2. TASTATUR: Taste losgelassen
    elseif event.type == InputEvent.KeyUp then
        print("[InputDemo] KeyUp: " .. (event.keyName or tostring(event.key)))

    -- 3. MAUS: Mausklick (nutzt worldX und worldY fuer exakte Weltkoordinaten)
    elseif event.type == InputEvent.MouseDown then
        print(string.format("[InputDemo] Mausklick [%s] auf Screen (%d, %d) -> Weltposition (%.1f, %.1f)",
            event.buttonName or "Unknown",
            event.x or 0, event.y or 0,
            event.worldX or 0.0, event.worldY or 0.0
        ))

        -- Pruefe Maustaste direkt mit Mouse.Left / Mouse.Right
        if event.button == Mouse.Left and event.worldX and event.worldY then
            -- Teleportiere Entity direkt dorthin, wo geklickt wurde
            Transform.SetPosition(self, event.worldX, event.worldY)
            print("[InputDemo] Entity wurde zur Mausklick-Position teleportiert!")
        end

    -- 4. MAUS: Scrollrad
    elseif event.type == InputEvent.MouseWheel then
        print(string.format("[InputDemo] Mausrad gedreht: %.1f", event.delta or 0.0))
        local currentScaleX, currentScaleY = Transform.GetScale(self)
        if currentScaleX and currentScaleY then
            local factor = (event.delta > 0) and 1.1 or 0.9
            Transform.SetScale(self, currentScaleX * factor, currentScaleY * factor)
        end
    end
end

-- OnUpdate bleibt komplett sauber oder kann fuer reine Physik/Animation genutzt werden.
-- Wenn kein Input kommt, verbraucht OnInputReceived 0% CPU!
function OnUpdate(self, dt)
    -- Hier muss KEIN Input.IsKeyPressed("W") mehr abgefragt werden,
    -- wenn man rein eventbasiert arbeiten moechte!
end
