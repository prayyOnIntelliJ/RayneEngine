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
---@param start number
---@param endVal number
---@param factor number
---@return number
function MathR.Lerp(start, endVal, factor) end
---@param value number
---@param min number
---@param max number
---@return number
function MathR.ClampF(value, min, max) end

--- GLOBAL FUNCTIONS ---

---Called at construction of the Entity
function OnCreate() end

---Called every frame
function OnUpdate() end

---Creates a new Entity ID
---@return Entity
function CreateEntity() end

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