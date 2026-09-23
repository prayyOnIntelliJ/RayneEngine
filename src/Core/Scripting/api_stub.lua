---@meta --

---@class Entity : integer -- Defines Entity as a number

---@class Transform
---@field x number
---@field y number
Transform = {}

---@class Velocity
---@field dx number
---@field dy number
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

--- GLOBAL FUNCTIONS ---

---Called at construction of the Entity
---@param self Entity
function OnCreate(self) end

---Called every frame
---@param self Entity
---@param dt number
function OnUpdate(self, dt) end

---Called when this entity collides with another
---@param self Entity
---@param other Entity
function OnCollision(self, other) end

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

---Loads a different scene from the assets/scenes folder
---@param sceneName string The name of the scene (without the .json extension)
function Engine.LoadScene(sceneName) end

---Pauses or unpauses game simulation (physics, timers, tweens). Scripts receive dt=0 while paused.
---@param paused boolean
function Engine.SetPaused(paused) end

---Returns true if the game simulation is currently paused.
---@return boolean
function Engine.IsPaused() end

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

---Adds a Camera Component to an Entity
---@param e Entity
function AddCamera(e) end

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

---@class Mouse
---@field Left number
---@field Right number
---@field Middle number
Mouse = {}

--------------------------------------------------------------------------------
-- UI Subsystem API
--------------------------------------------------------------------------------

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