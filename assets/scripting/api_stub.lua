---@meta --

---@class Entity : integer -- Defines Entity as a number

---@class Transform
---@field x number
---@field y number
---@field rotation number
---@field scaleX number
---@field scaleY number
---@field worldX number
---@field worldY number
---@field worldRotation number
---@field worldScaleX number
---@field worldScaleY number
Transform = {}

---@class Velocity
---@field dx number
---@field dy number
---@field vx number
---@field vy number
---@field x number
---@field y number
Velocity = {}

---@class MathR
MathR = {}

---Clamps an integer value between a minimum and maximum
---@param value number
---@param min number
---@param max number
---@return number
function MathR.ClampI(value, min, max) end

---Clamps a float value between a minimum and maximum
---@param value number
---@param min number
---@param max number
---@return number
function MathR.ClampF(value, min, max) end

---Clamps a float value between 0.0 and 1.0
---@param value number
---@return number
function MathR.ClampF01(value) end

---Returns the absolute value of an integer
---@param value number
---@return number
function MathR.AbsI(value) end

---Returns the absolute value of a float
---@param value number
---@return number
function MathR.AbsF(value) end

---Returns the smallest integral value greater than or equal to x
---@param value number
---@return number
function MathR.Ceil(value) end

---Returns the largest integral value less than or equal to x
---@param value number
---@return number
function MathR.Floor(value) end

---Linearly interpolates between start and end by factor
---@param start number
---@param endVal number
---@param factor number
---@return number
function MathR.Lerp(start, endVal, factor) end

---Calculates the linear parameter t that produces the interpolant value within the range [start, end]
---@param start number
---@param endVal number
---@param value number
---@return number
function MathR.InverseLerp(start, endVal, value) end

---Calculates the sine of a value using Taylor series approximation
---@param x number
---@return number
function MathR.Sin(x) end

---Calculates the cosine of a value
---@param x number
---@return number
function MathR.Cos(x) end

---@class Timer
Timer = {}

---Schedules a callback function to run after a specified duration in seconds.
---@param seconds number Delay in seconds before running the callback
---@param callback fun() Function to call when timer expires
function Timer.After(seconds, callback) end

---@class Tween
Tween = {}

---Smoothly animates an Entity's position over time using an easing curve.
---@param entity Entity The entity to animate
---@param targetX number Target world/local X position
---@param targetY number Target world/local Y position
---@param duration number Duration of the tween animation in seconds
---@param ease? "linear"|"EaseInQuad"|"EaseOutQuad"|"EaseInOutQuad"|string Optional easing equation (defaults to "linear")
function Tween.Position(entity, targetX, targetY, duration, ease) end

--- GLOBAL FUNCTIONS ---

---Called at construction of the Entity
---@param self Entity
function OnCreate(self) end

---Called every frame
---@param self Entity
---@param dt number
function OnUpdate(self, dt) end

---@class InputEventEnum
---@field Unknown integer
---@field KeyDown integer
---@field Keydown integer
---@field KeyPressed integer
---@field KeyUp integer
---@field Keyup integer
---@field KeyReleased integer
---@field MouseDown integer
---@field Mousedown integer
---@field MousePressed integer
---@field MouseUp integer
---@field Mouseup integer
---@field MouseReleased integer
---@field MouseMove integer
---@field Mousemove integer
---@field MouseMoved integer
---@field MouseWheel integer
---@field Mousewheel integer
---@field MouseScroll integer
---@field TextEntered integer
---@field Text integer
---@field JoystickPressed integer
---@field JoystickReleased integer
---@field JoystickMoved integer
InputEvent = {}

---@type InputEventEnum
InputEventType = InputEvent

---@type InputEventEnum
EventType = InputEvent

---@class InputEventData
---@field type integer Event type ID (compare with InputEvent.KeyDown / InputEvent.Keydown, InputEvent.MouseDown etc.)
---@field typeName string Human-readable type ("KeyDown", "KeyUp", "MouseDown", "MouseUp", "MouseMove", "MouseWheel", "TextEntered")
---@field typeStr string Legacy string type ("key_pressed", "key_released", "mouse_pressed", "mouse_released", "mouse_moved", "mouse_wheel", "text_entered")
---@field key? integer Key code integer (compare with Key.Space, Key.W etc.)
---@field keyCode? integer Numeric key code
---@field keyName? string Name of key (e.g. "Space", "W", "Escape")
---@field button? integer Mouse button integer (compare with Mouse.Left, Mouse.Right, Mouse.Middle)
---@field buttonCode? integer Numeric mouse button code
---@field buttonName? string Name of mouse button ("Left", "Right", "Middle")
---@field x? number Screen mouse X position
---@field y? number Screen mouse Y position
---@field worldX? number World coordinate X mapped to camera view
---@field worldY? number World coordinate Y mapped to camera view
---@field delta? number Scroll wheel delta
---@field wheel? string "vertical" or "horizontal"
---@field text? string Character entered (for text_entered)
---@field unicode? integer Unicode codepoint
---@field alt? boolean Alt key state
---@field control? boolean Control key state
---@field shift? boolean Shift key state
---@field system? boolean System / Super key state
---@field joystickId? integer Joystick ID
---@field axis? integer Joystick axis index
---@field position? number Joystick axis position (-100 to 100)
---@field isInput boolean Always true
---@field isPaused boolean True if the game is currently paused

---Called only when input arrives (keyboard, mouse, joystick, text)
---@param self Entity
---@param event InputEventData Event details table
function OnInputReceived(self, event) end

---Alias for OnInputReceived (supports alternate spelling)
---@param self Entity
---@param event InputEventData Event details table
function OnInputReceiced(self, event) end

---Called when this entity collides with another
---@param self Entity
---@param other Entity
function OnCollision(self, other) end

---Called when a solid physical collision begins with another entity
---@param self Entity
---@param other Entity The colliding entity
---@param normalX number Collision contact normal X
---@param normalY number Collision contact normal Y
function OnCollisionEnter(self, other, normalX, normalY) end

---Called when entering a trigger / sensor collider
---@param self Entity
---@param other Entity The trigger entity entered
function OnTriggerEnter(self, other) end

---Called when an Entity or scene is destroyed / unloaded
---@param self Entity
function OnDestroy(self) end

---Called when a UI button is clicked
---@param id string The ID of the clicked button
function OnButtonClicked(id) end

---Called when the mouse cursor enters / hovers over a UI button
---@param id string The ID of the hovered button
function OnButtonHovered(id) end

---Called when a slider's value is changed
---@param id string The ID of the slider
---@param value number The new slider value
function OnSliderChanged(id, value) end

---Called when a checkbox state is toggled
---@param id string The ID of the checkbox
---@param checked boolean True if checked, false otherwise
function OnCheckboxChanged(id, checked) end

