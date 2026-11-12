#include <juce_gui_extra/juce_gui_extra.h>
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
static WKWebView* findWeb(NSView* root){if([root isKindOfClass:[WKWebView class]])return (WKWebView*)root;for(NSView* child in [root subviews])if(auto* found=findWeb(child))return found;return nil;}
void evaluateHostedEditor(juce::Component& editor,const juce::String& js,std::function<void(juce::String,bool)> callback){
 auto* peer=editor.getPeer();WKWebView* web=peer?findWeb((NSView*)peer->getNativeHandle()):nil;
 if(!web){callback("WebView not attached",false);return;}
 [web evaluateJavaScript:[NSString stringWithUTF8String:js.toRawUTF8()] completionHandler:^(id value,NSError* error){
  if(error)callback(juce::String([[error localizedDescription] UTF8String]),false);
  else callback(juce::String([[value description] UTF8String]),true);
 }];
}
