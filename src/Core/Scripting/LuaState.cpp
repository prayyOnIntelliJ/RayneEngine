#include "LuaState.h"

#include "../Math/MathR.h"

sol::state LuaState::s_Lua;

void LuaState::Init()
{
    s_Lua.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string
    );


}

sol::state& LuaState::GetLua()
{
    return s_Lua;
}

void LuaState::RegisterStatics()
{
    MathR::RegisterLua(s_Lua);
}
