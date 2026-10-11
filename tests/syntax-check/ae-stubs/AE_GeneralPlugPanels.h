// Stand-in for the Adobe SDK header of the same name (see AEConfig.h in this directory): only what this
// repository uses.
#pragma once

#include "AE_GeneralPlug.h"

#include <Cocoa/Cocoa.h>

typedef NSView* AEGP_PlatformViewRef;
typedef struct _AEGP_CreatePanelRefcon* AEGP_CreatePanelRefcon;
typedef struct _AEGP_PanelRefcon* AEGP_PanelRefcon;
typedef struct _AEGP_PanelH* AEGP_PanelH;

typedef A_long AEGP_FlyoutMenuCmdID;
typedef A_long AEGP_FlyoutMenuMarkType;
typedef A_long AEGP_FlyoutMenuIndent;
enum { AEGP_FlyoutMenuMarkType_NORMAL, AEGP_FlyoutMenuMarkType_CHECKED };

typedef struct {
  AEGP_FlyoutMenuIndent indent;
  AEGP_FlyoutMenuMarkType type;
  A_Boolean enabledB;
  AEGP_FlyoutMenuCmdID cmdID;
  const A_u_char* utf8NameZ;
} AEGP_FlyoutMenuItem;

typedef struct {
  A_Err (*GetSnapSizes)(AEGP_PanelRefcon, A_LPoint*, A_long*);
  A_Err (*PopulateFlyout)(AEGP_PanelRefcon, AEGP_FlyoutMenuItem*, A_long*);
  A_Err (*DoFlyoutCommand)(AEGP_PanelRefcon, AEGP_FlyoutMenuCmdID);
} AEGP_PanelFunctions1;

typedef A_Err (*AEGP_CreatePanelHook)(AEGP_GlobalRefcon, AEGP_CreatePanelRefcon, AEGP_PlatformViewRef,
                                      AEGP_PanelH, AEGP_PanelFunctions1*, AEGP_PanelRefcon*);

#define kAEGPPanelSuite "AEGP Workspace Panel Suite"
#define kAEGPPanelSuiteVersion1 1
typedef struct {
  SPAPI A_Err (*AEGP_RegisterCreatePanelHook)(AEGP_PluginID, const A_u_char*, AEGP_CreatePanelHook,
                                              AEGP_CreatePanelRefcon, A_Boolean);
  SPAPI A_Err (*AEGP_SetTitle)(AEGP_PanelH, const A_u_char*);
  SPAPI A_Err (*AEGP_ToggleVisibility)(const A_u_char*);
  SPAPI A_Err (*AEGP_IsShown)(const A_u_char*, A_Boolean*, A_Boolean*);
} AEGP_PanelSuite1;