---Called when text in a TextInput element is changed
---@param id string The ID of the TextInput element
---@param text string The current text content
function OnTextInputChanged(id, text) end

---Called when the Enter key is pressed in a focused TextInput element
---@param id string The ID of the TextInput element
---@param text string The submitted text content
function OnTextInputSubmitted(id, text) end

---Called when the hover state of any UI element changes
---@param id string The ID of the UI element
---@param hovered boolean True if mouse entered, false if mouse left
function OnUIHover(id, hovered) end

---Called when a UI element gains or loses keyboard/input focus
---@param id string The ID of the UI element
---@param focused boolean True if element gained focus, false if lost focus
function OnUIFocus(id, focused) end

---Creates a new Entity ID
---@return Entity
function CreateEntity() end

---Loads a different scene from the assets/scenes folder
---@param sceneName string The name of the scene (without the .json extension)
function LoadScene(sceneName) end

---Quits the game. In Standalone, closes the application. In Editor, returns to the editor.
function QuitGame() end

---Restarts the currently active scene, resetting all entities, UI, and scripts.
function RestartScene() end

---Pauses or unpauses game simulation (physics, timers, tweens). Scripts receive dt=0 while paused.
---@param paused boolean
function PauseGame(paused) end

---Sets the game simulation time scale (e.g. 0.5 for half speed, 2.0 for double speed).
---@param scale number
function SetTimeScale(scale) end

---Returns the current frames per second (FPS).
---@return number
function GetFPS() end

---@class Engine
Engine = {}

---Quits the game. In Standalone, closes the application. In Editor, returns to the editor.
function Engine.Quit() end

---Restarts the currently active scene, resetting all entities, UI, and scripts.
function Engine.RestartScene() end

---Restarts the currently active scene, resetting all entities, UI, and scripts (alias for RestartScene).
function Engine.RestartCurrentScene() end

---Loads a different scene from the assets/scenes folder
---@param sceneName string The name of the scene (without the .json extension)
function Engine.LoadScene(sceneName) end

---Pauses or unpauses game simulation (physics, timers, tweens). Scripts receive dt=0 while paused.
---@param paused boolean
function Engine.SetPaused(paused) end

---Returns true if the game simulation is currently paused.
---@return boolean
function Engine.IsPaused() end

---Returns true if the game simulation is currently paused (alias for IsPaused).
---@return boolean
function Engine.GetPaused() end

---Toggles the pause state of the game simulation.
function Engine.TogglePause() end

---Sets the game simulation time scale (e.g. 0.5 for half speed, 2.0 for double speed).
---@param scale number
function Engine.SetTimeScale(scale) end

---Gets the current game simulation time scale.
---@return number
function Engine.GetTimeScale() end

---Sets fullscreen mode.
---@param fullscreen boolean
function Engine.SetFullscreen(fullscreen) end

---Toggles fullscreen mode.
function Engine.ToggleFullscreen() end

---Returns true if the window is currently in fullscreen mode.
---@return boolean
function Engine.IsFullscreen() end

---Sets whether the mouse cursor is visible.
---@param visible boolean
function Engine.SetCursorVisible(visible) end

---Captures a screenshot and saves it to the screenshots/ folder.
---@param filename? string Optional custom filename or relative path
---@return string The saved filepath
function Engine.TakeScreenshot(filename) end

---Opens a URL or system path using the default OS handler.
---@param url string
function Engine.OpenURL(url) end

---Prints a highlighted message to the in-game console with a stylish teal [LOG] badge.
---@param ... any One or more values to print
function Engine.Log(...) end

---Prints a warning message to the in-game console with an amber [WARN] badge.
---@param msg string The warning message to print
function Engine.LogWarning(msg) end

---Prints an error message to the in-game console with a red [ERROR] badge.
---@param msg string The error message to print
function Engine.LogError(msg) end

---Displays an on-screen notification toast during editor play mode (hidden in standalone build).
---@param msg string The message to display on screen
---@param duration? number How long to display the message in seconds (default: 3.5)
---@param r? integer Optional red color component (0-255)
---@param g? integer Optional green color component (0-255)
---@param b? integer Optional blue color component (0-255)
function Engine.LogToScreen(msg, duration, r, g, b) end

---Displays an on-screen notification toast during editor play mode (hidden in standalone build).
---@param msg string The message to display on screen
---@param duration? number How long to display the message in seconds (default: 3.5)
---@param r? integer Optional red color component (0-255)
---@param g? integer Optional green color component (0-255)
---@param b? integer Optional blue color component (0-255)
function LogToScreen(msg, duration, r, g, b) end

---Returns the current frames per second (FPS).
---@return number
function Engine.GetFPS() end

---Returns the unscaled delta time of the current frame in seconds.
---@return number
function Engine.GetDeltaTime() end

---Enables or disables the on-screen FPS counter overlay.
---@param show boolean
function Engine.ShowFPS(show) end

---Returns whether the on-screen FPS counter overlay is visible.
---@return boolean
function Engine.IsFPSShown() end

---Sets the Position of an Entity
---@param e Entity
---@param x number
---@param y number
function SetPosition(e, x, y)  end

---Sets the Rotation of an Entity in degrees
---@param e Entity
---@param r number Rotation angle in degrees
function SetRotation(e, r) end

---Sets the Scale of an Entity
---@param e Entity
---@param sx number Horizontal scale factor
---@param sy number Vertical scale factor
function SetScale(e, sx, sy) end

---@class Template
---@field path string Path to the .template file
---@field Instantiate fun(self: Template, x: number, y: number, parent?: Entity): Entity

---Creates a Template reference (used for export variables or spawning)
---@param path string Path to the .template file (e.g. "assets/templates/bullet.template")
---@return Template
function Template(path) end

---@class Image
---@field __type string
---@field path string Path to the image asset file

---Creates an Image reference (used for export variables or texture paths)
---@param path? string Path to the image file (e.g. "assets/sprites/character.png")
---@return Image
function Image(path) end

---@class Vec2
---@field __type string
---@field x number
---@field y number

---Creates a 2D Vector (used for export variables or coordinates)
---@param x? number X coordinate (defaults to 0.0)
---@param y? number Y coordinate (defaults to 0.0)
---@return Vec2
function Vec2(x, y) end

---@class Color
---@field __type string
---@field r integer Red component (0-255)
---@field g integer Green component (0-255)
---@field b integer Blue component (0-255)

---Creates a Color value (used for export variables pickable via the Inspector)
---@param r? integer Red component (defaults to 255)
---@param g? integer Green component (defaults to 255)
---@param b? integer Blue component (defaults to 255)
---@return Color
function Color(r, g, b) end

---@class EntityRef
---@field __type string
---@field name string Entity name in the hierarchy

---Creates an Entity reference (used for export variables referencing scene objects)
---@param name? string The name of the target entity in the scene
---@return EntityRef
function Entity(name) end

