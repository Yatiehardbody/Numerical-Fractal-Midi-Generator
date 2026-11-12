#include <juce_gui_extra/juce_gui_extra.h>
#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
static WKWebView* findWebView(NSView* root){if([root isKindOfClass:[WKWebView class]])return (WKWebView*)root;for(NSView* child in [root subviews])if(auto* found=findWebView(child))return found;return nil;}
void snapshotEditor(juce::Component& c,const juce::String& path,std::function<void(bool)> complete){
 auto* peer=c.getPeer();auto* view=peer?findWebView((NSView*)peer->getNativeHandle()):nil;
 if(!view){complete(false);return;}
 NSString* filename=[NSString stringWithUTF8String:path.toRawUTF8()];
 [view takeSnapshotWithConfiguration:nil completionHandler:^(NSImage* image,NSError* error){
   if(!image||error){complete(false);return;}
   NSBitmapImageRep* bitmap=[NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
   NSData* png=[bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
   complete([png writeToFile:filename atomically:YES]);
 }];
}
