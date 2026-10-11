// Syntax-check stand-in (see Foundation/Foundation.h): the WebKit API the sources use.
#pragma once

#include "Cocoa/Cocoa.h"

typedef NSInteger WKNavigationActionPolicy;
typedef NSInteger WKNavigationType;
typedef NSInteger WKUserScriptInjectionTime;
enum : WKNavigationActionPolicy { WKNavigationActionPolicyCancel = 0, WKNavigationActionPolicyAllow = 1 };
enum : WKNavigationType { WKNavigationTypeLinkActivated = 0 };
enum : WKUserScriptInjectionTime {
  WKUserScriptInjectionTimeAtDocumentStart = 0,
  WKUserScriptInjectionTimeAtDocumentEnd = 1
};

@class WKWebView;
@class WKNavigation;
@class WKFrameInfo;
@class WKOpenPanelParameters;
@class WKScriptMessage;
@class WKUserContentController;

@interface WKFrameInfo : NSObject
@property(readonly, getter=isMainFrame) BOOL mainFrame;
@property(readonly, copy) NSURLRequest* request;
@end

@interface WKNavigationAction : NSObject
@property(readonly, copy) NSURLRequest* request;
@property(readonly) WKNavigationType navigationType;
@property(readonly, copy) WKFrameInfo* targetFrame;
@end

@interface WKScriptMessage : NSObject
@property(readonly, copy) id body;
@property(readonly) WKFrameInfo* frameInfo;
@end

@protocol WKScriptMessageHandler <NSObject>
- (void)userContentController:(WKUserContentController*)controller
      didReceiveScriptMessage:(WKScriptMessage*)message;
@end

@protocol WKNavigationDelegate <NSObject>
@optional
- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)navigationAction
                    decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler;
- (void)webViewWebContentProcessDidTerminate:(WKWebView*)webView;
- (void)webView:(WKWebView*)webView
    didFailProvisionalNavigation:(WKNavigation*)navigation
                       withError:(NSError*)error;
@end

@protocol WKUIDelegate <NSObject>
@optional
- (void)webView:(WKWebView*)webView
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler;
- (void)webView:(WKWebView*)webView
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                        initiatedByFrame:(WKFrameInfo*)frame
                       completionHandler:(void (^)(BOOL))completionHandler;
- (void)webView:(WKWebView*)webView
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler;
- (void)webView:(WKWebView*)webView
    runOpenPanelWithParameters:(WKOpenPanelParameters*)parameters
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))completionHandler;
@end

@interface WKUserScript : NSObject
- (instancetype)initWithSource:(NSString*)source
                 injectionTime:(WKUserScriptInjectionTime)injectionTime
              forMainFrameOnly:(BOOL)forMainFrameOnly;
@end

@interface WKUserContentController : NSObject
- (void)addUserScript:(WKUserScript*)userScript;
- (void)removeAllUserScripts;
- (void)addScriptMessageHandler:(id<WKScriptMessageHandler>)handler name:(NSString*)name;
@end

@interface WKWebsiteDataStore : NSObject
+ (WKWebsiteDataStore*)nonPersistentDataStore;
@end

@interface WKPreferences : NSObject
@end

@interface WKWebViewConfiguration : NSObject
@property(strong) WKWebsiteDataStore* websiteDataStore;
@property(strong) WKUserContentController* userContentController;
@property(strong) WKPreferences* preferences;
@end

@interface WKWebView : NSView
@property(nonatomic, weak) id<WKNavigationDelegate> navigationDelegate;
@property(nonatomic, weak) id<WKUIDelegate> UIDelegate;
@property(readonly, copy) WKWebViewConfiguration* configuration;
@property BOOL inspectable;
- (instancetype)initWithFrame:(NSRect)frame configuration:(WKWebViewConfiguration*)configuration;
- (void)loadRequest:(NSURLRequest*)request;
- (void)loadFileURL:(NSURL*)url allowingReadAccessToURL:(NSURL*)readAccessURL;
- (void)evaluateJavaScript:(NSString*)javaScript completionHandler:(void (^)(id, NSError*))completionHandler;
@end