---Instantiates an entity from a template at the specified coordinates
---@param template Template|string The template object or path string
---@param x number X coordinate
---@param y number Y coordinate
---@param parent? Entity Optional parent entity
---@return Entity The root entity ID created
function Instantiate(template, x, y, parent) end

---Sets the parent of an Entity
---@param child Entity The child entity
---@param parent Entity The new parent entity (or 0 to unparent)
---@param keepWorldTransform? boolean Whether to maintain world transform (defaults to true)
function SetParent(child, parent, keepWorldTransform) end

---Gets the parent Entity ID of an entity
---@param child Entity The child entity
---@return Entity The parent entity ID, or 0 if none
function GetParent(child) end

---Gets the children Entity IDs of an entity
---@param parent Entity The parent entity
---@return Entity[] An array of child entity IDs
function GetChildren(parent) end

---Gets the world position of an Entity
---@param e Entity
---@return number x, number y
function GetWorldPosition(e) end

---Gets the world rotation of an Entity
---@param e Entity
---@return number
function GetWorldRotation(e) end

---Gets the world scale of an Entity
---@param e Entity
---@return number sx, number sy
function GetWorldScale(e) end

---Adds a Transform Component to an Entity
---@param e Entity
---@param x number
---@param y number
function AddTransform(e, x, y) end

---Returns the Transform Component of an Entity
---@param e Entity
---@return Transform
function GetTransform(e) end

---Destroys the Entity
---@param e Entity
function DestroyEntity(e) end

---Checks if an Entity has a Transform Component
---@param e Entity
---@return boolean
function HasTransform(e) end

---Adds a Velocity Component to an Entity
---@param e Entity
---@param dx number
---@param dy number
function AddVelocity(e, dx, dy) end

---Returns the Velocity Component of an Entity
---@param e Entity
---@return Velocity
function GetVelocity(e) end

---Sets the Velocity of an Entity
---@param e Entity
---@param dx number
---@param dy number
function SetVelocity(e, dx, dy) end

---Checks if an Entity has a Velocity Component
---@param e Entity
---@return boolean
function HasVelocity(e) end

---Removes the Velocity Component from an Entity
---@param e Entity
function RemoveVelocity(e) end

---Adds a Sprite Component to an Entity
---@param e Entity
---@param path string
---@param width number
---@param height number
function AddSprite(e, path, width, height) end

---Sets or replaces the Sprite texture of an Entity
---@param e Entity
---@param path string
function SetSprite(e, path) end

---Sets the rendered Sprite size of an Entity
---@param e Entity
---@param width number
---@param height number
function SetSpriteSize(e, width, height) end

---Checks if an Entity has a Sprite Component
---@param e Entity
---@return boolean
function HasSprite(e) end

---Sets the render color of an Entity (tint or shape color)
---@param e Entity
---@param r number
---@param g number
---@param b number
---@param a? number
function SetColor(e, r, g, b, a) end

---Adds a Text Component to an Entity
---@param e Entity
---@param text string
---@param size? number
function AddText(e, text, size) end

---Sets the text of an Entity's Text Component
---@param e Entity
---@param text string
function SetText(e, text) end

---Gets the text of an Entity's Text Component
---@param e Entity
---@return string
function GetText(e) end

---Sets the font character size of an Entity's Text Component
---@param e Entity
---@param size number
function SetTextSize(e, size) end

---Sets the text color of an Entity's Text Component
---@param e Entity
---@param r number
---@param g number
---@param b number
---@param a? number
function SetTextColor(e, r, g, b, a) end

---Checks if an Entity has a Text Component
---@param e Entity
---@return boolean
function HasText(e) end

---Adds an Audio Source Component to an Entity
---@param e Entity
---@param path string
---@param volume? number
---@param pitch? number
---@param loop? boolean
function AddAudioSource(e, path, volume, pitch, loop) end

---Plays the audio clip of an Entity's Audio Source Component
---@param e Entity
function PlayAudio(e) end

---Sets the sound clip path of an Entity's Audio Source Component
---@param e Entity
---@param path string
function SetAudioClip(e, path) end

---Sets the volume of an Entity's Audio Source Component (0 - 100)
---@param e Entity
---@param volume number
function SetAudioVolume(e, volume) end

---Sets the pitch of an Entity's Audio Source Component
---@param e Entity
---@param pitch number
function SetAudioPitch(e, pitch) end

---Sets whether an Entity's Audio Source Component should loop
---@param e Entity
---@param loop boolean
function SetAudioLoop(e, loop) end

---Checks if an Entity has an Audio Source Component
---@param e Entity
---@return boolean
function HasAudioSource(e) end

---Adds a Particle Emitter Component to an Entity
---@param e Entity
function AddParticleEmitter(e) end

---Sets whether a Particle Emitter is currently emitting
---@param e Entity
---@param emitting boolean
function SetParticleEmitting(e, emitting) end

---Checks whether a Particle Emitter is currently emitting
---@param e Entity
---@return boolean
function IsParticleEmitting(e) end

---Sets the emission rate (particles per second)
---@param e Entity
---@param rate number
function SetParticleRate(e, rate) end

---Sets the particle speed
---@param e Entity
---@param speed number
function SetParticleSpeed(e, speed) end

---Checks if an Entity has a Particle Emitter Component
---@param e Entity
---@return boolean
function HasParticleEmitter(e) end

---@enum CameraMultiFollowMode
CameraMultiFollowMode = {
    Priority = 0,
    Average = 1,
    AutoFrame = 2,
}

---@class CameraComponent
---@field active boolean
---@field smoothSpeed number Smooth follow speed (0 for instant snapping, >0 for smooth lerp)
---@field offsetX number Horizontal follow offset in world units
---@field offsetY number Vertical follow offset in world units
---@field zoom number Camera zoom factor (1.0 = normal)
---@field priority integer Priority value (higher priority camera wins in Priority mode)
---@field multiFollowMode CameraMultiFollowMode|integer 0 = Priority, 1 = Average (Midpoint), 2 = AutoFrame (Smart framing)
---@field minZoom number Minimum allowed zoom for auto-framing (default: 0.3)
---@field maxZoom number Maximum allowed zoom for auto-framing (default: 3.0)
---@field autoFramePadding number World unit padding around framed targets (default: 200.0)
CameraComponent = {}

---Adds a Camera Component to an Entity
---@param e Entity
---@param smoothSpeed? number Smooth follow speed (0 for instant snapping, >0 for smooth lerp)
---@param offsetX? number Follow offset X in world units
---@param offsetY? number Follow offset Y in world units
---@param zoom? number Camera zoom factor (default: 1.0)
---@param priority? integer Camera priority value (default: 0)
---@return CameraComponent
function AddCamera(e, smoothSpeed, offsetX, offsetY, zoom, priority) end

