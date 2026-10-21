#include "../NativeWindow.h"

#import <Cocoa/Cocoa.h>
#import <objc/runtime.h>

#include <cstdint>
#include <functional>
#include <unordered_map>

namespace eokas
{
    namespace
    {
        std::unordered_map<void*, std::function<void()>> gSystemClose;

        NSWindow* nsWindow(void* window)
        {
            return (__bridge NSWindow*)window;
        }

        void* own(id object)
        {
            if (object == nil) {
                return nullptr;
            }
#if __has_feature(objc_arc)
            return (__bridge_retained void*)object;
#else
            return (__bridge void*)object;
#endif
        }

        void autoreleaseOwned(void* owned)
        {
            if (!owned) {
                return;
            }
            objc_autorelease((__bridge id)owned);
        }

        void ensureApp()
        {
            if (NSApp == nil) {
                [NSApplication sharedApplication];
            }
            if (![NSApp isRunning]) {
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            }
        }

        NSRect primaryFrame()
        {
            NSArray<NSScreen*>* screens = [NSScreen screens];
            if (screens.count == 0) {
                return NSMakeRect(0, 0, 0, 0);
            }
            return screens[0].frame;
        }
    }

    void notifySystemClose(void* key)
    {
        auto found = gSystemClose.find(key);
        if (found == gSystemClose.end()) {
            return;
        }
        std::function<void()> callback = std::move(found->second);
        gSystemClose.erase(found);
        if (callback) {
            callback();
        }
    }
}

@interface EokasNativeWindow : NSWindow
@end

@implementation EokasNativeWindow

- (BOOL)canBecomeKeyWindow
{
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end

@interface EokasNativeWindowView : NSView
@property (nonatomic, assign) void* owner;
@end

@implementation EokasNativeWindowView

- (BOOL)isFlipped
{
    return YES;
}

- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent*)event
{
    (void)event;
    return YES;
}

- (void)updateTrackingAreas
{
    [super updateTrackingAreas];
    NSArray<NSTrackingArea*>* existing = [self.trackingAreas copy];
    for (NSTrackingArea* area in existing) {
        [self removeTrackingArea:area];
    }
    NSTrackingAreaOptions options =
        NSTrackingMouseMoved
        | NSTrackingMouseEnteredAndExited
        | NSTrackingActiveAlways
        | NSTrackingInVisibleRect
        | NSTrackingCursorUpdate
        | NSTrackingEnabledDuringMouseDrag;
    NSTrackingArea* area = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                         options:options
                                                           owner:self
                                                        userInfo:nil];
    [self addTrackingArea:area];
#if !__has_feature(objc_arc)
    [area release];
    [existing release];
#endif
}

- (void)clientPointFromEvent:(NSEvent*)event x:(float*)x y:(float*)y
{
    NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    if (x != nullptr) {
        *x = (float)point.x;
    }
    if (y != nullptr) {
        *y = (float)point.y;
    }
}

- (eokas::NativeWindow*)nativeWindow
{
    return static_cast<eokas::NativeWindow*>(self.owner);
}

- (BOOL)applyCursorAtX:(float)x y:(float)y
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr || !window->onCursor) {
        return NO;
    }
    eokas::NativeCursor cursor = eokas::NativeCursor::Arrow;
    if (!window->onCursor(x, y, cursor)) {
        return NO;
    }
    NSCursor* shape = [NSCursor arrowCursor];
    if (cursor == eokas::NativeCursor::SizeWestEast) {
        shape = [NSCursor resizeLeftRightCursor];
    } else if (cursor == eokas::NativeCursor::SizeNorthSouth) {
        shape = [NSCursor resizeUpDownCursor];
    }
    [shape set];
    return YES;
}

- (void)emitMove:(NSEvent*)event
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    [self clientPointFromEvent:event x:&x y:&y];
    if (window->onMouseMove) {
        window->onMouseMove(x, y);
    }
    [self applyCursorAtX:x y:y];
}

- (void)emitDown:(NSEvent*)event
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr || !window->onMouseDown) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    [self clientPointFromEvent:event x:&x y:&y];
    window->onMouseDown(x, y, eokas::transferMouseButton((int)event.buttonNumber));
}

- (void)emitUp:(NSEvent*)event
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr || !window->onMouseUp) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    [self clientPointFromEvent:event x:&x y:&y];
    window->onMouseUp(x, y, eokas::transferMouseButton((int)event.buttonNumber));
}

- (void)mouseMoved:(NSEvent*)event
{
    [self emitMove:event];
}

