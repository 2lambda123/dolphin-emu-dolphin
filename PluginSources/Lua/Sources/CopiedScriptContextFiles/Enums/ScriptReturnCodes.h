#ifndef SCRIPT_RETURN_CODES
#define SCRIPT_RETURN_CODES

#ifdef __cplusplus
extern "C" {
#endif

enum ScriptReturnCodes
{
  SuccessCode = 0, // duh
  DLLFileNotFoundError = 1,
  DLLComponentNotFoundError = 2,
  ScriptFileNotFoundError = 3,
  UnknownError = 4
};

#ifdef __cplusplus
}
#endif

#endif