---Gets the Camera Component of an Entity, or nil if none exists
---@param e Entity
---@return CameraComponent|nil
function GetCamera(e) end

---Removes the Camera Component from an Entity
---@param e Entity
function RemoveCamera(e) end

---Checks if an Entity has a Camera Component
---@param e Entity
---@return boolean
function HasCamera(e) end

---Adds a Collision Component to an Entity
---@param e Entity
---@param channel? integer
function AddCollision(e, channel) end

---Removes the Collision Component from an Entity
---@param e Entity
function RemoveCollision(e) end

---Checks if an Entity has a Collision Component
---@param e Entity
---@return boolean
function HasCollision(e) end

---Sets the collision channel for an Entity
---@param e Entity
---@param channel integer
function SetCollisionChannel(e, channel) end

---Gets the collision channel for an Entity
---@param e Entity
---@return integer
function GetCollisionChannel(e) end

---Sets the collision type for an Entity ("solid" or "static")
---@param e Entity
---@param type string
function SetCollisionType(e, type) end

---Gets the collision type for an Entity
---@param e Entity
---@return string
function GetCollisionType(e) end

---Adds a Tag component to an Entity
---@param e Entity
---@param tag string
function AddTag(e, tag) end

---Sets the Tag of an Entity (adds Tag component if not present)
---@param e Entity
---@param tag string
function SetTag(e, tag) end

---Gets the Tag of an Entity, or empty string if none
---@param e Entity
---@return string
function GetTag(e) end

---Checks if an Entity has a Tag component
---@param e Entity
---@return boolean
function HasTag(e) end

---Finds the first Entity in the scene matching the given Tag, or 0 if none found
---@param tag string
---@return Entity
function FindEntityWithTag(tag) end

---Finds all entities in the scene matching the given Tag
---@param tag string
---@return Entity[]
function FindEntitiesWithTag(tag) end

---Adds or sets the Name of an Entity
---@param e Entity
---@param name string
function SetName(e, name) end

---Gets the Name of an Entity, or empty string if none
---@param e Entity
---@return string
function GetName(e) end

---Checks if an Entity has a Name component
---@param e Entity
---@return boolean
function HasName(e) end

---Finds the first Entity in the scene matching the given Name, or 0 if none found
---@param name string
---@return Entity
function FindEntityWithName(name) end

---Finds all entities in the scene matching the given Name
---@param name string
---@return Entity[]
function FindEntitiesWithName(name) end

---Finds the first Entity in the scene matching the given Name or Tag, or 0 if none found
---@param identifier string
---@return Entity
function FindEntity(identifier) end

---Checks if an Entity is valid and alive in the scene
---@param e Entity
---@return boolean
function IsEntityValid(e) end

---Gets the script environment table of an Entity (target can be Entity ID, string name/tag, or Entity wrapper)
---Allows calling methods and accessing variables directly, e.g.: GetScript(other).TakeDamage(10)
---@param target Entity|string|table
---@return table|nil
function GetScript(target) end

---Checks if an Entity has a script component attached
---@param target Entity|string|table
---@return boolean
function HasScript(target) end

---Safely calls a function on another entity's script if it exists
---@param target Entity|string|table
---@param funcName string
---@param ... any
---@return any
function CallScript(target, funcName, ...) end

---@class EntityHandle
---@field id Entity
---@field name string
---@field GetId fun(self: EntityHandle): Entity
---@field GetName fun(self: EntityHandle): string
---@field GetTag fun(self: EntityHandle): string
---@field SetTag fun(self: EntityHandle, tag: string)
---@field GetScript fun(self: EntityHandle): table|nil
---@field HasScript fun(self: EntityHandle): boolean
---@field IsValid fun(self: EntityHandle): boolean
---@field Destroy fun(self: EntityHandle)
---@field GetTransform fun(self: EntityHandle): Transform|nil
---@field SetPosition fun(self: EntityHandle, x: number, y: number)
---@field GetPosition fun(self: EntityHandle): number, number
---@field GetRotation fun(self: EntityHandle): number
---@field SetRotation fun(self: EntityHandle, r: number)
---@field GetScale fun(self: EntityHandle): number, number
---@field SetScale fun(self: EntityHandle, sx: number, sy: number)
---@field GetVelocity fun(self: EntityHandle): number, number
---@field SetVelocity fun(self: EntityHandle, dx: number, dy: number)

---Creates an Entity wrapper table that forwards method calls and property access directly to the entity's script.
---Allows syntax like Enemy.BlaBlaBla() or Enemy:TakeDamage(10) or Enemy.health
---@param nameOrId? string|number|table
---@return EntityHandle
function Entity(nameOrId) end

---Alias for Entity(nameOrId)
---@param nameOrId? string|number|table
---@return EntityHandle
function GetEntity(nameOrId) end

---@type Entity
self_entity = nil -- The ID of the current Entity

--- RESOURCE MANAGEMENT ---

---@class Resource
Resource = {}

---Preloads and caches a texture
---@param path string
---@return boolean
function Resource.PreloadTexture(path) end

---Preloads and caches a font
---@param path string
---@return boolean
function Resource.PreloadFont(path) end

---Preloads and caches a sound buffer
---@param path string
---@return boolean
function Resource.PreloadSound(path) end

---Clears all cached textures
function Resource.ClearTextures() end

---Clears all cached fonts
function Resource.ClearFonts() end

---Clears all cached sounds
function Resource.ClearSounds() end

---Clears all cached assets
function Resource.ClearAll() end

---Returns the number of cached textures
---@return number
function Resource.TextureCount() end

---Returns the number of cached fonts
---@return number
function Resource.FontCount() end

---Returns the number of cached sound buffers
---@return number
function Resource.SoundCount() end

---Prints resource cache statistics to console
function Resource.PrintStats() end

--- AUDIO ---

---@class Audio
Audio = {}

---Plays a sound effect
---@param path string
---@param volume? number Default: 100
---@param pitch? number Default: 1.0
function Audio.PlaySound(path, volume, pitch) end

---Stops all playing sound effects
function Audio.StopAllSounds() end

---Plays background music
---@param path string
---@param loop? boolean Default: true
---@param volume? number Default: 100
function Audio.PlayMusic(path, loop, volume) end

---Stops background music
function Audio.StopMusic() end

---Pauses background music
function Audio.PauseMusic() end

---Resumes paused background music
function Audio.ResumeMusic() end

---Sets music volume (0 - 100)
---@param volume number
function Audio.SetMusicVolume(volume) end

---Sets master volume for both sound effects and music (0 - 100)
---@param volume number
function Audio.SetMasterVolume(volume) end

--- INPUT ---

---@class Input
Input = {}

---Returns true while the key is held down
---@param key number|string Key code (e.g. Key.W) or key name (e.g. "W", "Space", "Left")
---@return boolean
function Input.IsKeyDown(key) end