- (void)mouseDragged:(NSEvent*)event
{
    [self emitMove:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
    [self emitMove:event];
}

- (void)otherMouseDragged:(NSEvent*)event
{
    [self emitMove:event];
}

- (void)mouseEntered:(NSEvent*)event
{
    [self emitMove:event];
}

- (void)mouseDown:(NSEvent*)event
{
    [self emitDown:event];
}

- (void)rightMouseDown:(NSEvent*)event
{
    [self emitDown:event];
}

- (void)otherMouseDown:(NSEvent*)event
{
    [self emitDown:event];
}

- (void)mouseUp:(NSEvent*)event
{
    [self emitUp:event];
}

- (void)rightMouseUp:(NSEvent*)event
{
    [self emitUp:event];
}

- (void)otherMouseUp:(NSEvent*)event
{
    [self emitUp:event];
}

- (void)scrollWheel:(NSEvent*)event
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr || !window->onMouseWheel) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    [self clientPointFromEvent:event x:&x y:&y];
    float deltaX = (float)event.scrollingDeltaX;
    float deltaY = (float)event.scrollingDeltaY;
    if (!event.hasPreciseScrollingDeltas) {
        deltaX *= 48.0f;
        deltaY *= 48.0f;
    }
    if ((event.modifierFlags & NSEventModifierFlagShift) != 0 && deltaX == 0.0f) {
        deltaX = deltaY;
        deltaY = 0.0f;
    }
    window->onMouseWheel(x, y, -deltaX, -deltaY);
}

- (void)keyDown:(NSEvent*)event
{
    eokas::NativeWindow* window = [self nativeWindow];
    if (window == nullptr) {
        [super keyDown:event];
        return;
    }
    bool ctrl = (event.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl)) != 0;
    bool shift = (event.modifierFlags & NSEventModifierFlagShift) != 0;
    bool alt = (event.modifierFlags & NSEventModifierFlagOption) != 0;
    if (window->onKeyDown && window->onKeyDown(eokas::transferKeyCode((int)event.keyCode, false), false, ctrl, shift, alt)) {
        return;
    }

    bool consumed = false;
    NSString* chars = event.characters;
    if (window->onChar && chars != nil) {
        NSUInteger count = chars.length;
        for (NSUInteger index = 0; index < count;) {
            unichar unit = [chars characterAtIndex:index];
            uint32_t codepoint = unit;
            if (CFStringIsSurrogateHighCharacter(unit) && index + 1 < count) {
                unichar low = [chars characterAtIndex:index + 1];
                if (CFStringIsSurrogateLowCharacter(low)) {
                    codepoint = CFStringGetLongCharacterForSurrogatePair(unit, low);
                    index += 2;
                } else {
                    index += 1;
                }
            } else {
                index += 1;
            }
            if (codepoint >= 0xF700 && codepoint <= 0xF8FF) {
                continue;
            }
            if (window->onChar(codepoint)) {
                consumed = true;
            }
        }
    }
    if (!consumed) {
        [super keyDown:event];
    }
}

- (void)cursorUpdate:(NSEvent*)event
{
    float x = 0.0f;
    float y = 0.0f;
    [self clientPointFromEvent:event x:&x y:&y];
    if (![self applyCursorAtX:x y:y]) {
        [super cursorUpdate:event];
    }
}

@end

@interface EokasNativeWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) void* owner;
@end

@implementation EokasNativeWindowDelegate

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    (void)sender;
    return YES;
}

- (void)windowDidBecomeKey:(NSNotification*)notification
{
    NSWindow* window = notification.object;
    if (window.contentView != nil) {
        [window makeFirstResponder:window.contentView];
    }
}

- (void)windowDidResize:(NSNotification*)notification
{
    (void)notification;
    auto* window = static_cast<eokas::NativeWindow*>(self.owner);
    if (window == nullptr || !window->onClientSize) {
        return;
    }
    eokas::Vector2 size = window->clientSize();
    window->onClientSize(size.x, size.y);
}

- (void)windowWillClose:(NSNotification*)notification
{
    eokas::notifySystemClose((__bridge void*)notification.object);
}

@end

namespace eokas
{
    struct NativeWindowState
    {
        void* window = nullptr;
        void* delegate = nullptr;
        bool closing = false;
    };

    namespace
    {
        NativeWindowState& ensureState(NativeWindow& window)
        {
            if (!window.state) {
                window.state = std::make_unique<NativeWindowState>();
            }
            return *window.state;
        }

