// Stand-in for the Adobe SDK header of the same name, for CI machines that do not have the SDK (it cannot be
// redistributed). Declares only what this repository uses, with the same names and signatures as the SDK.
// When AE_SDK_ROOT is set, check-syntax.sh uses the real headers instead and this directory is not on the
// path.
#pragma once

#include <cstdint>

#define SPAPI
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef int32_t A_long;
typedef uint32_t A_u_long;
typedef unsigned short A_u_short;
typedef char A_char;
typedef unsigned char A_u_char;
typedef unsigned char A_Boolean;
typedef A_long A_Err;
typedef A_long PF_Err;
typedef A_long A_LegacyEnumType;
typedef int32_t SPErr;
typedef int32_t int32;

enum { A_Err_NONE = 0 };
enum { PF_Err_NONE = 0 };

typedef struct {
  A_long x, y;
} A_LPoint;

typedef struct _Up_OpaqueMem** AEGP_MemHandle;

struct SPBasicSuite {
  SPAPI SPErr (*AcquireSuite)(const char* name, int32 version, const void** suite);
  SPAPI SPErr (*ReleaseSuite)(const char* name, int32 version);
};
