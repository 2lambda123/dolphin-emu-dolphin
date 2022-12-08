#include "Lua.h"

#include <mutex>
#include <condition_variable>
#include <thread>

#include "LuaFunctions/LuaMemoryApi.h"
#include "LuaFunctions/LuaEmuFunctions.h"
#include "LuaFunctions/LuaBitFunctions.h"
#include "LuaFunctions/LuaConverter.h"
#include "LuaFunctions/LuaByteWrapper.h"
#include "Core/Movie.h"
#include <filesystem>

namespace Lua
{
std::thread luaThread;

void tempRunner()
{
  std::filesystem::path p = std::filesystem::current_path().filename();
  while (Movie::GetCurrentFrame() < 5)
    ;
  if (luaL_dofile(mainLuaState, "script.lua") != LUA_OK)
  {
    const char* tempString = lua_tostring(mainLuaState, -1);
    fprintf(stderr, "%s\n", tempString);
  }
}

void Init()
{
  LuaEmu::luaScriptActive = true;
  mainLuaState = luaL_newstate();
  luaL_openlibs(mainLuaState);
  LuaMemoryApi::InitLuaMemoryApi(mainLuaState);
  LuaEmu::InitLuaEmuFunctions(mainLuaState);
  LuaBit::InitLuaBitFunctions(mainLuaState);
  LuaConverter::InitLuaConverterFunctions(mainLuaState);
  LuaByteWrapper::InitLuaByteWrapperClass(mainLuaState);
  luaThread = std::thread(tempRunner);
}

void Shutdown()
{
  luaThread.join();
}
}
