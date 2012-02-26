/*
 * libbrlapi - A library providing access to braille terminals for applications.
 *
 * Copyright (C) 2002-2010 by The BRLTTY Developers.
 *
 * libbrlapi comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_EU_BRLDEFS
#define BRLTTY_INCLUDED_EU_BRLDEFS

typedef enum {
  EU_IRIS_20             = 0X01,
  EU_IRIS_40             = 0X02,
  EU_IRIS_S20            = 0X03,
  EU_IRIS_S32            = 0X04,
  EU_IRIS_KB20           = 0X05,
  EU_IRIS_KB40           = 0X06,
  EU_ESYS_12             = 0X07,
  EU_ESYS_40             = 0X08,
  EU_ESYS_LIGHT_40       = 0X09,
  EU_ESYS_24             = 0X0A,
  EU_ESYS_64             = 0X0B,
  EU_ESYS_80             = 0X0C,
  EU_ESYTIME_32          = 0X0E,
  EU_ESYTIME_32_STANDARD = 0X0F
} EU_EsysirisModel;

typedef enum {
  EU_IRIS_OPT_UnimanualKeyboard    = 0X00000001,
  EU_IRIS_OPT_DialogueMode         = 0X00000002,
  EU_IRIS_OPT_Grade2Braille        = 0X00001000,
  EU_IRIS_OPT_MsnMessenger         = 0X00002000,
  EU_IRIS_OPT_DaisyReader          = 0X00004000,
  EU_IRIS_OPT_TelephoneExchange    = 0X00008000,
  EU_IRIS_OPT_Mathematics          = 0X00010000,
  EU_IRIS_OPT_Music                = 0X00020000,
  EU_IRIS_OPT_HqVoiceSynthesis     = 0X00040000,
  EU_IRIS_OPT_Documentation        = 0X00080000,
  EU_IRIS_OPT_FileExplorer         = 0X00100000,
  EU_IRIS_OPT_VocalMemo            = 0X00200000,
  EU_IRIS_OPT_PcSerial             = 0X00400000,
  EU_IRIS_OPT_PcEthernet           = 0X00800000,
  EU_IRIS_OPT_Editor               = 0X01000000,
  EU_IRIS_OPT_Spreadsheet          = 0X02000000,
  EU_IRIS_OPT_Internet             = 0X04000000,
  EU_IRIS_OPT_Calculator           = 0X08000000,
  EU_IRIS_OPT_ScientificCalculator = 0X10000000,
  EU_IRIS_OPT_Contact              = 0X20000000,
  EU_IRIS_OPT_Agenda               = 0X40000000,
  EU_IRIS_OPT_Libbraille           = 0X80000000
} EU_IrisOption;

typedef enum {
  EU_ESYS_OPT_Editor                = 0X00040001,
  EU_ESYS_OPT_Calculator            = 0X00040002,
  EU_ESYS_OPT_AlarmClock            = 0X00040004,
  EU_ESYS_OPT_Bluetooth             = 0X00000008,
  EU_ESYS_OPT_USB                   = 0X00000010,
  EU_ESYS_OPT_Readmath              = 0X00000100,
  EU_ESYS_OPT_Jaws                  = 0X0001000,
  EU_ESYS_OPT_WindowEyes            = 0X0002000,
  EU_ESYS_OPT_SuperNova             = 0X0004000,
  EU_ESYS_OPT_MobileSpeakPocket     = 0X01000000,
  EU_ESYS_OPT_MobileSpeakSmartphone = 0X02000000,
  EU_ESYS_OPT_Talks                 = 0X04000000,
  EU_ESYS_OPT_Orange                = 0X08000000,
  EU_ESYS_OPT_Tracker               = 0X10000000
} EU_EsysOption;

typedef enum {
  /* Iris linear and arrow keys */
  EU_CMD_L1    =  0,
  EU_CMD_L2    =  1,
  EU_CMD_L3    =  2,
  EU_CMD_L4    =  3,
  EU_CMD_L5    =  4,
  EU_CMD_L6    =  5,
  EU_CMD_L7    =  6,
  EU_CMD_L8    =  7,
  EU_CMD_Up    =  8,
  EU_CMD_Down  =  9,
  EU_CMD_Right = 10,
  EU_CMD_Left  = 11,

  /* Esytime function keys */
  EU_CMD_F1 =  0,
  EU_CMD_F2 =  1,
  EU_CMD_F3 =  2,
  EU_CMD_F4 =  3,
  EU_CMD_F8 =  4,
  EU_CMD_F7 =  5,
  EU_CMD_F6 =  6,
  EU_CMD_F5 =  7,

  /* Esys switches */
  EU_CMD_Switch1Right =  0,
  EU_CMD_Switch1Left  =  1,
  EU_CMD_Switch2Right =  2,
  EU_CMD_Switch2Left  =  3,
  EU_CMD_Switch3Right =  4,
  EU_CMD_Switch3Left  =  5,
  EU_CMD_Switch4Right =  6,
  EU_CMD_Switch4Left  =  7,
  EU_CMD_Switch5Right =  8,
  EU_CMD_Switch5Left  =  9,
  EU_CMD_Switch6Right = 10,
  EU_CMD_Switch6Left  = 11,

  /* Esys and Esytime joystick #1 */
  EU_CMD_LeftJoystickUp    = 16,
  EU_CMD_LeftJoystickDown  = 17,
  EU_CMD_LeftJoystickRight = 18,
  EU_CMD_LeftJoystickLeft  = 19,
  EU_CMD_LeftJoystickPress = 20, // activates internal menu

  /* Esys and Esytime joystick #2 */
  EU_CMD_RightJoystickUp    = 24,
  EU_CMD_RightJoystickDown  = 25,
  EU_CMD_RightJoystickRight = 26,
  EU_CMD_RightJoystickLeft  = 27,
  EU_CMD_RightJoystickPress = 28,
} EU_CommandKey;

typedef enum {
  EU_BRL_Dot1      =  0,
  EU_BRL_Dot2      =  1,
  EU_BRL_Dot3      =  2,
  EU_BRL_Dot4      =  3,
  EU_BRL_Dot5      =  4,
  EU_BRL_Dot6      =  5,
  EU_BRL_Dot7      =  6,
  EU_BRL_Dot8      =  7,
  EU_BRL_Backspace =  8,
  EU_BRL_Space     =  9
} EU_BrailleKey;

typedef enum {
  EU_SET_CommandKeys,
  EU_SET_BrailleKeys,
  EU_SET_RoutingKeys1,
  EU_SET_RoutingKeys2
} EU_KeySet;

#endif /* BRLTTY_INCLUDED_EU_BRLDEFS */ 