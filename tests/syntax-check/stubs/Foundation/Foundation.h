// Declarations of the Foundation, AppKit and WebKit API the sources use, so the Objective-C++ files can be
// syntax-checked on Linux. Nothing here links or runs.
#pragma once

#include <cstddef>
#include <cstdint>

#include "CoreFoundation/CoreFoundation.h"

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef double NSTimeInterval;
typedef double CGFloat;
typedef struct {
  CGFloat width, height;
} NSSize;
typedef signed char BOOL;
#define YES ((BOOL)1)
#define NO ((BOOL)0)
#define nil __null
#define Nil __null

typedef struct {
  unsigned long state;
  id __unsafe_unretained* itemsPtr;
  unsigned long* mutationsPtr;
  unsigned long extra[5];
} NSFastEnumerationState;

@class NSString;

@protocol NSObject
- (BOOL)isKindOfClass:(Class)aClass;
- (BOOL)respondsToSelector:(SEL)aSelector;
@end

@protocol NSFastEnumeration
- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState*)state
                                  objects:(id __unsafe_unretained*)buffer
                                    count:(NSUInteger)len;
@end

@protocol NSCopying
@end

@interface NSObject <NSObject>
+ (instancetype)alloc;
+ (instancetype)new;
- (instancetype)init;
+ (Class)class;
- (Class)class;
- (BOOL)isEqual:(id)other;
@end

typedef NSUInteger NSStringEncoding;
#define NSUTF8StringEncoding ((NSStringEncoding)4)

@class NSError;
@class NSCharacterSet;

@interface NSString : NSObject <NSCopying>
@property(readonly) const char* UTF8String;
@property(readonly) NSUInteger length;
@property(readonly, copy) NSString* lowercaseString;
+ (instancetype)stringWithUTF8String:(const char*)bytes;
+ (instancetype)stringWithContentsOfFile:(NSString*)path
                                encoding:(NSStringEncoding)enc
                                   error:(NSError**)error;
- (unsigned short)characterAtIndex:(NSUInteger)index;
- (BOOL)isEqualToString:(NSString*)other;
- (NSString*)stringByAppendingPathComponent:(NSString*)component;
- (NSString*)stringByTrimmingCharactersInSet:(NSCharacterSet*)set;
@end

@interface NSConstantString : NSString
@end

@interface NSCharacterSet : NSObject
+ (NSCharacterSet*)whitespaceAndNewlineCharacterSet;
@end

@interface NSNumber : NSObject
+ (NSNumber*)numberWithBool:(BOOL)value;
+ (NSNumber*)numberWithInt:(int)value;
- (long long)longLongValue;
@end

@interface NSArray<ObjectType> : NSObject <NSFastEnumeration>
@property(readonly) NSUInteger count;
+ (instancetype)arrayWithObjects:(const ObjectType*)objects count:(NSUInteger)cnt;
@end

@interface NSMutableArray<ObjectType> : NSArray <ObjectType>
+ (instancetype)array;
- (void)addObject:(ObjectType)object;
@end

@interface NSDictionary<KeyType, ObjectType> : NSObject
+ (instancetype)dictionaryWithObjects:(const ObjectType*)objects
                              forKeys:(const KeyType*)keys
                                count:(NSUInteger)cnt;
- (ObjectType)objectForKey:(KeyType)key;
@end

@interface NSHashTable<ObjectType> : NSObject <NSFastEnumeration>
@property(readonly) NSUInteger count;
@property(readonly, copy) NSArray<ObjectType>* allObjects;
+ (NSHashTable*)weakObjectsHashTable;
- (void)addObject:(ObjectType)object;
@end

@interface NSError : NSObject
@property(readonly, copy) NSString* localizedDescription;
@end

@interface NSURL : NSObject
@property(readonly, copy) NSString* absoluteString;
+ (instancetype)URLWithString:(NSString*)string;
- (NSURL*)URLByAppendingPathComponent:(NSString*)component;
- (NSURL*)URLByDeletingLastPathComponent;
@end

@interface NSURLRequest : NSObject
@property(readonly, copy) NSURL* URL;
+ (instancetype)requestWithURL:(NSURL*)url;
@end

@interface NSBundle : NSObject
@property(readonly, copy) NSURL* resourceURL;
+ (NSBundle*)bundleForClass:(Class)aClass;
@end

@interface NSUserDefaults : NSObject
+ (NSUserDefaults*)standardUserDefaults;
- (void)registerDefaults:(NSDictionary<NSString*, id>*)defaults;
@end

@interface NSOperationQueue : NSObject
+ (NSOperationQueue*)mainQueue;
@end

typedef NSString* NSNotificationName;

@interface NSNotification : NSObject
@end

@interface NSNotificationCenter : NSObject
+ (NSNotificationCenter*)defaultCenter;
- (id)addObserverForName:(NSNotificationName)name
                  object:(id)obj
                   queue:(NSOperationQueue*)queue
              usingBlock:(void (^)(NSNotification*))block;
- (void)removeObserver:(id)observer;
@end

extern "C" {
void NSLog(NSString* format, ...);
NSString* NSHomeDirectory(void);
NSString* NSStringFromClass(Class aClass);
SEL NSSelectorFromString(NSString* name);
}

@interface NSObject (KeyValueCoding)
- (void)setValue:(id)value forKey:(NSString*)key;
@end
