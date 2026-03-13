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

---@type Entity
self_entity = nil -- The ID of the current Entity

--- INPUT ---

---@class Input
Input = {}

---Returns true while the key is held down
---@param key number
---@return boolean
function Input.IsKeyDown(key) end

---Returns true on the frame the key was pressed
---@param key number
---@return boolean
function Input.IsKeyPressed(key) end

---Returns true on the frame the key was released
---@param key number
---@return boolean
function Input.IsKeyReleased(key) end

---Returns true while the mouse button is held down
---@param button number
---@return boolean
function Input.IsMouseDown(button) end

---Returns true on the frame the mouse button was pressed
---@param button number
---@return boolean
function Input.IsMousePressed(button) end

---Returns true on the frame the mouse button was released
---@param button number
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