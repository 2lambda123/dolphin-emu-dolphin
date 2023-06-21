#include "Core/Scripting/HelperClasses/GCButtonFunctions.h"

#include "Core/Scripting/CoreScriptContextFiles/Enums/GCButtonNameEnum.h"

#include "string.h"

static bool IsEqualIgnoreCase(const char* string_1, const char* string_2)
{
  int index = 0;
  while (string_1[index] != '\0')
  {
    if (string_1[index] != string_2[index])
    {
      char first_char = string_1[index];
      if (first_char >= 65 && first_char <= 90)
        first_char += 32;
      char second_char = string_2[index];
      if (second_char >= 65 && second_char <= 90)
        second_char += 32;
      if (first_char != second_char)
        return false;
    }
    ++index;
  }

  return string_2[index] == '\0';
}

int ParseGCButton_impl(const char* button_name)
{
  if (strlen(button_name) == 1)
  {
    switch (button_name[0])
    {
    case 'a':
    case 'A':
      return (int) GcButtonNameEnum::A;

    case 'b':
    case 'B':
      return (int) GcButtonNameEnum::B;

    case 'x':
    case 'X':
      return (int) GcButtonNameEnum::X;

    case 'y':
    case 'Y':
      return (int) GcButtonNameEnum::Y;

    case 'z':
    case 'Z':
      return (int) GcButtonNameEnum::Z;

    case 'l':
    case 'L':
      return (int) GcButtonNameEnum::L;

    case 'r':
    case 'R':
      return (int) GcButtonNameEnum::R;

    default:
      return (int)GcButtonNameEnum::UnknownButton;
    }
  }

  switch (button_name[0])
  {
  case 'd':
  case 'D':
    if (IsEqualIgnoreCase(button_name, "dPadUp"))
      return (int)GcButtonNameEnum::DPadUp;
    else if (IsEqualIgnoreCase(button_name, "dPadDown"))
      return (int)GcButtonNameEnum::DPadDown;
    else if (IsEqualIgnoreCase(button_name, "dPadLeft"))
      return (int)GcButtonNameEnum::DPadLeft;
    else if (IsEqualIgnoreCase(button_name, "dPadRight"))
      return (int)GcButtonNameEnum::DPadRight;
    else if (IsEqualIgnoreCase(button_name, "disc"))
      return (int)GcButtonNameEnum::Disc;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 'a':
  case 'A':
    if (IsEqualIgnoreCase(button_name, "analogStickX"))
      return (int)GcButtonNameEnum::AnalogStickX;
    else if (IsEqualIgnoreCase(button_name, "analogStickY"))
      return (int)GcButtonNameEnum::AnalogStickY;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 'c':
  case 'C':
    if (IsEqualIgnoreCase(button_name, "cStickX"))
      return (int)GcButtonNameEnum::CStickX;
    else if (IsEqualIgnoreCase(button_name, "cStickY"))
      return (int)GcButtonNameEnum::CStickY;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 't':
  case 'T':
    if (IsEqualIgnoreCase(button_name, "triggerL"))
      return (int)GcButtonNameEnum::TriggerL;
    else if (IsEqualIgnoreCase(button_name, "triggerR"))
      return (int)GcButtonNameEnum::TriggerR;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 'r':
  case 'R':
    if (IsEqualIgnoreCase(button_name, "reset"))
      return (int)GcButtonNameEnum::Reset;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 's':
  case 'S':
    if (IsEqualIgnoreCase(button_name, "start"))
      return (int)GcButtonNameEnum::Start;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 'g':
  case 'G':
    if (IsEqualIgnoreCase(button_name, "getOrigin"))
      return (int)GcButtonNameEnum::GetOrigin;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  case 'i':
  case 'I':
    if (IsEqualIgnoreCase(button_name, "isConnected"))
      return (int)GcButtonNameEnum::IsConnected;
    else
      return (int)GcButtonNameEnum::UnknownButton;

  default:
    return (int)GcButtonNameEnum::UnknownButton;
  }
}

const char* ConvertButtonEnumToString_impl(int button)
{
  switch (button)
  {
  case GcButtonNameEnum::A:
    return "A";
  case GcButtonNameEnum::B:
    return "B";
  case GcButtonNameEnum::X:
    return "X";
  case GcButtonNameEnum::Y:
    return "Y";
  case GcButtonNameEnum::Z:
    return "Z";
  case GcButtonNameEnum::L:
    return "L";
  case GcButtonNameEnum::R:
    return "R";
  case GcButtonNameEnum::Start:
    return "Start";
  case GcButtonNameEnum::Reset:
    return "Reset";
  case GcButtonNameEnum::DPadUp:
    return "dPadUp";
  case GcButtonNameEnum::DPadDown:
    return "dPadDown";
  case GcButtonNameEnum::DPadLeft:
    return "dPadLeft";
  case GcButtonNameEnum::DPadRight:
    return "dPadRight";
  case GcButtonNameEnum::TriggerL:
    return "triggerL";
  case GcButtonNameEnum::TriggerR:
    return "triggerR";
  case GcButtonNameEnum::AnalogStickX:
    return "analogStickX";
  case GcButtonNameEnum::AnalogStickY:
    return "analogStickY";
  case GcButtonNameEnum::CStickX:
    return "cStickX";
  case GcButtonNameEnum::CStickY:
    return "cStickY";
  case GcButtonNameEnum::Disc:
    return "disc";
  case GcButtonNameEnum::GetOrigin:
    return "getOrigin";
  case GcButtonNameEnum::IsConnected:
    return "isConnected";
  default:
    return "";
  }
}

int IsValidButtonEnum_impl(int button)
{
  if (button >= 0 && button < GcButtonNameEnum::UnknownButton)
    return 1;
  else
    return 0;
}

int IsDigitalButton_impl(int raw_button_val)
{
  GcButtonNameEnum button_name = (GcButtonNameEnum)raw_button_val;

  switch (button_name)
  {
  case GcButtonNameEnum::A:
  case GcButtonNameEnum::B:
  case GcButtonNameEnum::Disc:
  case GcButtonNameEnum::DPadDown:
  case GcButtonNameEnum::DPadLeft:
  case GcButtonNameEnum::DPadRight:
  case GcButtonNameEnum::DPadUp:
  case GcButtonNameEnum::GetOrigin:
  case GcButtonNameEnum::IsConnected:
  case GcButtonNameEnum::L:
  case GcButtonNameEnum::R:
  case GcButtonNameEnum::Reset:
  case GcButtonNameEnum::Start:
  case GcButtonNameEnum::X:
  case GcButtonNameEnum::Y:
  case GcButtonNameEnum::Z:
    return 1;

  default:
    return 0;
  }
}
int IsAnalogButton_impl(int raw_button_val)
{
  GcButtonNameEnum button_name = (GcButtonNameEnum)raw_button_val;

  switch (button_name)
  {
  case GcButtonNameEnum::AnalogStickX:
  case GcButtonNameEnum::AnalogStickY:
  case GcButtonNameEnum::CStickX:
  case GcButtonNameEnum::CStickY:
  case GcButtonNameEnum::TriggerL:
  case GcButtonNameEnum::TriggerR:
    return 1;

  default:
    return 0;
  }
}