        NativeWindowState& platform(NativeWindow& window)
        {
            return ensureState(window);
        }

        const NativeWindowState& platform(const NativeWindow& window)
        {
            return *window.state;
        }

        void detachState(NativeWindowState& platformState)
        {
            platformState.window = nullptr;
            platformState.delegate = nullptr;
            platformState.closing = false;
        }

        void clearContentOwner(NSWindow* window)
        {
            NSView* content = window.contentView;
            if ([content isKindOfClass:[EokasNativeWindowView class]]) {
                ((EokasNativeWindowView*)content).owner = nil;
            }
        }

        void makeContentFirstResponder(NSWindow* window)
        {
            if (window.contentView != nil) {
                [window makeFirstResponder:window.contentView];
            }
        }
    }

    NativeWindow::NativeWindow() = default;

    NativeWindow::~NativeWindow()
    {
        this->destroy();
        state.reset();
    }

    bool NativeWindow::create(const String& title, float width, float height, NativeWindowStyle style)
    {
        if (state != nullptr && platform(*this).window) {
            return true;
        }

        ensureApp();

        float clientWidth = width < 0.0f ? 0.0f : width;
        float clientHeight = height < 0.0f ? 0.0f : height;
        NSRect content = NSMakeRect(0, 0, clientWidth, clientHeight);
        NSWindowStyleMask styleMask = style == NativeWindowStyle::Borderless
            ? NSWindowStyleMaskBorderless
            : (NSWindowStyleMaskTitled
                | NSWindowStyleMaskClosable
                | NSWindowStyleMaskMiniaturizable
                | NSWindowStyleMaskResizable);
        EokasNativeWindow* window = [[EokasNativeWindow alloc] initWithContentRect:content
                                                                         styleMask:styleMask
                                                                           backing:NSBackingStoreBuffered
                                                                             defer:NO];
        if (window == nil) {
            return false;
        }

        window.releasedWhenClosed = NO;
        window.acceptsMouseMovedEvents = YES;
        if (@available(macOS 10.14, *)) {
            window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
        }

        const char* utf8 = title.cstr();
        window.title = utf8 ? [NSString stringWithUTF8String:utf8] : @"";
        [window setContentSize:NSMakeSize(clientWidth, clientHeight)];
        if (style == NativeWindowStyle::Borderless) {
            window.movable = NO;
        } else {
            [window center];
        }

        EokasNativeWindowView* view = [[EokasNativeWindowView alloc] initWithFrame:NSMakeRect(0, 0, clientWidth, clientHeight)];
        view.owner = this;
        window.contentView = view;
#if !__has_feature(objc_arc)
        [view release];
#endif

        EokasNativeWindowDelegate* delegate = [[EokasNativeWindowDelegate alloc] init];
        delegate.owner = this;
        window.delegate = delegate;

        platform(*this).window = own(window);
        platform(*this).delegate = own(delegate);
        platform(*this).closing = false;

        gSystemClose[platform(*this).window] = [this]() {
            if (!platform(*this).window) {
                return;
            }
            platform(*this).closing = true;
            NSWindow* closing = nsWindow(platform(*this).window);
            clearContentOwner(closing);
            if (onClose) {
                auto callback = std::move(onClose);
                callback();
            }
            if (!platform(*this).window) {
                return;
            }
            void* ownedWindow = platform(*this).window;
            void* ownedDelegate = platform(*this).delegate;
            gSystemClose.erase(ownedWindow);
            closing.delegate = nil;
            detachState(platform(*this));
            autoreleaseOwned(ownedDelegate);
            autoreleaseOwned(ownedWindow);
        };
        return true;
    }

    void NativeWindow::destroy()
    {
        if (!state) {
            return;
        }
        if (!platform(*this).window || platform(*this).closing) {
            return;
        }
        platform(*this).closing = true;
        [nsWindow(platform(*this).window) close];
    }

    void NativeWindow::show(bool activate)
    {
        if (!platform(*this).window) {
            return;
        }
        ensureApp();
        NSWindow* window = nsWindow(platform(*this).window);
        if (activate) {
            [window makeKeyAndOrderFront:nil];
            makeContentFirstResponder(window);
            [NSApp activateIgnoringOtherApps:YES];
        } else {
            [window orderFront:nil];
        }
    }

    void NativeWindow::hide()
    {
        if (!platform(*this).window) {
            return;
        }
        [nsWindow(platform(*this).window) orderOut:nil];
    }

    void NativeWindow::close()
    {
        this->destroy();
    }

