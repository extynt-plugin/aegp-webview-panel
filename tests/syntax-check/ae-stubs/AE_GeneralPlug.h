// Stand-in for the SDK header of the same name (see AEConfig.h).
#pragma once

#include "AEConfig.h"

typedef A_long AEGP_PluginID;
typedef struct _AEGP_GlobalRefcon* AEGP_GlobalRefcon;
typedef struct _AEGP_CommandRefcon* AEGP_CommandRefcon;
typedef struct _AEGP_UpdateMenuRefcon* AEGP_UpdateMenuRefcon;
typedef struct _AEGP_DeathRefcon* AEGP_DeathRefcon;
typedef struct _AEGP_IdleRefcon* AEGP_IdleRefcon;
typedef struct _AEGP_Item** AEGP_ItemH;
typedef struct _AEGP_Comp** AEGP_CompH;
typedef A_long AEGP_Command;
typedef A_LegacyEnumType AEGP_WindowType;
typedef A_LegacyEnumType AEGP_MenuID;
typedef A_u_long AEGP_HookPriority;
typedef A_long AEGP_MemSize;

enum { AEGP_HP_BeforeAE = 0x1, AEGP_HP_AfterAE = 0x2 };
enum { AEGP_Menu_NONE = 0, AEGP_Menu_WINDOW = 5 };
#define AEGP_MENU_INSERT_SORTED (-2)

typedef struct {
  A_long slots[12];
} AEGP_ErrReportState;

typedef A_Err (*AEGP_CommandHook)(AEGP_GlobalRefcon, AEGP_CommandRefcon, AEGP_Command, AEGP_HookPriority,
                                  A_Boolean, A_Boolean*);
typedef A_Err (*AEGP_UpdateMenuHook)(AEGP_GlobalRefcon, AEGP_UpdateMenuRefcon, AEGP_WindowType);
typedef A_Err (*AEGP_DeathHook)(AEGP_GlobalRefcon, AEGP_DeathRefcon);
typedef A_Err (*AEGP_IdleHook)(AEGP_GlobalRefcon, AEGP_IdleRefcon, A_long*);

#define kAEGPMemorySuite "AEGP Memory Suite"
#define kAEGPMemorySuiteVersion1 1
typedef struct {
  SPAPI A_Err (*AEGP_FreeMemHandle)(AEGP_MemHandle);
  SPAPI A_Err (*AEGP_LockMemHandle)(AEGP_MemHandle, void**);
  SPAPI A_Err (*AEGP_UnlockMemHandle)(AEGP_MemHandle);
} AEGP_MemorySuite1;

#define kAEGPItemSuite "AEGP Item Suite"
#define kAEGPItemSuiteVersion8 13
typedef struct {
  SPAPI A_Err (*AEGP_GetItemName)(AEGP_PluginID, AEGP_ItemH, AEGP_MemHandle*);
} AEGP_ItemSuite8;

#define kAEGPCompSuite "AEGP Comp Suite"
#define kAEGPCompSuiteVersion8 18
typedef struct {
  SPAPI A_Err (*AEGP_GetItemFromComp)(AEGP_CompH, AEGP_ItemH*);
  SPAPI A_Err (*AEGP_GetMostRecentlyUsedComp)(AEGP_CompH*);
} AEGP_CompSuite8;

#define kAEGPUtilitySuite "AEGP Utility Suite"
#define kAEGPUtilitySuiteVersion4 10
typedef struct {
  SPAPI A_Err (*AEGP_StartQuietErrors)(AEGP_ErrReportState*);
  SPAPI A_Err (*AEGP_EndQuietErrors)(A_Boolean, AEGP_ErrReportState*);
  SPAPI A_Err (*AEGP_CauseIdleRoutinesToBeCalled)(void);
  SPAPI A_Err (*AEGP_ExecuteScript)(AEGP_PluginID, const A_char*, const A_Boolean, AEGP_MemHandle*,
                                    AEGP_MemHandle*);
} AEGP_UtilitySuite4;

#define kAEGPRegisterSuite "AEGP Register Suite"
#define kAEGPRegisterSuiteVersion5 6
typedef struct {
  SPAPI A_Err (*AEGP_RegisterCommandHook)(AEGP_PluginID, AEGP_HookPriority, AEGP_Command, AEGP_CommandHook,
                                          AEGP_CommandRefcon);
  SPAPI A_Err (*AEGP_RegisterUpdateMenuHook)(AEGP_PluginID, AEGP_UpdateMenuHook, AEGP_UpdateMenuRefcon);
  SPAPI A_Err (*AEGP_RegisterDeathHook)(AEGP_PluginID, AEGP_DeathHook, AEGP_DeathRefcon);
  SPAPI A_Err (*AEGP_RegisterIdleHook)(AEGP_PluginID, AEGP_IdleHook, AEGP_IdleRefcon);
} AEGP_RegisterSuite5;

#define kAEGPCommandSuite "AEGP Command Suite"
#define kAEGPCommandSuiteVersion1 1
typedef struct {
  SPAPI A_Err (*AEGP_GetUniqueCommand)(AEGP_Command*);
  SPAPI A_Err (*AEGP_InsertMenuCommand)(AEGP_Command, const A_char*, AEGP_MenuID, A_long);
  SPAPI A_Err (*AEGP_EnableCommand)(AEGP_Command);
  SPAPI A_Err (*AEGP_CheckMarkMenuCommand)(AEGP_Command, A_Boolean);
} AEGP_CommandSuite1;

#define kPFAppSuite "PF AE App Suite"
#define kPFAppSuiteVersion4 6
typedef struct PF_App_Color {
  A_u_short red;
  A_u_short green;
  A_u_short blue;
} PF_App_Color;
typedef struct PFAppSuite4 {
  SPAPI PF_Err (*PF_AppGetBgColor)(PF_App_Color*);
} PFAppSuite4;
