#pragma once
#include <string>
#include "Core/Scripting/HelperClasses/ArgHolder.h"
#include "Core/Scripting/HelperClasses/ArgTypeEnum.h"

#include "Core/Scripting/CoreScriptContextFiles/Implementations/ScriptContext_Implementation.h"

namespace Scripting
{
class FunctionMetadata
{
public:
  FunctionMetadata()
  {
    this->function_name = "";
    this->function_version = "";
    this->example_function_call = "";
    function_pointer = nullptr;
    return_type = (ArgTypeEnum)0;
    arguments_list = std::vector<ArgTypeEnum>();
  }

  FunctionMetadata(const char* new_func_name, const char* new_func_version,
                   const char* new_example_function_call,
                   ArgHolder* (*new_function_ptr)(ScriptContext*, std::vector<ArgHolder*>*),
                   ArgTypeEnum new_return_type, std::vector<ArgTypeEnum> new_arguments_list)
  {
    this->function_name = std::string(new_func_name);
    this->function_version = std::string(new_func_version);
    this->example_function_call = std::string(new_example_function_call);
    this->function_pointer = new_function_ptr;
    this->return_type = new_return_type;
    this->arguments_list = new_arguments_list;
  }

  std::string function_name;
  std::string function_version;
  std::string example_function_call;
  ArgHolder* (*function_pointer)(ScriptContext*, std::vector<ArgHolder*>*);
  ArgTypeEnum return_type;
  std::vector<ArgTypeEnum> arguments_list;
};
}  // namespace Scripting
