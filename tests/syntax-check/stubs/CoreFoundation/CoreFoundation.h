// Syntax-check stand-in (see Foundation/Foundation.h): the few CoreFoundation names the sources use.
#pragma once

typedef const void* CFTypeRef;

extern "C" CFTypeRef CFBridgingRetain(id object);
