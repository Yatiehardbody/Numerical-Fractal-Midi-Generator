#include "MidiDrag.h"
#import <Cocoa/Cocoa.h>
@interface NMGFileDragView : NSView <NSDraggingSource> {
 NSString* midiPath;
 BOOL dragging;
}
-(void)setMidiPath:(NSString*)path;
@end
@implementation NMGFileDragView
-(BOOL)isOpaque{return NO;}
-(void)setMidiPath:(NSString*)path { [midiPath release]; midiPath=[path copy]; }
-(void)dealloc { [midiPath release]; [super dealloc]; }
-(void)mouseDown:(NSEvent*)event { dragging=NO; }
-(void)mouseDragged:(NSEvent*)event {
 if(dragging || !midiPath || ![[NSFileManager defaultManager] fileExistsAtPath:midiPath])return;
 dragging=YES;
 NSURL* url=[NSURL fileURLWithPath:midiPath];
 NSDraggingItem* item=[[NSDraggingItem alloc] initWithPasteboardWriter:url];
 NSPoint point=[self convertPoint:[event locationInWindow] fromView:nil];
 [item setDraggingFrame:NSMakeRect(point.x-16,point.y-16,32,32) contents:[[NSWorkspace sharedWorkspace] iconForFile:midiPath]];
 [self beginDraggingSessionWithItems:@[item] event:event source:self];
 [item release];
}
-(NSDragOperation)draggingSession:(NSDraggingSession*)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context {return NSDragOperationCopy;}
-(BOOL)ignoreModifierKeysForDraggingSession:(NSDraggingSession*)session {return YES;}
-(void)draggingSession:(NSDraggingSession*)session endedAtPoint:(NSPoint)p operation:(NSDragOperation)operation {dragging=NO;}
@end
struct NativeMidiDrag::Impl {NMGFileDragView* view=[[NMGFileDragView alloc] initWithFrame:NSZeroRect];~Impl(){[view removeFromSuperview];[view release];}};
NativeMidiDrag::NativeMidiDrag():impl(std::make_unique<Impl>()){}
NativeMidiDrag::~NativeMidiDrag()=default;
void NativeMidiDrag::setFile(const juce::File& f){[impl->view setMidiPath:[NSString stringWithUTF8String:f.getFullPathName().toRawUTF8()]];}
void NativeMidiDrag::place(juce::Component& owner,juce::Rectangle<int> r,bool visible){
 auto* peer=owner.getPeer();if(!peer){[impl->view setHidden:YES];return;}
 NSView* root=(NSView*)peer->getNativeHandle();
 if([impl->view superview]!=root){[impl->view removeFromSuperview];[root addSubview:impl->view positioned:NSWindowAbove relativeTo:nil];}
 auto origin=owner.localPointToGlobal(r.getPosition());auto local=peer->getComponent().getLocalPoint(nullptr,origin);
 const double scale=peer->getComponent().getWidth()>0?[root bounds].size.width/peer->getComponent().getWidth():1.0;
 double x=local.x*scale,y=local.y*scale,w=r.getWidth()*scale,h=r.getHeight()*scale;
 if(![root isFlipped])y=[root bounds].size.height-y-h;
 [impl->view setFrame:NSMakeRect(x,y,w,h)];[impl->view setHidden:!visible];
}