---Returns true on the frame the key was pressed
---@param key number|string Key code or key name
---@return boolean
function Input.IsKeyPressed(key) end

---Returns true on the frame the key was pressed (alias for IsKeyPressed)
---@param key number|string Key code or key name
---@return boolean
function Input.IsKeyJustPressed(key) end

---Returns true on the frame the key was released
---@param key number|string Key code or key name
---@return boolean
function Input.IsKeyReleased(key) end

---Returns true while the mouse button is held down
---@param button number|string Mouse button code (e.g. Mouse.Left) or name ("Left", "Right", "Middle")
---@return boolean
function Input.IsMouseDown(button) end

---Returns true on the frame the mouse button was pressed
---@param button number|string Mouse button code or name
---@return boolean
function Input.IsMousePressed(button) end

---Returns true on the frame the mouse button was released
---@param button number|string Mouse button code or name
---@return boolean
function Input.IsMouseReleased(button) end

---Returns the mouse X position in screen coordinates
---@return number
function Input.MouseX() end

---Returns the mouse Y position in screen coordinates
---@return number
function Input.MouseY() end

---Returns the mouse scroll delta this frame
---@return number
function Input.MouseScroll() end

---Returns whether any input event was received in the current frame
---@return boolean
function Input.HasInput() end

---Returns the last input event received
---@return InputEventData|nil
function Input.GetLastEvent() end

---Converts a KeyCode integer to a human-readable key name
---@param key integer
---@return string
function Input.KeyToString(key) end

---Converts a Mouse button integer to a human-readable button name
---@param button integer
---@return string
function Input.MouseButtonToString(button) end

---@class Key
---@field A number
---@field B number
---@field C number
---@field D number
---@field E number
---@field F number
---@field G number
---@field H number
---@field I number
---@field J number
---@field K number
---@field L number
---@field M number
---@field N number
---@field O number
---@field P number
---@field Q number
---@field R number
---@field S number
---@field T number
---@field U number
---@field V number
---@field W number
---@field X number
---@field Y number
---@field Z number
---@field Space number
---@field Enter number
---@field Escape number
---@field LShift number
---@field RShift number
---@field LCtrl number
---@field RCtrl number
---@field Left number
---@field Right number
---@field Up number
---@field Down number
---@field Tab number
---@field Delete number
Key = {}

---@type Key
KeyCode = Key

---@class Mouse
---@field Left number
---@field Right number
---@field Middle number
Mouse = {}

--------------------------------------------------------------------------------
-- UI Subsystem API
--------------------------------------------------------------------------------

---@class UI
UI = {}

---Called when a UI button is clicked (can be defined as a callback)
---@type fun(id: string)|nil
UI.OnButtonClicked = nil

---Called when a UI button is hovered (can be defined as a callback)
---@type fun(id: string)|nil
UI.OnButtonHovered = nil

---Called when a UI slider's value changes (can be defined as a callback)
---@type fun(id: string, value: number)|nil
UI.OnSliderChanged = nil

---Called when a UI checkbox is toggled (can be defined as a callback)
---@type fun(id: string, checked: boolean)|nil
UI.OnCheckboxChanged = nil

---Called when a UI TextInput's text changes (can be defined as a callback)
---@type fun(id: string, text: string)|nil
UI.OnTextInputChanged = nil

---Called when the Enter key is pressed in a UI TextInput (can be defined as a callback)
---@type fun(id: string, text: string)|nil
UI.OnTextInputSubmitted = nil

---Called when the hover state of any UI element changes (can be defined as a callback)
---@type fun(id: string, hovered: boolean)|nil
UI.OnUIHover = nil

---Called when a UI element gains or loses focus (can be defined as a callback)
---@type fun(id: string, focused: boolean)|nil
UI.OnUIFocus = nil

---Checks whether a UI button with the specified ID was clicked in the current frame.
---@param id string The ID of the button configured in the UI Editor.
---@return boolean True if the button was clicked, false otherwise.
function UI.IsButtonClicked(id) end

---Checks whether a UI button is currently being hovered.
---@param id string The ID of the button.
---@return boolean True if the mouse is over the button, false otherwise.
function UI.IsButtonHovered(id) end

---Sets the text of a UI element (Text or Button).
---@param id string The ID of the UI element.
---@param text string The new text to display.
function UI.SetText(id, text) end

---Gets the text of a UI element.
---@param id string The ID of the UI element.
---@return string The current text string.
function UI.GetText(id) end

---Sets the position of a UI element on the 1920x1080 canvas.
---@param id string The ID of the UI element.
---@param x number The X coordinate.
---@param y number The Y coordinate.
function UI.SetPosition(id, x, y) end

---Sets the parent of a UI element
---@param childId string The child element ID
---@param parentId string The parent element ID (or "" to unparent)
---@param keepWorldPos? boolean Whether to maintain world position (defaults to true)
function UI.SetParent(childId, parentId, keepWorldPos) end

---Gets the parent ID of a UI element
---@param id string The element ID
---@return string The parent ID, or "" if none
function UI.GetParent(id) end

---Gets the child IDs of a UI element
---@param id string The element ID
---@return string[] Array of child element IDs
function UI.GetChildren(id) end

---Gets the world position of a UI element on the 1920x1080 canvas
---@param id string The element ID
---@return number x, number y
function UI.GetWorldPosition(id) end

---Sets the dimensions of a UI element.
---@param id string The ID of the UI element.
---@param width number The width in canvas units.
---@param height number The height in canvas units.
function UI.SetSize(id, width, height) end

---Sets the color of a UI element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a? integer Alpha (0-255, optional, defaults to 255).
function UI.SetColor(id, r, g, b, a) end

---Sets the Z-Index (depth layer) of a UI element. Higher values draw in front.
---@param id string The ID of the UI element.
---@param z integer The new Z-Index value.
function UI.SetZIndex(id, z) end

---Gets the Z-Index (depth layer) of a UI element.
---@param id string The ID of the UI element.
---@return integer The current Z-Index value.
function UI.GetZIndex(id) end

---Sets whether a UI element is visible. Hidden elements are not drawn and cannot be interacted with.
---@param id string The ID of the UI element.
---@param visible boolean True to show, false to hide.
function UI.SetVisible(id, visible) end

---Gets whether a UI element is visible.
---@param id string The ID of the UI element.
---@return boolean True if visible, false if hidden.
function UI.GetVisible(id) end

---Sets the opacity of a UI element (affects the alpha channel).
---@param id string The ID of the UI element.
---@param opacity number Alpha value from 0 (fully transparent) to 255 (fully opaque).
function UI.SetOpacity(id, opacity) end

