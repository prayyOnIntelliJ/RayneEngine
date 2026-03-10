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
function OnCreate() end

---Called every frame
function OnUpdate(dt) end

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