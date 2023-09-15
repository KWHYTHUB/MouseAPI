#include <Sapphire/Loader.hpp>
#include "../include/API.hpp"

#ifdef SAPPHIRE_IS_MACOS

#include <Sapphire/cocos/platform/mac/CCEventDispatcher.h>
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <Sapphire/cocos/platform/mac/EAGLView.h>
#import <objc/runtime.h>
#include "Platform.hpp"

using namespace sapphire::prelude;
using namespace mouse;

using EventType = void(*)(id, SEL, NSEvent*);

@interface MacMouseEvent : NSObject {
    float m_xPosition;
    float m_yPosition;
}   

    +(MacMouseEvent*) sharedEvent;

    -(void) moved:(NSEvent*)event;
    -(void) down:(NSEvent*)event type:(MouseButton)type;
    -(void) up:(NSEvent*)event type:(MouseButton)type;

    -(cocos2d::CCPoint) getMousePosition;

@end

static MacMouseEvent* s_sharedEvent = nil;



@implementation MacMouseEvent

// i just want to get this working
#define SAPPHIRE_API_OBJC_SWIZZLE_METHODS(mouseType, prefix, originalPrefix)               \
static EventType originalPrefix##Dragged;                                               \
- (void)prefix##DraggedHook:(NSEvent*)event {                                           \
/*	originalPrefix##Dragged(self, @selector(prefix##Dragged:), event);                */\
	[[MacMouseEvent sharedEvent] moved:event];                                          \
}                                                                                       \
static EventType originalPrefix##Down;                                                  \
- (void)prefix##DownHook:(NSEvent*)event {                                              \
/*	originalPrefix##Down(self, @selector(prefix##Down:), event);                      */\
	[[MacMouseEvent sharedEvent] down:event type:MouseButton::mouseType];               \
}                                                                                       \
static EventType originalPrefix##Up;                                                    \
- (void)prefix##UpHook:(NSEvent*)event {                                                \
/*	originalPrefix##Up(self, @selector(prefix##Up:), event);                          */\
	[[MacMouseEvent sharedEvent] up:event type:MouseButton::mouseType];                 \
}

static EventType s_originalMouseMoved;
- (void)mouseMovedHook:(NSEvent*)event {
	// s_originalMouseMoved(self, @selector(mouseMoved:), event);
	[[MacMouseEvent sharedEvent] moved:event];
}
SAPPHIRE_API_OBJC_SWIZZLE_METHODS(Left, mouse, s_originalMouse)
SAPPHIRE_API_OBJC_SWIZZLE_METHODS(Right, rightMouse, s_originalRightMouse)
SAPPHIRE_API_OBJC_SWIZZLE_METHODS(Middle, otherMouse, s_originalOtherMouse)

#define SAPPHIRE_API_OBJC_SWIZZLE(method, swizzle, original)                               \
Method method##Method = class_getInstanceMethod(class_, @selector(method:));            \
Method swizzle##Method = class_getInstanceMethod([self class], @selector(swizzle:));    \
original = (decltype(original))method_getImplementation(method##Method);                \
method_exchangeImplementations(method##Method, swizzle##Method);


+ (void)load {
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		Class class_ = NSClassFromString(@"EAGLView");

		SAPPHIRE_API_OBJC_SWIZZLE(mouseMoved, mouseMovedHook, s_originalMouseMoved)

		SAPPHIRE_API_OBJC_SWIZZLE(mouseDragged, mouseDraggedHook, s_originalMouseDragged)
		SAPPHIRE_API_OBJC_SWIZZLE(mouseDown, mouseDownHook, s_originalMouseDown)
		SAPPHIRE_API_OBJC_SWIZZLE(mouseUp, mouseUpHook, s_originalMouseUp)

		SAPPHIRE_API_OBJC_SWIZZLE(rightMouseDragged, rightMouseDraggedHook, s_originalRightMouseDragged)
		SAPPHIRE_API_OBJC_SWIZZLE(rightMouseDown, rightMouseDownHook, s_originalRightMouseDown)
		SAPPHIRE_API_OBJC_SWIZZLE(rightMouseUp, rightMouseUpHook, s_originalRightMouseUp)

		SAPPHIRE_API_OBJC_SWIZZLE(otherMouseDragged, otherMouseDraggedHook, s_originalOtherMouseDragged)
		SAPPHIRE_API_OBJC_SWIZZLE(otherMouseDown, otherMouseDownHook, s_originalOtherMouseDown)
		SAPPHIRE_API_OBJC_SWIZZLE(otherMouseUp, otherMouseUpHook, s_originalOtherMouseUp)
	});
}

+(MacMouseEvent*) sharedEvent {
	@synchronized(self) {
		if (s_sharedEvent == nil) {
			s_sharedEvent = [[self alloc] init]; // assignment not done here
		}
	}
	
	return s_sharedEvent;
}

-(void) moved:(NSEvent*)event {
	NSPoint event_location = [event locationInWindow];
	NSPoint local_point = [[NSClassFromString(@"EAGLView") sharedEGLView] convertPoint:event_location fromView:nil];
	
	float x = local_point.x;
	float y = [[NSClassFromString(@"EAGLView") sharedEGLView] getHeight] - local_point.y;

	m_xPosition = x / [[NSClassFromString(@"EAGLView") sharedEGLView] frameZoomFactor];
	m_yPosition = y / [[NSClassFromString(@"EAGLView") sharedEGLView] frameZoomFactor];

	Loader::get()->queueInGDThread([=]() {
		auto event = MouseMoveEvent(Mouse::get()->getCapturingNode(), [self getMousePosition]);
		postMouseEventThroughTouches(
			event,
			(Mouse::get()->isHeld(MouseButton::Left) ?
				CCTOUCHMOVED :
				CCTOUCHOTHER)
		);
	});
}

-(void) down:(NSEvent*)event type:(MouseButton)type {
	[self moved: event];
	Mouse::get()->m_heldButtons.insert(type);

	Loader::get()->queueInGDThread([=]() {
		auto event = MouseClickEvent(
			Mouse::get()->getCapturingNode(),
			type, true,
			[self getMousePosition]
		);
		postMouseEventThroughTouches(
			event,
			(type == MouseButton::Left ?
				CCTOUCHBEGAN :
				CCTOUCHOTHER)
		);
	});
}

-(void) up:(NSEvent*)event type:(MouseButton)type {
	[self moved: event];
	Mouse::get()->m_heldButtons.erase(type);

	Loader::get()->queueInGDThread([=]() {
		auto event = MouseClickEvent(
			Mouse::get()->getCapturingNode(),
			type, false,
			[self getMousePosition]
		);
		postMouseEventThroughTouches(
			event,
			(type == MouseButton::Left ?
				CCTOUCHENDED :
				CCTOUCHOTHER)
		);
	});

	
}

-(cocos2d::CCPoint) getMousePosition {
	static auto cachedMousePos = CCPointZero;
	auto mpos = cocos2d::CCPoint(m_xPosition, m_yPosition);
	if (mpos == cachedMousePos) return cachedMousePos;
	
	auto winSize = CCDirector::sharedDirector()->getWinSize();
	auto winSizePx = CCDirector::sharedDirector()->getOpenGLView()->getViewPortRect();
	auto ratio_w = winSize.width / winSizePx.size.width;
	auto ratio_h = winSize.height / winSizePx.size.height;
	mpos.y = winSizePx.size.height - mpos.y;
	mpos.x *= ratio_w;
	mpos.y *= ratio_h;
	return mpos;
}

@end

#endif