---Sets the text style of a Text or Button element using a bitmask.
---@param id string The ID of the UI element.
---@param style integer Bitmask: 0=Regular, 1=Bold, 2=Italic, 4=Underline, 8=StrikeThrough. Combine with bitwise OR.
function UI.SetTextStyle(id, style) end

---Sets the horizontal text alignment for a Text element.
---@param id string The ID of the UI element.
---@param align integer 0=Left, 1=Center, 2=Right.
function UI.SetTextAlign(id, align) end

---Sets whether the text of a Text or Button element is displayed in uppercase.
---@param id string The ID of the UI element.
---@param upper boolean True to force uppercase display.
function UI.SetUpperCase(id, upper) end

---Sets the character (font) size of a Text or Button element.
---@param id string The ID of the UI element.
---@param size integer Font size in points.
function UI.SetFontSize(id, size) end

---Sets the letter spacing multiplier of a Text or Button element.
---@param id string The ID of the UI element.
---@param spacing number Multiplier. 1.0 is default spacing.
function UI.SetLetterSpacing(id, spacing) end

---Sets the line spacing multiplier of a Text or Button element.
---@param id string The ID of the UI element.
---@param spacing number Multiplier. 1.0 is default spacing.
function UI.SetLineSpacing(id, spacing) end

---Sets the outline color and thickness of the text on a Text or Button element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
---@param thickness number Outline thickness in pixels.
function UI.SetTextOutline(id, r, g, b, a, thickness) end

---Sets a manual pixel offset applied to the text position within a Text or Button element.
---@param id string The ID of the UI element.
---@param ox number Horizontal offset in canvas units.
---@param oy number Vertical offset in canvas units.
function UI.SetTextOffset(id, ox, oy) end

---Sets the text fill color of a Text or Button element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
function UI.SetTextColor(id, r, g, b, a) end

---Sets the outline (border) color and thickness of a Panel element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
---@param thickness number Border thickness in canvas units.
function UI.SetOutline(id, r, g, b, a, thickness) end

---Sets whether a Button element is disabled. Disabled buttons cannot be hovered or clicked and use the disabled color.
---@param id string The ID of the button.
---@param disabled boolean True to disable, false to enable.
function UI.SetDisabled(id, disabled) end

---Sets the texture image of an Image or Button element.
---@param id string The ID of the UI element.
---@param path string Path to the texture image file (assets/ prefix is optional).
function UI.SetTexture(id, path) end

---Sets the hover texture image of a Button element.
---@param id string The ID of the button.
---@param path string Path to the texture image file.
function UI.SetHoverTexture(id, path) end

---Sets the pressed texture image of a Button element.
---@param id string The ID of the button.
---@param path string Path to the texture image file.
function UI.SetPressedTexture(id, path) end

---Sets the checked texture image of a Checkbox element.
---@param id string The ID of the checkbox.
---@param path string Path to the texture image file.
function UI.SetCheckedTexture(id, path) end

---Sets whether a Checkbox element is checked.
---@param id string The ID of the checkbox.
---@param checked boolean
function UI.SetChecked(id, checked) end

---Gets whether a Checkbox element is checked.
---@param id string The ID of the checkbox.
---@return boolean
function UI.GetChecked(id) end

---Sets the slider value.
---@param id string The ID of the slider.
---@param value number
function UI.SetSliderValue(id, value) end

---Gets the slider value.
---@param id string The ID of the slider.
---@return number
function UI.GetSliderValue(id) end

---Sets the progress value (0.0 to 1.0).
---@param id string The ID of the progress bar.
---@param value number
function UI.SetProgressValue(id, value) end

---Gets the progress value.
---@param id string The ID of the progress bar.
---@return number
function UI.GetProgressValue(id) end

---Sets keyboard focus on a TextInput element.
---@param id string The ID of the input element.
---@param focused boolean
function UI.SetFocused(id, focused) end

---Gets whether a TextInput element is focused.
---@param id string The ID of the input element.
---@return boolean
function UI.GetFocused(id) end

---Sets the vertical text alignment of a UI element (0=Top, 1=Middle, 2=Bottom).
---@param id string The ID of the element.
---@param valign number 0=Top, 1=Middle, 2=Bottom.
function UI.SetTextVAlign(id, valign) end

---Sets the minimum value of a slider element.
---@param id string The ID of the slider.
---@param min number The minimum value.
function UI.SetSliderMin(id, min) end

---Sets the maximum value of a slider element.
---@param id string The ID of the slider.
---@param max number The maximum value.
function UI.SetSliderMax(id, max) end

---Gets the minimum value of a slider element.
---@param id string The ID of the slider.
---@return number
function UI.GetSliderMin(id) end

---Gets the maximum value of a slider element.
---@param id string The ID of the slider.
---@return number
function UI.GetSliderMax(id) end

---Gets whether a UI element is currently hovered by the mouse.
---@param id string The ID of the UI element.
---@return boolean
function UI.IsHovered(id) end

---Gets whether an interactive UI element is currently being pressed.
---@param id string The ID of the UI element.
---@return boolean
function UI.IsPressed(id) end

---Gets whether a UI element is disabled.
---@param id string The ID of the UI element.
---@return boolean
function UI.IsDisabled(id) end

---Checks whether a UI button with the specified ID was clicked in the current frame.
---@param id string The ID of the button configured in the UI Editor.
---@return boolean True if the button was clicked, false otherwise.
function UI_IsButtonClicked(id) end

---Sets the text of a UI element (Text or Button).
---@param id string The ID of the UI element.
---@param text string The new text to display.
function UI_SetText(id, text) end

---Gets the text of a UI element.
---@param id string The ID of the UI element.
---@return string The current text string.
function UI_GetText(id) end

---Sets the position of a UI element on the 1920x1080 canvas.
---@param id string The ID of the UI element.
---@param x number The X coordinate.
---@param y number The Y coordinate.
function UI_SetPosition(id, x, y) end

---Sets the parent of a UI element
---@param childId string The child element ID
---@param parentId string The parent element ID (or "" to unparent)
---@param keepWorldPos? boolean Whether to maintain world position (defaults to true)
function UI_SetParent(childId, parentId, keepWorldPos) end

---Gets the parent ID of a UI element
---@param id string The element ID
---@return string The parent ID, or "" if none
function UI_GetParent(id) end

---Gets the child IDs of a UI element
---@param id string The element ID
---@return string[] Array of child element IDs
function UI_GetChildren(id) end

---Gets the world position of a UI element on the 1920x1080 canvas
---@param id string The element ID
---@return number x, number y
function UI_GetWorldPosition(id) end

---Sets the dimensions of a UI element.
---@param id string The ID of the UI element.
---@param width number The width in canvas units.
---@param height number The height in canvas units.
function UI_SetSize(id, width, height) end

---Sets the color of a UI element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a? integer Alpha (0-255, optional, defaults to 255).
function UI_SetColor(id, r, g, b, a) end