    void NativeWindow::setTitle(const String& title)
    {
        if (!platform(*this).window) {
            return;
        }
        const char* utf8 = title.cstr();
        nsWindow(platform(*this).window).title = utf8 ? [NSString stringWithUTF8String:utf8] : @"";
    }

    String NativeWindow::title() const
    {
        if (!platform(*this).window) {
            return String();
        }
        NSString* text = nsWindow(platform(*this).window).title;
        const char* utf8 = text ? text.UTF8String : nullptr;
        return utf8 ? String(utf8) : String();
    }

    void NativeWindow::setClientSize(float width, float height)
    {
        if (!platform(*this).window) {
            return;
        }
        if (width < 0.0f) {
            width = 0.0f;
        }
        if (height < 0.0f) {
            height = 0.0f;
        }
        NSWindow* window = nsWindow(platform(*this).window);
        NSRect frame = window.frame;
        NSRect content = [window contentRectForFrameRect:frame];
        CGFloat extraWidth = width - content.size.width;
        CGFloat extraHeight = height - content.size.height;
        frame.size.width += extraWidth;
        frame.size.height += extraHeight;
        frame.origin.y -= extraHeight;
        [window setFrame:frame display:YES];
    }

    Vector2 NativeWindow::clientSize() const
    {
        if (!platform(*this).window) {
            return Vector2();
        }
        NSWindow* window = nsWindow(platform(*this).window);
        NSRect content = [window contentRectForFrameRect:window.frame];
        return Vector2((float)content.size.width, (float)content.size.height);
    }

    void NativeWindow::setPosition(float x, float y)
    {
        if (!platform(*this).window) {
            return;
        }
        NSRect primary = primaryFrame();
        NSWindow* window = nsWindow(platform(*this).window);
        NSRect frame = window.frame;
        CGFloat originY = NSMaxY(primary) - y - frame.size.height;
        [window setFrameOrigin:NSMakePoint(x, originY)];
    }

    Vector2 NativeWindow::position() const
    {
        if (!platform(*this).window) {
            return Vector2();
        }
        NSRect primary = primaryFrame();
        NSRect frame = nsWindow(platform(*this).window).frame;
        float x = (float)frame.origin.x;
        float y = (float)(NSMaxY(primary) - NSMaxY(frame));
        return Vector2(x, y);
    }

    void NativeWindow::center()
    {
        if (!platform(*this).window) {
            return;
        }
        [nsWindow(platform(*this).window) center];
    }

    void NativeWindow::focus()
    {
        if (!platform(*this).window) {
            return;
        }
        ensureApp();
        NSWindow* window = nsWindow(platform(*this).window);
        [window makeKeyAndOrderFront:nil];
        makeContentFirstResponder(window);
    }

    void NativeWindow::minimize()
    {
        if (!platform(*this).window) {
            return;
        }
        [nsWindow(platform(*this).window) miniaturize:nil];
    }

    void NativeWindow::maximize()
    {
        if (!platform(*this).window) {
            return;
        }
        NSWindow* window = nsWindow(platform(*this).window);
        if (!window.zoomed) {
            [window zoom:nil];
        }
    }

    void NativeWindow::restore()
    {
        if (!platform(*this).window) {
            return;
        }
        NSWindow* window = nsWindow(platform(*this).window);
        if (window.miniaturized) {
            [window deminiaturize:nil];
        } else if (window.zoomed) {
            [window zoom:nil];
        }
    }

    bool NativeWindow::isOpen() const
    {
        return platform(*this).window != nullptr;
    }

    bool NativeWindow::isVisible() const
    {
        return platform(*this).window && nsWindow(platform(*this).window).visible;
    }

    bool NativeWindow::isMinimized() const
    {
        return platform(*this).window && nsWindow(platform(*this).window).miniaturized;
    }

    bool NativeWindow::isMaximized() const
    {
        return platform(*this).window && nsWindow(platform(*this).window).zoomed;
    }

    void NativeWindow::pumpMessages()
    {
        if (!platform(*this).window || NSApp == nil) {
            return;
        }
        void* window = platform(*this).window;
        while (NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                                   untilDate:[NSDate distantPast]
                                                      inMode:NSDefaultRunLoopMode
                                                     dequeue:YES]) {
            [NSApp sendEvent:event];
            if (platform(*this).window != window) {
                break;
            }
        }
    }

    void* NativeWindow::handle() const
    {
        if (!state) {
            return nullptr;
        }
        return platform(*this).window;
    }

    void* NativeWindow::display() const
    {
        return nullptr;
    }

}
