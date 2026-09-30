-- ExportDemoScript.lua
-- Demonstrates all export variable types in RayneEngine:
--   - Image: Texture/Sprite asset path (drag & drop from Content Browser)
--   - Vec2: 2D vector (X/Y coordinates)
--   - Color: RGBA/RGB color (pickable via Windows color picker in Inspector)
--   - Entity: Reference to another scene object (drag & drop from Hierarchy)
--   - Template: Prefab/template file path (drag & drop from Content Browser)
--   - Primitives: Int, Float, Bool, String

-- 1. Declare variables with default values using constructors
characterTexture = Image("assets/sprites/character.png")
spawnOffset      = Vec2(50.0, -20.0)
tintColor        = Color(255, 128, 64)
targetObject     = Entity("Player")
spawnPrefab      = Template("assets/templates/bullet.template")

-- Primitive types
moveSpeed        = 120.0
maxHealth        = 100
enableGlow       = true
greetingMessage  = "Hello RayneEngine!"

-- 2. Export list: names of variables to expose in the Editor Inspector
Export = {
    "characterTexture",
    "spawnOffset",
    "tintColor",
    "targetObject",
    "spawnPrefab",
    "moveSpeed",
    "maxHealth",
    "enableGlow",
    "greetingMessage"
}

function OnCreate(self)
    Engine.Log("[ExportDemo] OnCreate called for entity " .. tostring(self))
    Engine.Log("[ExportDemo] Greeting: " .. greetingMessage)
    Engine.Log("[ExportDemo] MoveSpeed: " .. tostring(moveSpeed) .. ", MaxHealth: " .. tostring(maxHealth))
    Engine.Log("[ExportDemo] Glow enabled: " .. tostring(enableGlow))

    -- Using Image:
    if characterTexture and characterTexture.path ~= "" then
        Engine.Log("[ExportDemo] Setting sprite from Image: " .. characterTexture.path)
        SetSprite(self, characterTexture.path)
    end

    -- Using Color:
    if tintColor then
        Engine.Log(string.format("[ExportDemo] Applying tint Color: R=%d, G=%d, B=%d", tintColor.r, tintColor.g, tintColor.b))
        SetColor(self, tintColor.r, tintColor.g, tintColor.b, 255)
    end

    -- Using Vec2:
    if spawnOffset then
        Engine.Log(string.format("[ExportDemo] Spawn offset: X=%.2f, Y=%.2f", spawnOffset.x, spawnOffset.y))
    end

    -- Using Entity:
    if targetObject and targetObject.name ~= "" then
        Engine.Log("[ExportDemo] Target entity name: " .. targetObject.name)
    end

    -- Using Template:
    if spawnPrefab and spawnPrefab.path ~= "" then
        Engine.Log("[ExportDemo] Configured template prefab: " .. spawnPrefab.path)
    end
end

function OnUpdate(self, dt)
    -- Example: Move according to Vec2 direction
    if spawnOffset then
        local transform = GetTransform(self)
        if transform then
            -- Can use spawnOffset or other exported values here
        end
    end
end