---Sets the Z-Index (depth layer) of a UI element. Higher values draw in front.
---@param id string The ID of the UI element.
---@param z integer The new Z-Index value.
function UI_SetZIndex(id, z) end

---Gets the Z-Index (depth layer) of a UI element.
---@param id string The ID of the UI element.
---@return integer The current Z-Index value.
function UI_GetZIndex(id) end

---Returns whether a UI button is currently being hovered.
---@param id string The ID of the button.
---@return boolean True if the mouse is over the button, false otherwise.
function UI_IsButtonHovered(id) end

---Sets whether a UI element is visible. Hidden elements are not drawn and cannot be interacted with.
---@param id string The ID of the UI element.
---@param visible boolean True to show, false to hide.
function UI_SetVisible(id, visible) end

---Gets whether a UI element is visible.
---@param id string The ID of the UI element.
---@return boolean True if visible, false if hidden.
function UI_GetVisible(id) end

---Sets the opacity of a UI element (affects the alpha channel).
---@param id string The ID of the UI element.
---@param opacity number Alpha value from 0 (fully transparent) to 255 (fully opaque).
function UI_SetOpacity(id, opacity) end

---Sets the text style of a Text or Button element using a bitmask.
---@param id string The ID of the UI element.
---@param style integer Bitmask: 0=Regular, 1=Bold, 2=Italic, 4=Underline, 8=StrikeThrough. Combine with bitwise OR.
function UI_SetTextStyle(id, style) end

---Sets the horizontal text alignment for a Text element.
---@param id string The ID of the UI element.
---@param align integer 0=Left, 1=Center, 2=Right.
function UI_SetTextAlign(id, align) end

---Sets whether the text of a Text or Button element is displayed in uppercase.
---@param id string The ID of the UI element.
---@param upper boolean True to force uppercase display.
function UI_SetUpperCase(id, upper) end

---Sets the character (font) size of a Text or Button element.
---@param id string The ID of the UI element.
---@param size integer Font size in points.
function UI_SetFontSize(id, size) end

---Sets the letter spacing multiplier of a Text or Button element.
---@param id string The ID of the UI element.
---@param spacing number Multiplier. 1.0 is default spacing.
function UI_SetLetterSpacing(id, spacing) end

---Sets the line spacing multiplier of a Text or Button element.
---@param id string The ID of the UI element.
---@param spacing number Multiplier. 1.0 is default spacing.
function UI_SetLineSpacing(id, spacing) end

---Sets the outline color and thickness of the text on a Text or Button element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
---@param thickness number Outline thickness in pixels.
function UI_SetTextOutline(id, r, g, b, a, thickness) end

---Sets a manual pixel offset applied to the text position within a Text or Button element.
---@param id string The ID of the UI element.
---@param ox number Horizontal offset in canvas units.
---@param oy number Vertical offset in canvas units.
function UI_SetTextOffset(id, ox, oy) end

---Sets the text fill color of a Text or Button element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
function UI_SetTextColor(id, r, g, b, a) end

---Sets the outline (border) color and thickness of a Panel element.
---@param id string The ID of the UI element.
---@param r integer Red (0-255).
---@param g integer Green (0-255).
---@param b integer Blue (0-255).
---@param a integer Alpha (0-255).
---@param thickness number Border thickness in canvas units.
function UI_SetOutline(id, r, g, b, a, thickness) end

---Sets whether a Button element is disabled. Disabled buttons cannot be hovered or clicked and use the disabled color.
---@param id string The ID of the button.
---@param disabled boolean True to disable, false to enable.
function UI_SetDisabled(id, disabled) end

---@enum BodyType
BodyType = {
    Dynamic = 0,
    Kinematic = 1,
    Static = 2,
}

---@enum ColliderShape
ColliderShape = {
    Box = 0,
    Circle = 1,
}

---@class Rigidbody2D
---@field bodyType BodyType|integer 0 = Dynamic, 1 = Kinematic, 2 = Static
---@field mass number Mass in kilograms (default: 1.0)
---@field gravityScale number Multiplier for gravity acceleration (default: 1.0)
---@field restitution number Coefficient of restitution / bounciness (0.0 to 1.0)
---@field drag number Linear velocity damping factor (default: 0.0)
---@field freezeRotation boolean If true, rotation is locked
Rigidbody2D = {}

---Adds a 2D Rigidbody component to an entity
---@param entity Entity The entity to attach the rigidbody to
---@param bodyType? BodyType|integer Optional body type (0=Dynamic, 1=Kinematic, 2=Static, default: Dynamic)
---@param mass? number Optional mass in kg (default: 1.0)
---@param gravityScale? number Optional gravity scale multiplier (default: 1.0)
---@return Rigidbody2D
function AddRigidbody(entity, bodyType, mass, gravityScale) end

---Gets the 2D Rigidbody component of an entity, or nil if none exists
---@param entity Entity The entity to query
---@return Rigidbody2D|nil
function GetRigidbody(entity) end

---Returns whether an entity has a 2D Rigidbody component
---@param entity Entity The entity to check
---@return boolean
function HasRigidbody(entity) end

---@class RaycastResult
---@field hit boolean
---@field entity Entity
---@field pointX number
---@field pointY number
---@field normalX number
---@field normalY number
---@field distance number
RaycastResult = {}

---@class Physics
Physics = {}

---Casts a ray into the scene and returns the closest hit
---@param startX number Ray start X
---@param startY number Ray start Y
---@param dirX number Ray direction X (will be normalized)
---@param dirY number Ray direction Y (will be normalized)
---@param distance number Maximum raycast distance
---@param channel? integer Optional collision channel filter (-1 for all)
---@return RaycastResult
function Physics.Raycast(startX, startY, dirX, dirY, distance, channel) end

---Applies a continuous force to an entity's Rigidbody (accumulated for the next physics step)
---@param entity Entity The entity to apply force to
---@param fx number Force along X axis
---@param fy number Force along Y axis
function Physics.ApplyForce(entity, fx, fy) end

---Applies an instant impulse to an entity (directly modifies velocity: v += impulse / mass)
---@param entity Entity The entity to apply impulse to
---@param ix number Impulse along X axis
---@param iy number Impulse along Y axis
function Physics.ApplyImpulse(entity, ix, iy) end

---Sets the velocity of an entity directly
---@param entity Entity The entity
---@param vx number Velocity along X axis
---@param vy number Velocity along Y axis
function Physics.SetVelocity(entity, vx, vy) end

---Gets the current velocity of an entity
---@param entity Entity The entity
---@return number vx, number vy The current linear velocity components
function Physics.GetVelocity(entity) end

---Sets the global physics gravity vector
---@param gx number Gravity acceleration along X axis
---@param gy number Gravity acceleration along Y axis (default: 980.0)
function Physics.SetGravity(gx, gy) end

