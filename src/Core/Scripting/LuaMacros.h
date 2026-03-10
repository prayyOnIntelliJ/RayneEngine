#ifndef RAYNEENGINE_LUAREGISTRATION_H
#define RAYNEENGINE_LUAREGISTRATION_H

#define LUA_REG(luaName, fn, retType, desc, ...) \
s_Lua.set_function(luaName, fn); \
s_ApiDocs.push_back({ \
luaName, retType, \
std::vector<std::pair<std::string,std::string>>{__VA_ARGS__}, \
desc \
});

#endif //RAYNEENGINE_LUAREGISTRATION_H