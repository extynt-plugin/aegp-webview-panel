// Syntax-check stand-in (see Foundation/Foundation.h): the AppKit API the sources use.
#pragma once

#include "Foundation/Foundation.h"

typedef NSUInteger NSEventMask;
typedef NSUInteger NSEventModifierFlags;
typedef NSUInteger NSEventType;
typedef NSUInteger NSAutoresizingMaskOptions;

enum : NSEventType { NSEventTypeLeftMouseDown = 1, NSEventTypeKeyDown = 10, NSEventTypeKeyUp = 11 };
enum : NSEventMask {
  NSEventMaskLeftMouseDown = 1 << 1,
  NSEventMaskKeyDown = 1 << 10,
  NSEventMaskKeyUp = 1 << 11
};
enum : NSEventModifierFlags {
  NSEventModifierFlagShift = 1 << 17,
  NSEventModifierFlagControl = 1 << 18,
  NSEventModifierFlagOption = 1 << 19,
  NSEventModifierFlagCommand = 1 << 20,
  NSEventModifierFlagDeviceIndependentFlagsMask = 0xffff0000UL
};
enum : NSAutoresizingMaskOptions { NSViewWidthSizable = 2, NSViewHeightSizable = 16 };

typedef struct {
  CGFloat x, y;
} NSPoint;
typedef struct {
  NSPoint origin;
  NSSize size;
} NSRect;

extern NSNotificationName const NSWindowDidBecomeKeyNotification;
extern NSNotificationName const NSApplicationDidBecomeActiveNotification;

@class NSEvent;
@class NSWindow;
@class NSView;

@interface NSResponder : NSObject
@property(readonly) NSResponder* nextResponder;
- (void)keyDown:(NSEvent*)event;
- (void)keyUp:(NSEvent*)event;
@end

@interface NSEvent : NSObject
@property(readonly) NSEventType type;
@property(readonly) NSEventModifierFlags modifierFlags;
@property(readonly) NSTimeInterval timestamp;
@property(readonly) NSInteger windowNumber;
@property(readonly) NSWindow* window;
@property(readonly) NSPoint locationInWindow;
@property(readonly) unsigned short keyCode;
@property(readonly, copy) NSString* charactersIgnoringModifiers;
+ (id)addLocalMonitorForEventsMatchingMask:(NSEventMask)mask handler:(NSEvent* (^)(NSEvent*))block;
+ (void)removeMonitor:(id)monitor;
@end

@interface NSView : NSResponder
@property(readonly) NSWindow* window;
@property(readonly) NSView* superview;
@property(readonly) NSRect bounds;
@property NSAutoresizingMaskOptions autoresizingMask;
- (instancetype)initWithFrame:(NSRect)frame;
- (BOOL)isDescendantOf:(NSView*)view;
- (NSView*)hitTest:(NSPoint)point;
- (NSPoint)convertPoint:(NSPoint)point fromView:(NSView*)view;
- (void)addSubview:(NSView*)view;
- (void)viewDidMoveToWindow;
- (void)viewDidUnhide;
- (BOOL)acceptsFirstMouse:(NSEvent*)event;
@end

@interface NSWindow : NSResponder
@property(readonly) NSResponder* firstResponder;
@property(readonly) BOOL keyWindow;
@property(readonly) BOOL canBecomeKeyWindow;
@property NSSize contentMinSize;
- (BOOL)makeFirstResponder:(NSResponder*)responder;
- (void)makeKeyWindow;
@end

@interface NSApplication : NSObject
@property(readonly) NSWindow* mainWindow;
- (BOOL)sendAction:(SEL)action to:(id)target from:(id)sender;
@end

extern NSApplication* NSApp;

@interface NSWorkspaceOpenConfiguration : NSObject
+ (instancetype)configuration;
@end

@interface NSWorkspace : NSObject
+ (NSWorkspace*)sharedWorkspace;
- (void)openURL:(NSURL*)url
        configuration:(NSWorkspaceOpenConfiguration*)configuration
    completionHandler:(void (^)(id, NSError*))handler;
@end