---Gets the current global physics gravity vector
---@return number gx, number gy The current gravity acceleration components
function Physics.GetGravity() end

---Sets the fixed simulation timestep for physics
---@param dt number Timestep duration in seconds (default: 1/60 ~ 0.01667)
function Physics.SetFixedTimestep(dt) end

---Gets the fixed simulation timestep for physics
---@return number dt The timestep duration in seconds
function Physics.GetFixedTimestep() end

---@class Camera
Camera = {}

---Sets the absolute world position of the camera center
---@param x number World X coordinate
---@param y number World Y coordinate
function Camera.SetPosition(x, y) end

---Gets the current world position of the camera center
---@return number x, number y
function Camera.GetPosition() end

---Gets the current world X position of the camera center
---@return number
function Camera.GetX() end

---Gets the current world Y position of the camera center
---@return number
function Camera.GetY() end

---Moves the camera center by a relative delta (in world units)
---@param dx number Delta X
---@param dy number Delta Y
function Camera.Move(dx, dy) end

---Sets the camera zoom factor (1.0 = default 1:1, >1 = zoomed in, <1 = zoomed out)
---@param zoom number
function Camera.SetZoom(zoom) end

---Gets the current camera zoom factor
---@return number
function Camera.GetZoom() end

---Multiplies the current camera zoom by a factor (e.g. 1.1 to zoom in, 0.9 to zoom out)
---@param factor number
function Camera.Zoom(factor) end

---Sets the rotation of the camera in degrees
---@param deg number Angle in degrees
function Camera.SetRotation(deg) end

---Gets the current camera rotation in degrees
---@return number
function Camera.GetRotation() end

---Rotates the camera by a delta angle in degrees
---@param deltaDeg number Delta angle in degrees
function Camera.Rotate(deltaDeg) end

---Sets the base size of the camera view
---@param w number Width in units
---@param h number Height in units
function Camera.SetSize(w, h) end

---Gets the base size of the camera view
---@return number w, number h
function Camera.GetSize() end

---Resets the camera to default position, zoom (1.0), rotation (0.0), no shake, and no follow target
function Camera.Reset() end

---Follows an entity with optional smooth damping and positional offset
---@param entity Entity The entity to follow
---@param smoothSpeed? number Smooth lerp speed (0 = instant snapping, >0 = smooth lerp)
---@param offsetX? number Follow offset X in world units
---@param offsetY? number Follow offset Y in world units
function Camera.Follow(entity, smoothSpeed, offsetX, offsetY) end

---Stops following any target entity
function Camera.StopFollow() end

---Resumes following the target entity if one was set
function Camera.ResumeFollow() end

---Returns true if the camera is currently following an entity
---@return boolean
function Camera.IsFollowing() end

---Returns the entity currently followed by the camera (or 0 if none)
---@return Entity
function Camera.GetFollowTarget() end

---Sets the follow lerp speed (0 = instant, >0 = smooth lerp)
---@param speed number
function Camera.SetFollowSpeed(speed) end

---Gets the current follow lerp speed
---@return number
function Camera.GetFollowSpeed() end

---Sets the follow positional offset in world units
---@param ox number Offset X
---@param oy number Offset Y
function Camera.SetFollowOffset(ox, oy) end

---Gets the follow positional offset
---@return number ox, number oy
function Camera.GetFollowOffset() end

---Sets world boundaries that restrict camera movement
---@param minX number Left boundary
---@param minY number Top boundary
---@param maxX number Right boundary
---@param maxY number Bottom boundary
---@param clampEdges? boolean If true, camera edges won't show past bounds; if false, camera center is clamped (default: true)
function Camera.SetBounds(minX, minY, maxX, maxY, clampEdges) end

---Clears camera boundary limits
function Camera.ClearBounds() end

---Returns true if camera movement boundaries are active
---@return boolean
function Camera.HasBounds() end

---Gets the active camera boundaries (minX, minY, maxX, maxY)
---@return number minX, number minY, number maxX, number maxY
function Camera.GetBounds() end

---Triggers a screen shake effect
---@param intensity number Shake intensity in pixels/world units
---@param duration number Shake duration in seconds
---@param decay? boolean Whether the shake decays smoothly over duration (default: true)
function Camera.Shake(intensity, duration, decay) end

---Immediately stops any active screen shake
function Camera.StopShake() end

---Returns true if the camera is currently shaking
---@return boolean
function Camera.IsShaking() end

---Converts screen pixel coordinates to world coordinates taking current camera view and zoom into account
---@param sx number Screen X coordinate
---@param sy number Screen Y coordinate
---@return number wx, number wy
function Camera.ScreenToWorld(sx, sy) end

---Converts world coordinates to screen pixel coordinates taking current camera view and zoom into account
---@param wx number World X coordinate
---@param wy number World Y coordinate
---@return number sx, number sy
function Camera.WorldToScreen(wx, wy) end

---Sets the multi-target follow strategy
---@param mode CameraMultiFollowMode|integer|string 0 / "priority", 1 / "average", 2 / "auto_frame"
function Camera.SetMultiFollowMode(mode) end

---Gets the current multi-target follow strategy name ("priority", "average", "auto_frame")
---@return string
function Camera.GetMultiFollowMode() end

---Adds an entity to the multi-target camera follow group
---@param entity Entity The entity to follow
function Camera.AddFollowTarget(entity) end

---Removes an entity from the multi-target camera follow group
---@param entity Entity The entity to remove
function Camera.RemoveFollowTarget(entity) end

---Clears all entities from the multi-target follow group
function Camera.ClearFollowTargets() end

---Sets a group of entities for the camera to follow together
---@param targets Entity[] Array of entities to track
function Camera.FollowGroup(targets) end

---Returns the list of entities currently followed in the group
---@return Entity[]
function Camera.GetFollowTargets() end

---Sets the padding (margin) around framed targets when in AutoFrame mode
---@param padding number World units padding (default: 200.0)
function Camera.SetAutoFramePadding(padding) end

---Gets the padding around framed targets when in AutoFrame mode
---@return number
function Camera.GetAutoFramePadding() end

---Sets the minimum and maximum allowed zoom bounds for auto-framing
---@param minZoom number Minimum zoom (default: 0.3)
---@param maxZoom number Maximum zoom (default: 3.0)
function Camera.SetAutoFrameZoomLimits(minZoom, maxZoom) end

---Gets the minimum and maximum allowed zoom bounds for auto-framing
---@return number minZoom, number maxZoom
function Camera.GetAutoFrameZoomLimits() end

---Sets the designated primary camera entity (highest priority tiebreaker)
---@param entity Entity The primary camera entity
function Camera.SetPrimary(entity) end

---Gets the designated primary camera entity (or 0 if none)
---@return Entity
function Camera.GetPrimary() end

