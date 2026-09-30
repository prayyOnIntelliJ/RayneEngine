# RayneEngine Lua API

Every script attached to an entity runs in its own isolated Lua 5.4 environment. The entity handle is passed to all callbacks as `self`. Scripts are hot-reloaded on file change (`F6` in play mode). The `api_stub.lua` file provides LSP autocomplete.

## Contents

- [Callbacks](#callbacks)
- [Export Variables](#export-variables)
- [Entity & Components](#entity--components)
- [Hierarchy & Templates](#hierarchy--templates)
- [Physics](#physics)
- [Engine](#engine)
- [MathR](#mathr)
- [Timer & Tween](#timer--tween)
- [Input](#input)
- [Audio](#audio)
- [Resource](#resource)
- [UI](#ui)
- [Enums](#enums)

---

## Callbacks

Define these as global functions in your script.

| Callback | Description |
| --- | --- |
| `OnCreate(self)` | Called once when the entity is created |
| `OnUpdate(self, dt)` | Called every frame; `dt` in seconds (0 while paused) |
| `OnDestroy(self)` | Called when the entity or scene is destroyed |
| `OnCollision(self, other)` | Legacy collision callback |
| `OnCollisionEnter(self, other, normalX, normalY)` | Solid contact began, with contact normal |
| `OnTriggerEnter(self, other)` | Entered a trigger / sensor collider |
| `OnInputReceived(self, event)` | Keyboard, mouse, joystick or text input (`OnInputReceiced` is an alias) |
| `OnButtonClicked(id)` | UI button clicked |
| `OnButtonHovered(id)` | UI button hovered |
| `OnSliderChanged(id, value)` | Slider value changed |
| `OnCheckboxChanged(id, checked)` | Checkbox toggled |
| `OnTextInputChanged(id, text)` | TextInput content changed |
| `OnTextInputSubmitted(id, text)` | Enter pressed in a focused TextInput |
| `OnUIHover(id, hovered)` | Hover state of any UI element changed |
| `OnUIFocus(id, focused)` | UI element gained or lost focus |

The UI callbacks can also be assigned on the `UI` table (e.g. `UI.OnButtonClicked = function(id) end`).

### InputEventData

Passed to `OnInputReceived`. Fields marked optional depend on the event type.

| Field | Type | Description |
| --- | --- | --- |
| `type` | integer | Compare with `InputEvent.KeyDown`, `InputEvent.MouseDown`, ... |
| `typeName` | string | e.g. `"KeyDown"`, `"MouseMove"`, `"TextEntered"` |
| `typeStr` | string | Legacy name, e.g. `"key_pressed"`, `"mouse_moved"` |
| `key`, `keyCode`, `keyName` | integer / integer / string | Key info (optional) |
| `button`, `buttonCode`, `buttonName` | integer / integer / string | Mouse button info (optional) |
| `x`, `y` | number | Screen mouse position (optional) |
| `worldX`, `worldY` | number | Mouse position in world space (optional) |
| `delta`, `wheel` | number, string | Scroll delta, `"vertical"` or `"horizontal"` (optional) |
| `text`, `unicode` | string, integer | Entered character (optional) |
| `alt`, `control`, `shift`, `system` | boolean | Modifier states (optional) |
| `joystickId`, `axis`, `position` | integer, integer, number | Joystick data, position -100 to 100 (optional) |
| `isInput` | boolean | Always `true` |
| `isPaused` | boolean | Game is paused |

---

## Export Variables

Expose variables to the Inspector by listing their names in the `Export` table.

```lua
characterTexture = Image("assets/sprites/character.png")
spawnOffset      = Vec2(50.0, -20.0)
tintColor        = Color(255, 128, 64)
targetObject     = Entity("Player")
spawnPrefab      = Template("assets/bullet.template")
moveSpeed        = 120.0

Export = { "characterTexture", "spawnOffset", "tintColor", "targetObject", "spawnPrefab", "moveSpeed" }
```

| Constructor | Fields | Inspector widget |
| --- | --- | --- |
| `Image(path?)` | `path` | Thumbnail, drop slot for textures |
| `Vec2(x?, y?)` | `x`, `y` (default 0) | Two number fields |
| `Color(r?, g?, b?)` | `r`, `g`, `b` (default 255) | Color picker |
| `Entity(name?)` | `name` | Drop slot for hierarchy entities |
| `Template(path?)` | `path` | Thumbnail, drop slot for templates |

Plain numbers, integers, booleans and strings are also supported.

---

## Entity & Components

`Entity` is an integer ID. `0` means "no entity".

### Entity

| Function | Description |
| --- | --- |
| `CreateEntity() -> Entity` | Creates a new entity |
| `DestroyEntity(e)` | Destroys the entity and its components |
| `FindEntityWithTag(tag) -> Entity` | First entity with the tag, or `0` |

### Tag

| Function | Description |
| --- | --- |
| `AddTag(e, tag)` | Adds a tag component |
| `SetTag(e, tag)` | Sets the tag, adds the component if missing |
| `GetTag(e) -> string` | Returns the tag, or `""` |
| `HasTag(e) -> boolean` | Has a tag component |

### Transform

| Function | Description |
| --- | --- |
| `AddTransform(e, x, y)` | Adds a transform |
| `GetTransform(e) -> Transform` | Mutable reference to the transform |
| `SetPosition(e, x, y)` | Sets position |
| `SetRotation(e, degrees)` | Sets rotation in degrees |
| `SetScale(e, sx, sy)` | Sets scale |
| `HasTransform(e) -> boolean` | Has a transform |

`Transform` fields: `x`, `y`, `rotation`, `scaleX`, `scaleY`, plus world-space fields `worldX`, `worldY`, `worldRotation`, `worldScaleX`, `worldScaleY`.

### Velocity

| Function | Description |
| --- | --- |
| `AddVelocity(e, dx, dy)` | Adds a velocity component |
| `GetVelocity(e) -> Velocity` | Mutable reference with `dx`, `dy` |
| `SetVelocity(e, dx, dy)` | Sets velocity |
| `HasVelocity(e) -> boolean` | Has a velocity component |

### Sprite & Color

| Function | Description |
| --- | --- |
| `AddSprite(e, path, w, h)` | Adds a sprite |
| `SetSprite(e, path)` | Replaces the texture |
| `SetSpriteSize(e, w, h)` | Sets rendered size |
| `HasSprite(e) -> boolean` | Has a sprite |
| `SetColor(e, r, g, b, a?)` | Sets tint / shape color (0-255) |

### Camera

| Function | Description |
| --- | --- |
| `AddCamera(e)` | Makes the entity the camera focus |
| `RemoveCamera(e)` | Removes the camera component |
| `HasCamera(e) -> boolean` | Has a camera component |

### Collision

| Function | Description |
| --- | --- |
| `AddCollision(e, channel?)` | Adds a collider (default channel `0`, type `"static"`) |
| `RemoveCollision(e)` | Removes the collider |
| `HasCollision(e) -> boolean` | Has a collider |
| `SetCollisionChannel(e, channel)` | Sets the filter channel |
| `GetCollisionChannel(e) -> integer` | Gets the filter channel |
| `SetCollisionType(e, type)` | `"static"` or `"solid"` |
| `GetCollisionType(e) -> string` | Current type |

---

## Hierarchy & Templates

| Function | Description |
| --- | --- |
| `SetParent(child, parent, keepWorldTransform?)` | Sets the parent (`0` unparents); `keepWorldTransform` defaults to `true` |
| `GetParent(child) -> Entity` | Parent ID, or `0` |
| `GetChildren(parent) -> Entity[]` | Array of child IDs |
| `GetWorldPosition(e) -> x, y` | World position |
| `GetWorldRotation(e) -> number` | World rotation in degrees |
| `GetWorldScale(e) -> sx, sy` | World scale |
| `Template(path) -> Template` | Creates a template reference |
| `Template:Instantiate(x, y, parent?) -> Entity` | Spawns the template, returns the root entity |
| `Instantiate(template, x, y, parent?) -> Entity` | Spawns a `Template` or a path string |
| `LoadScene(name)` | Loads `assets/scenes/<name>.json` |

```lua
local bullet = Template("assets/templates/bullet.template")
local e = bullet:Instantiate(100, 200)
```

---

## Physics

### Rigidbody

| Function | Description |
| --- | --- |
| `AddRigidbody(e, bodyType?, mass?, gravityScale?) -> Rigidbody2D` | Adds a rigidbody (defaults: Dynamic, `1.0`, `1.0`) |
| `GetRigidbody(e) -> Rigidbody2D \| nil` | Returns the rigidbody or `nil` |
| `HasRigidbody(e) -> boolean` | Has a rigidbody |

`Rigidbody2D` fields: `bodyType`, `mass`, `gravityScale`, `restitution` (0-1), `drag`, `freezeRotation`.

### Physics library

| Function | Description |
| --- | --- |
| `Physics.Raycast(startX, startY, dirX, dirY, distance, channel?) -> RaycastResult` | Closest hit; direction is normalized, channel `-1` = all |
| `Physics.ApplyForce(e, fx, fy)` | Force accumulated for the next physics step |
| `Physics.ApplyImpulse(e, ix, iy)` | Instant impulse (`v += impulse / mass`) |
| `Physics.SetVelocity(e, vx, vy)` | Sets linear velocity |
| `Physics.GetVelocity(e) -> vx, vy` | Gets linear velocity |
| `Physics.SetGravity(gx, gy)` | Global gravity (default `0, 980`) |
| `Physics.GetGravity() -> gx, gy` | Current gravity |
| `Physics.SetFixedTimestep(dt)` | Fixed timestep in seconds (default `1/60`) |
| `Physics.GetFixedTimestep() -> dt` | Current timestep |

`RaycastResult` fields: `hit`, `entity`, `pointX`, `pointY`, `normalX`, `normalY`, `distance`.

```lua
local r = Physics.Raycast(t.x, t.y, 0, 1, 50)
if r.hit then Engine.Log("Ground: " .. r.entity) end
```

---

## Engine

Most functions also exist as globals (noted in the second column).

| Function | Global alias | Description |
| --- | --- | --- |
| `Engine.Log(...)` | `print` | Log with teal `[LOG]` badge |
| `Engine.LogWarning(msg)` | | Log with amber `[WARN]` badge |
| `Engine.LogError(msg)` | | Log with red `[ERROR]` badge |
| `Engine.LogToScreen(msg, duration?, r?, g?, b?)` | `LogToScreen` | On-screen toast in editor play mode (default 3.5 s) |
| `Engine.SetPaused(paused)` | `PauseGame` | Pauses / unpauses simulation |
| `Engine.IsPaused()` / `Engine.GetPaused()` | | Returns pause state |
| `Engine.TogglePause()` | | Toggles pause |
| `Engine.SetTimeScale(scale)` | `SetTimeScale` | Simulation speed factor |
| `Engine.GetTimeScale()` | | Current time scale |
| `Engine.SetFullscreen(fullscreen)` | | Sets fullscreen |
| `Engine.ToggleFullscreen()` | | Toggles fullscreen |
| `Engine.IsFullscreen()` | | Returns fullscreen state |
| `Engine.SetCursorVisible(visible)` | | Shows / hides the cursor |
| `Engine.TakeScreenshot(filename?) -> string` | | Saves to `screenshots/`, returns the path |
| `Engine.OpenURL(url)` | | Opens a URL or path with the OS handler |
| `Engine.GetFPS()` | `GetFPS` | Current FPS |
| `Engine.GetDeltaTime()` | | Unscaled delta time in seconds |
| `Engine.ShowFPS(show)` | | Toggles the FPS overlay |
| `Engine.IsFPSShown()` | | Returns overlay state |
| `Engine.RestartScene()` / `Engine.RestartCurrentScene()` | `RestartScene` | Reloads the active scene |
| `Engine.LoadScene(name)` | `LoadScene` | Loads `assets/scenes/<name>.json` |
| `Engine.Quit()` | `QuitGame` | Closes the standalone game, or returns to the editor |

---

## MathR

| Function | Description |
| --- | --- |
| `MathR.ClampI(value, min, max)` | Clamp integer |
| `MathR.ClampF(value, min, max)` | Clamp float |
| `MathR.ClampF01(value)` | Clamp to `[0, 1]` |
| `MathR.AbsI(value)` | Integer absolute value |
| `MathR.AbsF(value)` | Float absolute value |
| `MathR.Ceil(value)` | Round up |
| `MathR.Floor(value)` | Round down |
| `MathR.Lerp(start, endVal, factor)` | Linear interpolation (factor clamped to `[0, 1]`) |
| `MathR.InverseLerp(start, endVal, value)` | Interpolation factor of a value |
| `MathR.Sin(x)` | Sine (Taylor approximation) |
| `MathR.Cos(x)` | Cosine |

---

## Timer & Tween

| Function | Description |
| --- | --- |
| `Timer.After(seconds, callback)` | Runs `callback` once after the delay |
| `Tween.Position(entity, targetX, targetY, duration, ease?)` | Animates position over time |

Easing values: `"linear"` (default), `"EaseInQuad"`, `"EaseOutQuad"`, `"EaseInOutQuad"`.

```lua
Timer.After(1.5, function() Engine.Log("fired") end)
Tween.Position(self, 500, 300, 1.2, "EaseOutQuad")
```

---

## Input

`key` accepts a `Key` enum value or a case-insensitive string (`"w"`, `"space"`, `"enter"`, `"esc"`, `"shift"`, `"ctrl"`, `"alt"`, `"left"`, `"tab"`, `"delete"`, `"backspace"`, `"0"`-`"9"`, ...). `button` accepts a `Mouse` enum value or `"left"`, `"right"`, `"middle"`.

| Function | Description |
| --- | --- |
| `Input.IsKeyDown(key)` | Key is held |
| `Input.IsKeyPressed(key)` | Key pressed this frame |
| `Input.IsKeyJustPressed(key)` | Alias for `IsKeyPressed` |
| `Input.IsKeyReleased(key)` | Key released this frame |
| `Input.IsMouseDown(button)` | Button is held |
| `Input.IsMousePressed(button)` | Button pressed this frame |
| `Input.IsMouseReleased(button)` | Button released this frame |
| `Input.MouseX()` / `Input.MouseY()` | Mouse position in screen coordinates |
| `Input.MouseScroll()` | Scroll delta this frame |
| `Input.HasInput() -> boolean` | Any input event received this frame |
| `Input.GetLastEvent() -> InputEventData \| nil` | Last input event |
| `Input.KeyToString(key) -> string` | Key code to name |
| `Input.MouseButtonToString(button) -> string` | Button code to name |

---

## Audio

Volumes are 0-100.

| Function | Description |
| --- | --- |
| `Audio.PlaySound(path, volume?, pitch?)` | Plays a sound effect (defaults `100`, `1.0`) |
| `Audio.StopAllSounds()` | Stops all sound effects |
| `Audio.PlayMusic(path, loop?, volume?)` | Streams music (defaults `true`, `100`) |
| `Audio.StopMusic()` | Stops music |
| `Audio.PauseMusic()` | Pauses music |
| `Audio.ResumeMusic()` | Resumes music |
| `Audio.SetMusicVolume(volume)` | Music volume |
| `Audio.SetMasterVolume(volume)` | Global volume, rescales active sounds |

---

## Resource

| Function | Description |
| --- | --- |
| `Resource.PreloadTexture(path) -> boolean` | Caches a texture |
| `Resource.PreloadFont(path) -> boolean` | Caches a font |
| `Resource.PreloadSound(path) -> boolean` | Caches a sound buffer |
| `Resource.ClearTextures()` / `ClearFonts()` / `ClearSounds()` / `ClearAll()` | Flushes caches |
| `Resource.TextureCount()` / `FontCount()` / `SoundCount()` | Number of cached resources |
| `Resource.PrintStats()` | Prints cache statistics to the console |

---

## UI

Elements are addressed by the string ID from the UI Editor (canvas 1920x1080). Functions marked with `*` also exist as `UI_<Name>` globals, e.g. `UI_SetText(id, text)`.

### General

| Function | Description |
| --- | --- |
| `UI.SetText(id, text)` * | Sets text (Text / Button) |
| `UI.GetText(id) -> string` * | Gets text |
| `UI.SetPosition(id, x, y)` * | Sets position |
| `UI.SetSize(id, w, h)` * | Sets size |
| `UI.SetColor(id, r, g, b, a?)` * | Sets fill color |
| `UI.SetZIndex(id, z)` * / `UI.GetZIndex(id)` * | Draw order, higher is in front |
| `UI.SetVisible(id, visible)` * / `UI.GetVisible(id)` * | Visibility (hidden elements are not interactive) |
| `UI.SetOpacity(id, opacity)` * | Alpha 0-255 |
| `UI.SetOutline(id, r, g, b, a, thickness)` * | Panel border |
| `UI.SetTexture(id, path)` | Texture of an Image / Button |

### Hierarchy

| Function | Description |
| --- | --- |
| `UI.SetParent(childId, parentId, keepWorldPos?)` * | Sets parent (`""` unparents) |
| `UI.GetParent(id) -> string` * | Parent ID, or `""` |
| `UI.GetChildren(id) -> string[]` * | Child IDs |
| `UI.GetWorldPosition(id) -> x, y` * | World position on the canvas |

### Text

| Function | Description |
| --- | --- |
| `UI.SetTextStyle(id, style)` * | Bitmask: 0 Regular, 1 Bold, 2 Italic, 4 Underline, 8 StrikeThrough |
| `UI.SetTextAlign(id, align)` * | 0 Left, 1 Center, 2 Right |
| `UI.SetTextVAlign(id, valign)` | 0 Top, 1 Middle, 2 Bottom |
| `UI.SetUpperCase(id, upper)` * | Force uppercase |
| `UI.SetFontSize(id, size)` * | Character size |
| `UI.SetLetterSpacing(id, spacing)` * | Multiplier, `1.0` = default |
| `UI.SetLineSpacing(id, spacing)` * | Multiplier, `1.0` = default |
| `UI.SetTextColor(id, r, g, b, a)` * | Text fill color |
| `UI.SetTextOutline(id, r, g, b, a, thickness)` * | Text outline |
| `UI.SetTextOffset(id, ox, oy)` * | Text offset inside the element |

### Interaction

| Function | Description |
| --- | --- |
| `UI.IsButtonClicked(id)` * | Button clicked this frame |
| `UI.IsButtonHovered(id)` * | Button hovered |
| `UI.IsHovered(id)` | Any element hovered |
| `UI.IsPressed(id)` | Interactive element pressed |
| `UI.IsDisabled(id)` | Element disabled |
| `UI.SetDisabled(id, disabled)` * | Disables a button |
| `UI.SetHoverTexture(id, path)` | Button hover texture |
| `UI.SetPressedTexture(id, path)` | Button pressed texture |
| `UI.SetChecked(id, checked)` / `UI.GetChecked(id)` | Checkbox state |
| `UI.SetCheckedTexture(id, path)` | Checkbox checked texture |
| `UI.SetSliderValue(id, v)` / `UI.GetSliderValue(id)` | Slider value |
| `UI.SetSliderMin(id, min)` / `UI.GetSliderMin(id)` | Slider minimum |
| `UI.SetSliderMax(id, max)` / `UI.GetSliderMax(id)` | Slider maximum |
| `UI.SetProgressValue(id, v)` / `UI.GetProgressValue(id)` | Progress bar, 0.0-1.0 |
| `UI.SetFocused(id, focused)` / `UI.GetFocused(id)` | TextInput focus |

```lua
function OnUpdate(self, dt)
    UI.SetText("score_label", "Score: " .. tostring(score))
    if UI.IsButtonClicked("btn_start") then
        LoadScene("level1")
    end
end
```

---

## Enums

| Enum | Values |
| --- | --- |
| `BodyType` | `Dynamic` (0), `Kinematic` (1), `Static` (2) |
| `ColliderShape` | `Box` (0), `Circle` (1) |
| `Key` / `KeyCode` | `A`-`Z`, `Space`, `Enter`, `Escape`, `LShift`, `RShift`, `LCtrl`, `RCtrl`, `Left`, `Right`, `Up`, `Down`, `Tab`, `Delete` |
| `Mouse` | `Left`, `Right`, `Middle` |
| `InputEvent` / `InputEventType` / `EventType` | `KeyDown`, `KeyUp`, `MouseDown`, `MouseUp`, `MouseMove`, `MouseWheel`, `TextEntered`, `JoystickPressed`, `JoystickReleased`, `JoystickMoved`, `Unknown` (plus aliases such as `KeyPressed`, `KeyReleased`, `MousePressed`, `MouseScroll`, `Text`) |
