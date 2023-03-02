#pragma once
#include <string>
#include "Core/Scripting/HelperClasses/ArgHolder.h"
#include "Core/Scripting/HelperClasses/ArgTypeEnum.h"
#include "Core/Scripting/HelperClasses/FunctionMetadata.h"
#include "Core/Scripting/HelperClasses/ClassMetadata.h"
#include <vector>

namespace Scripting::BitApi
{
extern const char* class_name;

ClassMetadata GetBitApiClassData(const std::string& api_version);

ArgHolder BitwiseAnd(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder BitwiseOr(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder BitwiseNot(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder BitwiseXor(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder LogicalAnd(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder LogicalOr(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder LogicalXor(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder LogicalNot(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder BitShiftLeft(ScriptContext* current_script, std::vector<ArgHolder>& args_list);
ArgHolder BitShiftRight(ScriptContext* current_script, std::vector<ArgHolder>& args_list);

}  // namespace Scripting::BitApi
