// Linux syntax-check stand-in for the CoreFoundation names the sources use.
#pragma once

typedef const void* CFTypeRef;

extern "C" CFTypeRef CFBridgingRetain(id object);
